/*
 * Copyright (C) 2005 Carsten Rietzschel <crPLESAE@DOdaravNOT.SPAMde>
 *
 * This file is part of the schedtoold, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef UTILS_H
#define UTILS_H


/*
 * running as daemon
 */
extern int run_daemon;

/*
 * verbose level
 */
extern int verbose;

/*
 * termination handler (called on exit) 
 */
extern void terminationHandler (int signum);


/*
 * become a daemon via fork and write pid_file
 */
void daemonize (void);


/* 
 * message output (either via syslog (daemon) or via printf)
 */
void msg (char *output, ...);


/*
 * write pid into file 
 */
void writePidFile (char *filename);

#endif
