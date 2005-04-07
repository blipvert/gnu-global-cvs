/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002
 *	Tama Communications Corporation
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
#include <assert.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "die.h"
#include "dbop.h"
#include "gtagsop.h"
#include "makepath.h"
#include "gpathop.h"
#include "strlimcpy.h"

static DBOP *dbop;
static int _nextkey;
static int _mode;
static int opened;
static int created;

/*
 * gpath_open: open gpath tag file
 *
 *	i)	dbpath	GTAGSDBPATH
 *	i)	mode	0: read only
 *			1: create
 *			2: modify
 *	i)	flags	DBOP_POSTGRES
 *	r)		0: normal
 *			-1: error
 */
int
gpath_open(dbpath, mode, flags)
	const char *dbpath;
	int mode;
	int flags;
{
	assert(opened == 0);
	/*
	 * We create GPATH just first time.
	 */
	_mode = mode;
	if (mode == 1 && created)
		mode = 0;
	dbop = dbop_open(makepath(dbpath, dbname(GPATH), NULL), mode, 0644, flags);
	if (dbop == NULL)
		return -1;
	if (mode == 1)
		_nextkey = 1;
	else {
		char *p = dbop_get(dbop, NEXTKEY);

		if (p == NULL)
			die("nextkey not found in GPATH.");
		_nextkey = atoi(p);
	}
	opened = 1;
	return 0;
}
/*
 * gpath_put: put path name
 *
 *	i)	path	path name
 */
void
gpath_put(path)
	const char *path;
{
	char fid[32];

	assert(opened == 1);
	if (_mode == 1 && created)
		return;
	if (dbop_get(dbop, path) != NULL)
		return;
	snprintf(fid, sizeof(fid), "%d", _nextkey++);
	dbop_put(dbop, path, fid);
	dbop_put(dbop, fid, path);
}
/*
 * gpath_path2fid: convert path into id
 *
 *	i)	path	path name
 *	r)		file id
 */
char *
gpath_path2fid(path)
	const char *path;
{
	assert(opened == 1);
	return dbop_get(dbop, path);
}
/*
 * gpath_fid2path: convert id into path
 *
 *	i)	fid	file id
 *	r)		path name
 */
char *
gpath_fid2path(fid)
	const char *fid;
{
	return dbop_get(dbop, fid);
}
/*
 * gpath_delete: delete specified path record
 *
 *	i)	path	path name
 */
void
gpath_delete(path)
	const char *path;
{
	char *fid;

	assert(opened == 1);
	assert(_mode == 2);
	assert(path[0] == '.' && path[1] == '/');
	fid = dbop_get(dbop, path);
	if (fid == NULL)
		return;
	dbop_delete(dbop, fid);
	dbop_delete(dbop, path);
}
/*
 * gpath_nextkey: return next key
 *
 *	r)		next id
 */
int
gpath_nextkey(void)
{
	assert(_mode != 1);
	return _nextkey;
}
/*
 * gpath_close: close gpath tag file
 */
void
gpath_close(void)
{
	char fid[32];

	assert(opened == 1);
	opened = 0;
	if (_mode == 1 && created) {
		dbop_close(dbop);
		return;
	}
	snprintf(fid, sizeof(fid), "%d", _nextkey);
	if (_mode == 1 || _mode == 2)
		dbop_update(dbop, NEXTKEY, fid);
	dbop_close(dbop);
	if (_mode == 1)
		created = 1;
}

/*
 * gfind iterator using GPATH.
 *
 * gfind_xxx() does almost same with find_xxx() but much faster,
 * because gfind_xxx() use GPATH (file index).
 * If GPATH exist then you should use this.
 */
static DBOP *gfind_dbop;
static int gfind_opened;
static int gfind_first;
static char gfind_prefix[MAXPATHLEN+1];

/*
 * gfind_open: start iterator using GPATH.
 */
void
gfind_open(dbpath, local)
	char *dbpath;
	char *local;
{
	assert(gfind_opened == 0);
	assert(gfind_first == 0);
	gfind_dbop = dbop_open(makepath(dbpath, dbname(GPATH), NULL), 0, 0, 0);
	if (gfind_dbop == NULL)
		die("GPATH not found.");
	strlimcpy(gfind_prefix, (local) ? local : "./", sizeof(gfind_prefix));
	gfind_opened = 1;
	gfind_first = 1;
}
/*
 * gfind_read: read path without GPATH.
 *
 *	r)		path
 */
char	*
gfind_read(void)
{
	assert(gfind_opened == 1);
	if (gfind_first) {
		gfind_first = 0;
		return dbop_first(gfind_dbop, gfind_prefix, NULL, DBOP_KEY | DBOP_PREFIX);
	}
	return dbop_next(gfind_dbop);
}
/*
 * gfind_close: close iterator.
 */
void
gfind_close(void)
{
	dbop_close(gfind_dbop);
	gfind_opened = gfind_first = 0;
}
