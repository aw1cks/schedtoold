/*
 * Copyright (C) 2005 Carsten Rietzschel <crPLESAE@DOdaravNOT.SPAMde>
 *
 * This file is part of the schedtoold, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/* globals.h */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

/* local */
#include "pid-handler.h"
#include "utils.h"

/*
 * list of pids, pid=PID_FREE means empty slot, 
 * pid=PID_EOL is last element (to stop search as early as possible)
 */
list_entry_t pid_list[PID_MAX];

/* current age to kill closed processes from list */
static int age = 0;


/*
 * dump pid list
 */
void
pidListDump (void)
{
  int i;

  printf ("dump of pid list:\n");
  for (i = 0; i < PID_MAX - 1; i++)
  {
    if (pid_list[i].pid == PID_FREE)
      printf ("\t%3d: free\n", i);
    else if (pid_list[i].pid == PID_EOL)
    {
      printf ("\t%3d: --end of list--\n", i);
      return;
    }
    else
      printf ("\t%3d: PID=%5d Status=%i\n", i, pid_list[i].pid,
	      pid_list[i].status);
  }
}

/*
 * add pid to list
 * return index in pid_list
 */
static int
pidListAdd (int pid)
{
  int i;
  int free_entry = -1;

  for (i = 0; i < (PID_MAX - 1); i++)
  {
    if (pid_list[i].pid == pid)
    {
      /* found */
      pid_list[i].age = age;
      return i;
    }
    else if (pid_list[i].pid == PID_FREE)
    {
      /* remember first free entry */
      if (free_entry == -1)
	free_entry = i;
    }
    else if (pid_list[i].pid == PID_EOL)
    {
      /* reached end and not already found - so we have to add it */
      if (free_entry == -1)
	free_entry = i;

      pid_list[free_entry].pid = pid;
      /* mark for update */
      pid_list[free_entry].status = PID_STATUS_UNSET;
      pid_list[free_entry].age = age;
      return free_entry;
    }
  }
  /* error: too many entries in this list */
  msg ("[pids]   too many entries in in pid-list!");
  return -1;
}


/*
 * remove dead processes from list
 */
static void
pidListRemoveDead (void)
{
  int i;

  for (i = 0; i < (PID_MAX - 1); i++)
  {
    if (pid_list[i].pid == PID_EOL)
    {
      /* increase age for next loop */
      age++;
      return;
    }
    else if ((pid_list[i].age != age) && (pid_list[i].pid != PID_FREE))
    {
      /* it's removed */
      pid_list[i].pid = PID_FREE;
    }

  }
  age++;
}


/*
 * check status of pid 
 * return: PID_SET, PID_NOT_SED or PID_RETRY
 */
int
pidGetStatus (int index)
{
  return pid_list[index].status;
}


/*
 * set status of pid
 */
void
pidSetStatus (int index, int pid_value)
{
  pid_list[index].status = pid_value;
}


/*
 * simple filter out all direntries not beginning with 0-9
 */
static int
pidFilter (const struct dirent *entry)
{
  if ((entry->d_name[0] >= '0') && (entry->d_name[0] <= '9'))
    return 1;
  else
    return 0;
}


/*
 * read all pids from /proc
 * update in-memory list
 * attempt schedtool action
 */
int
pidUpdateViaProc ()
{
  struct dirent **namelist;
  int n;

  n = scandir ("/proc", &namelist, pidFilter, 0);
  if (n < 0)
  {
    perror ("scandir");
    return -1;
  }
  else
  {
    while (n--)
    {
      int pid;

      /* add to pid list */
      pid = atoi (namelist[n]->d_name);
      if (pid > PID_IGNORE)
	/* add to list */
	pidListAdd (pid);

      /* free element */
      free (namelist[n]);
    }
    /* free list */
    free (namelist);
  }

  /* remove dead pids from pid_list */
  pidListRemoveDead ();

  return 0;
}
