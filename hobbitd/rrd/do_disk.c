/*----------------------------------------------------------------------------*/
/* Hobbit RRD handler module.                                                 */
/*                                                                            */
/* Copyright (C) 2004-2005 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char disk_rcsid[] = "$Id: do_disk.c,v 1.21 2005-04-04 21:32:55 henrik Exp $";

static char *disk_params[] = { "rrdcreate", rrdfn, "DS:pct:GAUGE:600:0:100", "DS:used:GAUGE:600:0:U", 
				rra1, rra2, rra3, rra4, NULL };

/* This is ported almost directly from disk-larrd.pl */

int do_disk_larrd(char *hostname, char *testname, char *msg, time_t tstamp)
{
	enum { DT_IRIX, DT_AS400, DT_NT, DT_UNIX, DT_NETAPP, DT_NETWARE } dsystype;
	char *eoln, *curline;

	if (strstr(msg, " xfs ") || strstr(msg, " efs ") || strstr(msg, " cxfs ")) dsystype = DT_IRIX;
	else if (strstr(msg, "DASD")) dsystype = DT_AS400;
	else if (strstr(msg, "NetWare Volumes")) dsystype = DT_NETWARE;
	else if (strstr(msg, "NetAPP")) dsystype = DT_NETAPP;
	else if (strstr(msg, "Filesystem")) dsystype = DT_NT;
	else dsystype = DT_UNIX;

	curline = msg;
	while (curline)  {
		char *fsline, *p;
		char *columns[20];
		int columncount;
		char *diskname = NULL;
		int pused = -1;
		unsigned long long aused = 0;

		eoln = strchr(curline, '\n'); if (eoln) *eoln = '\0';

		/* AS/400 reports must contain the word DASD */
		if ((dsystype == DT_AS400) && (strstr(curline, "DASD") == NULL)) goto nextline;

		/* All clients except AS/400 report the mount-point with slashes - ALSO Win32 clients. */
		if ((dsystype != DT_AS400) && (strchr(curline, '/') == NULL)) goto nextline;

		/* red/yellow filesystems show up twice */
		if ((dsystype != DT_NETAPP) && (dsystype != DT_NETWARE)) {
			if (*curline == '&') goto nextline; 
			if ((strstr(curline, " red ") || strstr(curline, " yellow "))) goto nextline;
		}

		for (columncount=0; (columncount<20); columncount++) columns[columncount] = "";
		fsline = xstrdup(curline); columncount = 0; p = strtok(fsline, " ");
		while (p && (columncount < 20)) { columns[columncount++] = p; p = strtok(NULL, " "); }

		/* 
		 * Some Unix filesystem reports contain the word "Filesystem".
		 * So check if there's a slash in the NT filesystem letter - if yes,
		 * then it's really a Unix system after all.
		 */
		if ((dsystype == DT_NT) && strchr(columns[0], '/') && strlen(columns[5])) dsystype = DT_UNIX;

		switch (dsystype) {
		  case DT_IRIX:
			diskname = xstrdup(columns[6]);
			p = diskname; while ((p = strchr(p, '/')) != NULL) { *p = ','; }
			p = strchr(columns[5], '%'); if (p) *p = ' ';
			pused = atoi(columns[5]);
			aused = atoi(columns[3]);
			break;
		  case DT_AS400:
			diskname = xstrdup(",DASD");
			p = strchr(columns[12], '%'); if (p) *p = ' ';
			/* 
			 * Yikes ... the format of this line varies depending on the color.
			 * Red:
			 *    March 23, 2005 12:32:54 PM EST DASD on deltacdc at panic level at 90.4967% 
			 * Yellow: 
			 *    April 4, 2005 9:20:26 AM EST DASD on deltacdc at warning level at 81.8919%
			 * Green:
			 *    April 3, 2005 7:53:53 PM EST DASD on deltacdc OK at 79.6986%
			 *
			 * So we'll just pick out the number from the last column.
			 */
			pused = atoi(columns[columncount-1]);
			aused = 0; /* Not available */
			break;
		  case DT_NT:
			diskname = xmalloc(strlen(columns[0])+2);
			sprintf(diskname, ",%s", columns[0]);
			p = strchr(columns[4], '%'); if (p) *p = ' ';
			pused = atoi(columns[4]);
			aused = atoi(columns[2]);
			break;
		  case DT_UNIX:
			diskname = xstrdup(columns[5]);
			p = diskname; while ((p = strchr(p, '/')) != NULL) { *p = ','; }
			p = strchr(columns[4], '%'); if (p) *p = ' ';
			pused = atoi(columns[4]);
			aused = atoi(columns[2]);
			break;
		  case DT_NETAPP:
			diskname = xstrdup(columns[1]);
			p = diskname; while ((p = strchr(p, '/')) != NULL) { *p = ','; }
			pused = atoi(columns[5]);
			p = columns[3] + strspn(columns[3], "0123456789");
			aused = atoll(columns[3]);
			/* Convert to KB if there's a modifier after the numbers */
			if (*p == 'M') aused *= 1024;
			else if (*p == 'G') aused *= (1024*1024);
			else if (*p == 'T') aused *= (1024*1024*1024);
			break;
		  case DT_NETWARE:
			diskname = xstrdup(columns[1]);
			p = diskname; while ((p = strchr(p, '/')) != NULL) { *p = ','; }
			aused = atoll(columns[3]);
			pused = atoi(columns[7]);
			break;
		}

		if (diskname && (pused != -1)) {
			if (strcmp(diskname, ",") == 0) {
				diskname = xrealloc(diskname, 6);
				strcpy(diskname, ",root");
			}

			/* 
			 * Use testname here. 
			 * The disk-handler also gets data from NetAPP inode- and qtree-messages,
			 * that are virtually identical to the disk-messages. So lets just handle
			 * all of it by using the testname as part of the filename.
			 */
			sprintf(rrdfn, "%s%s.rrd", testname, diskname);
			sprintf(rrdvalues, "%d:%d:%llu", (int)tstamp, pused, aused);
			create_and_update_rrd(hostname, rrdfn, disk_params, update_params);
		}
		if (diskname) { xfree(diskname); diskname = NULL; }

		if (eoln) *eoln = '\n';
		xfree(fsline);

nextline:
		curline = (eoln ? (eoln+1) : NULL);
	}

	return 0;
}

