/*
 * Copyright (C) 2005 Carsten Rietzschel <crPLESAE@DOdaravNOT.SPAMde>
 *
 * This file is part of the schedtoold, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#ifndef PID_HANDLER_H
#define PID_HANDLER_H

/* various definitions */
#define PID_MAX  	32768
#define PID_FREE 	-1
#define PID_EOL 	0

#define PID_STATUS_UNSET	0	//not set
#define PID_STATUS_SET		1	//priority for this pid as already set
#define PID_STATUS_RETRY	2	//1st run of schedtool failed - retry

#define PID_IGNORE      1000	//ignore pids smaller PID_IGNORE


typedef struct list_entry
{
  pid_t pid;			// pid
  int status;			// priority already set or no config option to set
  int age;			// each loop increase age to detect closed processes
} list_entry_t;

/* in-memory list of pids in system */
extern list_entry_t pid_list[PID_MAX];


/*
 * dump pid list
 */
void pidListDump (void);

/*
 * check status of pid 
 * return: PID_SET, PID_NOT_SED or PID_RETRY
 */
inline int pidGetStatus (int index);

/*
 * set status of pid
 */
inline void pidSetStatus (int index, int pid_value);

/*
 * read all pids from /proc
 * update in-memory list
 * attempt schedtool action
 */
inline int pidUpdateViaProc (void);


#endif
