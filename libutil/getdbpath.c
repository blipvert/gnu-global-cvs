/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gparam.h"
#include "die.h"
#include "getdbpath.h"
#include "gtagsop.h"
#include "makepath.h"
#include "path.h"
#include "test.h"

static const char *makeobjdirprefix;	/* obj partition		*/
static const char *makeobjdir;		/* obj directory		*/

/*
 * gtagsexist: test whether GTAGS's existence.
 *
 *	i)	candidate candidate root directory
 *	o)	dbpath	directory which GTAGS exist
 *	i)	size	size of dbpath buffer
 *	i)	verbose	verbose mode 1: on, 0: off
 *	r)		0: not found, 1: found
 *
 * Gtagsexist locate GTAGS file in "$candidate/", "$candidate/obj/" and
 * "/usr/obj/$candidate/" in this order by default.
 * This behavior is same with BSD make(1)'s one.
 */
int
gtagsexist(candidate, dbpath, size, verbose)
char	*candidate;
char	*dbpath;
int	size;
{
	char	path[MAXPATHLEN+1];

#ifdef HAVE_SNPRINTF
	snprintf(path, sizeof(path), "%s/%s", candidate, dbname(GTAGS));
#else
	sprintf(path, "%s/%s", candidate, dbname(GTAGS));
#endif /* HAVE_SNPRINTF */
	if (verbose)
		fprintf(stderr, "checking %s\n", path);
	if (test("fr", path)) {
		if (verbose)
			fprintf(stderr, "GTAGS found at '%s'.\n", path);
#ifdef HAVE_SNPRINTF
		snprintf(dbpath, size, "%s", candidate);
#else
		sprintf(dbpath, "%s", candidate);
#endif /* HAVE_SNPRINTF */
		return 1;
	}
#ifdef HAVE_SNPRINTF
	snprintf(path, sizeof(path), "%s/%s/%s", candidate, makeobjdir, dbname(GTAGS));
#else
	sprintf(path, "%s/%s/%s", candidate, makeobjdir, dbname(GTAGS));
#endif /* HAVE_SNPRINTF */
	if (verbose)
		fprintf(stderr, "checking %s\n", path);
	if (test("fr", path)) {
		if (verbose)
			fprintf(stderr, "GTAGS found at '%s'.\n", path);
#ifdef HAVE_SNPRINTF
		snprintf(dbpath, size, "%s/%s", candidate, makeobjdir);
#else
		sprintf(dbpath, "%s/%s", candidate, makeobjdir);
#endif /* HAVE_SNPRINTF */
		return 1;
	}
#ifdef HAVE_SNPRINTF
	snprintf(path, sizeof(path), "%s%s/%s", makeobjdirprefix, candidate, dbname(GTAGS));
#else
	sprintf(path, "%s%s/%s", makeobjdirprefix, candidate, dbname(GTAGS));
#endif /* HAVE_SNPRINTF */
	if (verbose)
		fprintf(stderr, "checking %s\n", path);
	if (test("fr", path)) {
		if (verbose)
			fprintf(stderr, "GTAGS found at '%s'.\n", path);
#ifdef HAVE_SNPRINTF
		snprintf(dbpath, size, "%s%s", makeobjdirprefix, candidate);
#else
		sprintf(dbpath, "%s%s", makeobjdirprefix, candidate);
#endif /* HAVE_SNPRINTF */
		return 1;
	}
	return 0;
}
/*
 * getdbpath: get dbpath directory
 *
 *	o)	cwd	current directory
 *	o)	root	root of source tree
 *	o)	dbpath	directory which GTAGS exist
 *	i)	verbose	verbose mode 1: on, 0: off
 *
 * root and dbpath assumed as
 *	char	cwd[MAXPATHLEN+1];
 *	char	root[MAXPATHLEN+1];
 *	char	dbpath[MAXPATHLEN+1];
 *
 * At first, getdbpath locate GTAGS file in the current directory.
 * If not found, it move up to the parent directory and locate GTAGS again.
 * It repeat above behavior until GTAGS file is found or reach the system's
 * root directory '/'. If reached to '/' then getdbpath print "GTAGS not found."
 * and exit.
 */
void
getdbpath(cwd, root, dbpath, verbose)
char	*cwd;
char	*root;
char	*dbpath;
int	verbose;
{
	struct stat sb;
	char	*p;

	if (!getcwd(cwd, MAXPATHLEN))
		die("cannot get current directory.");
	canonpath(cwd);
	if ((p = getenv("MAKEOBJDIRPREFIX")) != NULL) {
		makeobjdirprefix = p;
		if (verbose)
			fprintf(stderr, "MAKEOBJDIRPREFIX is set to '%s'.\n", p);
	} else {
		makeobjdirprefix = "/usr/obj";
	}
	if ((p = getenv("MAKEOBJDIR")) != NULL) {
		makeobjdir = p;
		if (verbose)
			fprintf(stderr, "MAKEOBJDIR is set to '%s'.\n", p);
	} else {
		makeobjdir = "obj";
	}

	if ((p = getenv("GTAGSROOT")) != NULL) {
		if (verbose)
			fprintf(stderr, "GTAGSROOT is set to '%s'.\n", p);
		if (!isabspath(p))
			die("GTAGSROOT must be an absolute path.");
		if (stat(p, &sb) || !S_ISDIR(sb.st_mode))
			die("directory '%s' not found.", p);
		if (realpath(p, root) == NULL)
			die("cannot get real path of '%s'.", p);
		/*
		 * GTAGSDBPATH is meaningful only when GTAGSROOT exist.
		 */
		if ((p = getenv("GTAGSDBPATH")) != NULL) {
			if (verbose)
				fprintf(stderr, "GTAGSDBPATH is set to '%s'.\n", p);
			if (!isabspath(p))
				die("GTAGSDBPATH must be an absolute path.");
			if (stat(p, &sb) || !S_ISDIR(sb.st_mode))
				die("directory '%s' not found.", p);
			strcpy(dbpath, getenv("GTAGSDBPATH"));
		} else {
			if (!gtagsexist(root, dbpath, MAXPATHLEN, verbose))
				die("GTAGS not found.");
		}
	} else {
		if (verbose && getenv("GTAGSDBPATH"))
			fprintf(stderr, "warning: GTAGSDBPATH is ignored becase GTAGSROOT is not set.\n");
		/*
		 * start from current directory to '/' directory.
		 */
		strcpy(root, cwd);
		p = root + strlen(root);
		while (!gtagsexist(root, dbpath, MAXPATHLEN, verbose)) {
			while (*--p != '/' && p > root)
				;
			*p = 0;
			if (root == p)	/* reached root directory */
				break;
		}
		if (*root == 0)
			die("GTAGS not found.");
	}
}
