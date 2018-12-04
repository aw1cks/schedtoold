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
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

/* local */
#include "utils.h"
#include "config-handler.h"

/* 
 * run as daemon (default: yes) 
 */
int run_daemon = 1;

/*
 * verbose level
 */
int verbose = 0;

/*
 * ---------------------------------------------------------------------------- 
 * helper functions
 * ----------------------------------------------------------------------------  
 */




/*
 * output via syslog (daemon) or printf 
 */
void
msg (char *output, ...)
{
  va_list list;
  char infostr[256];

  va_start (list, output);
  vsnprintf (infostr, 256, output, list);
  va_end (list);
  if (run_daemon)
    syslog (LOG_DAEMON | LOG_INFO, infostr);
  else
    printf ("%s\n", infostr);
}



/*
 * termination handler (called when exiting)
 */
void
terminationHandler (int signum)
{
  /* free config entries */
  configFreeEntries ();

  /* bye */
  msg ("Terminated. Bye bye.");
  exit (EXIT_SUCCESS);
}


/*
 * write pidfile
 */
void
writePidFile (char *filename)
{
  pid_t pid = getpid ();
  int fd = open (filename, O_TRUNC | O_CREAT | O_WRONLY,S_IRUSR|S_IWUSR);
  char buf[6];

  snprintf (buf, 6, "%i", pid);
  if (fd >= 0)
  {
    /* write pid into */
    write (fd, &buf, strlen (buf));
    /* chmod it and close */
    fchmod (fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    close (fd);
  }
  else
    msg ("Warning! Writing pid_file failed (%s)!\n", strerror (errno));

}


/*
 * become a daemon via fork
 */
void
daemonize (void)
{
  pid_t pid;

  pid = fork ();
  switch (pid)
  {
  case -1:
    msg ("Error! fork sched failed, exiting.");
    exit (EXIT_FAILURE);
  case 0:
    setsid ();
    chdir ("/");
    umask (0);
    break;
  default:
    exit (EXIT_SUCCESS);
  }
}
