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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* local */
#include "config-handler.h"
#include "utils.h"

static config_entry_t config_first_entry = { NULL, NULL, 0, 0, 0, 0, NULL };
static config_entry_t *config_last_entry = &config_first_entry;


/*
 * read from filename to get userIDs or groupIDs from given name
 *
 * returns ID ;  on error: < 0
 * 
 * NOTE: we could read file only once and cached all data, but as it's done only 
 * at startup of schedtoold I'll keep it the simple way :)
 */
static int
configReadID (char *filename, char *name)
{
  FILE *file = fopen (filename, "r");
  char line[256];
  int id = -1;
  char read[256];

  if (file != NULL)
  {
    /* read until we found username */
    while (fgets (line, 255, file) != NULL)
    {
      /* 
       * scan for username:x:uid or groupname:x:gid 
       */
      sscanf (line, "%[a-zA-z0-9]:x:%i:\n", read, &id);
      if (!strncmp (read, name, 32))
      {
	/* found */
	fclose (file);
	return id;
      }
    }

    /* close file */
    fclose (file);
  }
  /* open failed or not found */
  return ID_NOT_FOUND;
}



/*
 * add entry to in-memory config list
 */
static int
configAddEntry (int uid, int gid, char *name, char *options, int wildcard,
		int absolute)
{
  config_entry_t *entry = malloc (sizeof (config_entry_t));
  if (entry == NULL)
    return -1;

  /* verbose */
  if (verbose)
    printf ("Added: %s %s%s (uid:%i,gid:%i)\n",
	    name, options, wildcard ? " (wildcard)" : "",
	    uid, gid);

  /* add strings */
  config_last_entry->next = entry;
  entry->name = name;
  entry->options = options;
  entry->wildcard = wildcard;
  entry->absolute = absolute;
  entry->gid = gid;
  entry->uid = uid;
  entry->next = NULL;

  /* remember last entry */
  config_last_entry = entry;

  /* done */
  return 0;
}


/*
 * free the whole in-memory list of config entries
 */
void
configFreeEntries ()
{
  config_entry_t *entry = config_first_entry.next;
  config_entry_t *next_entry;
  while (entry)
  {
    next_entry = entry->next;
    /* free strings */
    free (entry->name);
    free (entry->options);

    /* free entrie itself */
    free (entry);

    entry = next_entry;
  }
  config_first_entry.next = NULL;
}



/* 
 * reads configfile 
 */
int
configLoadFile (char *filename)
{
  FILE *file;
  char line[256];
  char scan_name[128];
  int linecount = 0;

  char *name;
  char *options;
  int options_index;		/* index of options in read line */
  int wildcard;
  int absolute;
  int uid;			/* user id */
  int gid;			/* group id */

  /* open */
  file = fopen (filename, "r");
  if (file == NULL)
  {
    printf ("Error! Reading configfile %s failed!\n", filename);
    return -1;
  }

  /* now read */
  while (fgets (line, 256, file) != NULL)
  {
    linecount++;

    /* ignore comments */
    if (line[0] == '#')
      continue;

    /* kill newline at end of line */
    line[strlen (line) - 1] = '\0';

    /* parse for name */
    scan_name[0] = '\0';
    sscanf (line, "%127s", scan_name);
    scan_name[127] = '\0';

    /* begin of options relavtive to line just read */
    options_index = strlen (scan_name);

    /* check if user or group is set */
    if (scan_name[options_index - 1] == ':')
    {
      /* found a user- or groupname */

      /* kill ':' */
      scan_name[options_index - 1] = '\0';

      /* group or user? */
      if (scan_name[0] == '%')
      {
	/* get group id */
	if (!strcmp (&scan_name[1], "ALL"))
	  gid = GID_ALL;
	else
	  gid = configReadID ("/etc/group", &scan_name[1]);

	/* valid for all users */
	uid = UID_ALL;
      }
      else
      {
	/* get user id */
	if (!strcmp (scan_name, "ALL"))
	  uid = UID_ALL;
	else
	  uid = configReadID ("/etc/passwd", scan_name);

	/* valid for all groups */
	gid = GID_ALL;
      }

      if ((uid != ID_NOT_FOUND) && (gid != ID_NOT_FOUND))
      {
	/* found - update scan_name */
	sscanf (&line[strlen (scan_name) + 1], "%127s", scan_name);
      }
      else
      {
	/* not found - invalid scanname to catch error later! */
	scan_name[0] = '\0';
	printf
	  ("Warning! Line %i in config file contains invalid user or group. Ignored.\n",
	   linecount);
      }

      /* update options postion */
      options_index += strlen (scan_name);

    }
    else
    {
      /* no user or group set: relevant for all users of all groups! */
      uid = UID_ALL;
      gid = GID_ALL;
    }

    /* create strings to be stored in in-memory list */
    name = strdup (scan_name);
    options = strdup (&line[options_index + 1]);

    /* to avoid stupid errors */
    if ((name == NULL) || (options == NULL))
    {
      printf ("Error! Malloc failed. Abort reading config file.\n");
      if (name)
	free (name);
      if (options)
	free (options);
      break;
    }

    /* valid? (name and at least one option) */
    if ((strlen (options) > 0) && (strlen (name) > 0))
    {
      /* 
       * check for wildcard 
       * XXX: the only valid wildcard is programname* (the * at the end!) 
       */
      if (name[strlen (name) - 1] == '*')
	wildcard = 1;
      else
	wildcard = 0;

      /* 
       * check for absolute paths to executable 
       */
      if (name[0] == '/')
	absolute = 1;
      else
	absolute = 0;

      /* add to in-memory list */
      if (configAddEntry (uid, gid, name, options, wildcard, absolute))
      {
	printf ("Error! Malloc failed. Abort reading config file.\n");
	if (name)
	  free (name);
	if (options)
	  free (options);
	break;
      }

    }
    else
    {
      /* invalid line */
      if (strlen (name) > 0)
	printf
	  ("Warning! Line %i in config file has no schedtool options.\n",
	   linecount);

      /* free strings */
      if (name)
	free (name);
      if (options)
	free (options);
      name = options = NULL;

    }


  }
  /* close */
  fclose (file);
  /* done */
  return 0;
}


/*
 * compare given statusname with names from config 
 * to handle absolute paths we need the path to the executable (exename)
 *
 * returns index in config_list if found
 * on error returns value <0
 */
config_entry_t *
configSearch (char *statusname, char *exename, uid_t uid, gid_t gid)
{
  config_entry_t *entry = config_first_entry.next;
  char *comparename = statusname;

  while (entry)
  {
    config_entry_t *found = NULL;


    if ((entry->absolute) && (strlen (exename) > 0))
      /* 
       * for config with full path and valid exename (symlink readable) 
       * we take take the exename 
       */
      comparename = exename;
    else
      /* 
       * compare the whole statusname if no path given in config
       * or no valid exename was found
       */
      comparename = statusname;

    /* compare */
    if (entry->wildcard)
    {
      /* for wildcard we compare all characters except the final '*'  */
      if (!strncmp (entry->name, comparename, strlen (entry->name) - 1))
	found = entry;
    }
    else
    {
      /* full string */
      if (!strcmp (entry->name, comparename))
	found = entry;
    }

    /* found? then compare gid and uid */
    if (found)
    {
      if (((entry->gid == GID_ALL) || (entry->gid == gid))
	  && ((entry->uid == UID_ALL) || (entry->uid == uid)))
	return found;

      if (verbose)
	msg ("Warning! uid/gid mismatch for %s (exe: %s). Ignored.",
	     statusname, exename);
    }

    /* next one */
    entry = entry->next;
  }

  /* not found */
  return NULL;
}
