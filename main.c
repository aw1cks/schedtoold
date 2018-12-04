/*
 * Copyright (C) 2005 Carsten Rietzschel <crPLESAE@DOdaravNOT.SPAMde>
 *
 * This file is part of the schedtoold, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* globals */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* local */
#include "utils.h"
#include "pid-handler.h"
#include "config-handler.h"

#define SCHED_VERSION "0.3"


#warning XXX - Make us non-global - please :)
static char exename[1024];	/* name with path from /proc/pid/exe */
static char statusname[1024];	/* name from /proc/pid/status */
static uid_t proc_uid;		/* uid of pid */
static gid_t proc_gid;		/* gid of pid */


/*
 * prints usage info 
 */
void
usage (void)
{
  printf
    ("schedtoold %s: daemon to automagically update priority/nice-level of running programs.\n",
     SCHED_VERSION);
  printf (" [-h]\t\tshow this help\n"
	  " [-v]\t\tbe more verbose\n"
	  " [-n]\t\tstay in foreground (no daemon)\n"
	  " [-u sec]           update interval in seconds\n"
	  " [-c config_file]   load given config file (default: /etc/schedtoold.conf)\n"
	  " [-p pid_file]      filename to store pid in\n");
  exit (EXIT_SUCCESS);
}


/*
 * get process info from proc
 * return: 0 on sucess
 * error: -1 
 */
static inline int
getProcInfo (pid_t pid)
{
  int fd;
  int exename_len;

  /*
   * We get the name of program is it was started from /proc/pid/status
   * and we also get the exe-symlink which is running.
   * This is necessary for all scripts:
   *   /proc/pid/status shows the name as it was started (e.g. scriptname)
   *   and /proc/pid/exe is symlinked e.g. to bash
   * If we found one script running we're looking into /proc/pid/cmdline
   * and trying to get the absolute path to the 
   *
   * XXX - this is not perfect :(
   * Any ideas how to get the absolute path of the program (script,binary,..)?
   */

  /*
   *  get name from /proc/pid/status 
   */
  snprintf (statusname, 1023, "/proc/%i/status", pid);
  fd = open (statusname, O_RDONLY);
  if (fd > 0)
  {
    char buf[512];
    char *idstart;

    if (read (fd, buf, 511) > 0)
    {
      if (sscanf (buf, "Name:\t%511s\n", statusname) != 1)
      {
	/* error in conversation */
	close (fd);
	return -1;
      }

      /* check UID */
      idstart = strstr (buf, "Uid:");
      if ((idstart == NULL)
	  || (sscanf (idstart, "Uid:\t%i\t", &proc_uid) != 1))
      {
	/* either error in conversation  */
	close (fd);
	return -1;
      }

      /* get GID */
      idstart = strstr (buf, "Gid:");
      if ((idstart == NULL)
	  || (sscanf (idstart, "Gid:\t%i\t", &proc_gid) != 1))
      {
	/* either error in conversation  */
	close (fd);
	return -1;
      }

    }
    close (fd);
  }
  else
    statusname[0] = '\0';	/* make name read invalid */


  /*
   * get name from /proc/pid/exe-symlink and if necessary also get
   * /proc/pid/cmdline
   */
  snprintf (exename, 1023, "/proc/%i/exe", pid);
  if ((exename_len = readlink (exename, exename, 1023)) > 0)
  {
    char *binaryname;		/* the binary without path from exename */
    char cmdline[1024];		/* proc/pid/cmdline */
    int cmdline_len;		/* bytes read */

    /* readlink success and append final NULL to string */
    exename[exename_len] = '\0';
    binaryname = strrchr (exename, '/');
    binaryname++;
    /* compare exe- vs. statusname to detect scripts */
    if (strcmp (binaryname, statusname))
    {
      snprintf (cmdline, 1023, "/proc/%i/cmdline", pid);
      fd = open (cmdline, O_RDONLY);
      if (fd > 0)
      {
	if ((cmdline_len = read (fd, &cmdline, 1023)) > 0)
	{
	  /* 
	   * for all params we check if we found status name - hoping it has an 
	   * absolute path given 
	   */
	  char *param = cmdline;

	  while (param != NULL)
	  {
	    if ((cmdline_len -= strlen (param) + 1) <= 0)
	      break;

	    /* is this the param containing the status name? */
	    binaryname = strrchr (param, '/');
	    if (binaryname != NULL)
	    {
	      binaryname++;
	      if (!strcmp (binaryname, statusname))
	      {
		strncpy (exename, param, 1024);
		break;
	      }
	    }

	    /* get next param */
	    param = (char *) memchr (param, '\0', cmdline_len);
	    if (param != NULL)
	      param++;		/* the charater after \0 is relevant */

	  }
	}
	close (fd);
      }
    }
  }
  else
    exename[0] = '\0';		/* make name read invalid */

  /* sucess */
  return 0;
}


/*
 * just the main w/ work loop
 */
int
main (int argc, char *argv[])
{
  int c;
  char *config_file = NULL;
  char *pid_file = NULL;
  uid_t current_uid;		/* uid of schedtoold itself */
  unsigned int update_sec = 2;	/* time between to updates in seconds */
  char *buf = exename;		/* don't waste memory but make code a bit more readable */


  /* check if we're running as root, if not warn */
  if ((current_uid = getuid ()) != 0)
  {
    printf
      ("Warning! Not running with UID=0 (root)!\n         Setting priorities normally requires root privileges.\n");
    printf ("Only programs with UID=%i will be touched by sched.\n\n",
	    current_uid);
  }


  /* process cmdline arguments */
  while ((c = getopt (argc, argv, "vhnc:p:u:")) != -1)
  {
    switch (c)
    {
    case 'n':
      run_daemon = 0;
      break;
    case 'v':
      verbose++;
      break;
    case 'h':
      usage ();
      break;
    case 'c':
      config_file = optarg;
      break;
    case 'p':
      pid_file = optarg;
      break;
    case 'u':
      update_sec = atoi (optarg);
      if (update_sec < 1)
      {
	printf ("Error! Update interval must be at least one second!\n");
	exit (EXIT_FAILURE);
      }
      break;
    case '?':
      usage ();
      break;
    }
  }


  /* load config */
  if (config_file == NULL)
  {
    /* no config give via cmdline */
    if (current_uid == 0)
      config_file = "/etc/schedtoold.conf";
    else
    {
      /* non-root user get it from home */
      snprintf (buf, 128, "%s/.schedtoold.conf", getenv ("HOME"));
      config_file = buf;
    }
  }

  if (configLoadFile (config_file))
    exit (EXIT_FAILURE);

  /* now we daemonize */
  if (run_daemon)
    daemonize ();

  /* write pid file */
  if (pid_file)
    writePidFile (pid_file);

  /* Invoke terminationHandler when one of these signals happens */
  signal (SIGTERM, terminationHandler);
  signal (SIGINT, terminationHandler);
  signal (SIGHUP, terminationHandler);
  signal (SIGQUIT, terminationHandler);

  /* verbose into syslog */
  msg ("Ready: version %s", SCHED_VERSION);

  /* main loop */
  while (1)
  {
    int i;

    /* update list */
    pidUpdateViaProc ();

    /* check which pids should be updated */
    for (i = 0; i < PID_MAX - 1; i++)
    {
      /* end of list? */
      if (pid_list[i].pid == PID_EOL)
	break;

      /* needs to be updated? */
      if ((pidGetStatus (i) != PID_STATUS_SET)
	  && (pid_list[i].pid != PID_FREE))
      {
	config_entry_t *config_entry;	/* config entry */

	/* 
	 * get info from /proc for pid
	 */
	if (getProcInfo (pid_list[i].pid) != 0)
	{
	  /* couldn't get /proc-infos  */
	  config_entry = NULL;	/* next one */
	  if (verbose)
	    msg ("Error: Couldn't get info from /proc/%i\n", pid_list[i].pid);
	}
	else
	{
	  if ((current_uid == 0) || (current_uid == proc_uid))
	    /* got info - search in out config */
	    config_entry =
	      configSearch (statusname, exename, proc_uid, proc_gid);
	  else
	    /* not running as root and uid doesn't match */
	    config_entry = NULL;
	}

	if (config_entry != NULL)
	{
	  /* we found a valid one to update its priority */

	  /* verbose */
	  if (verbose)
	    msg
	      ("Updating: %s%s (exe:%s, pid:%i, options:%s)",
	       statusname,
	       pidGetStatus (i) == PID_STATUS_RETRY ? " (retry)" : "", exename,
	       pid_list[i].pid, config_entry->options);

	  /* run schedtool */
	  snprintf (buf, 1023, "schedtool %s %i",
		    config_entry->options, pid_list[i].pid);
	  if (system (buf))
	  {
	    /* show error message only at 2nd try - cause program could be already exited when trying 1st time */
	    if (pidGetStatus (i) == PID_STATUS_RETRY)
	    {
	      msg
		("Error! Running \"schedtool %s\" on %s (pid: %i) failed.",
		 config_entry->options, statusname, pid_list[i].pid);
	      /* don't try again */
	      pidSetStatus (i, PID_STATUS_SET);
	    }
	    else
	      /* retry next time */
	      pidSetStatus (i, PID_STATUS_RETRY);
	  }
	  else
	    /* success: mark set */
	    pidSetStatus (i, PID_STATUS_SET);

	}
	else
	  /* not found - but mark as set to avoid re-check */
	  pidSetStatus (i, PID_STATUS_SET);
      }
    }

    /* wait until a process is created/closed */
    sleep (update_sec);
    /* ?? XXX - is there a better way than polling ?? */
  }

  /* bye */
  msg ("Have a nice day.");

  /* free memory */
  configFreeEntries ();

  return 0;
};
