/*----------------------------------------------------------------------------*/
/* Hobbit message daemon.                                                     */
/*                                                                            */
/* This is a hobbitd worker module for the "status" and "data" channels.      */
/* This module maintains the RRD database-files, updating them as new         */
/* data arrives.                                                              */
/*                                                                            */
/* Copyright (C) 2004 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: hobbitd_rrd.c,v 1.10 2004-12-30 22:25:34 henrik Exp $";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>

#include "libbbgen.h"
#include "hobbitd_worker.h"

#include "do_larrd.h"

#define MAX_META 20	/* The maximum number of meta-data items in a message */


void sig_handler(int signum)
{
	switch (signum) {
	  case SIGCHLD:
		  break;
	}
}

int main(int argc, char *argv[])
{
	char *msg;
	int running;
	int argi, seq;
	struct sigaction sa;

	/* Handle program options. */
	for (argi = 1; (argi < argc); argi++) {
		if (strcmp(argv[argi], "--debug") == 0) {
			debug = 1;
		}
		else if (argnmatch(argv[argi], "--rrddir=")) {
			char *p = strchr(argv[argi], '=');
			rrddir = strdup(p+1);
		}
	}

	if ((rrddir == NULL) && getenv("BBRRDS")) {
		rrddir = strdup(getenv("BBRRDS"));
	}

	save_errbuf = 0;
	setup_signalhandler("hobbitd_larrd");
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;
	sigaction(SIGCHLD, &sa, NULL);
	signal(SIGPIPE, SIG_DFL);

	running = 1;
	while (running) {
		char *eoln, *restofmsg = NULL;
		char *metadata[MAX_META+1];
		int metacount;
		char *p;
		char *hostname = NULL, *testname = NULL;
		larrdrrd_t *ldef = NULL;
		time_t tstamp;
		int childstat;

		/* Get next message */
		msg = get_hobbitd_message(argv[0], &seq, NULL);
		if (msg == NULL) {
			running = 0;
			continue;
		}

		/* Split the message in the first line (with meta-data), and the rest */
 		eoln = strchr(msg, '\n');
		if (eoln) {
			*eoln = '\0';
			restofmsg = eoln+1;
		}

		/* Parse the meta-data */
		metacount = 0; 
		p = gettok(msg, "|");
		while (p && (metacount < MAX_META)) {
			metadata[metacount++] = p;
			p = gettok(NULL, "|");
		}
		metadata[metacount] = NULL;

		if (strncmp(metadata[0], "@@shutdown", 10) == 0) {
			running = 0;
			continue;
		}
		else if ((metacount > 3) && (strncmp(metadata[0], "@@drophost", 10) == 0)) {
			char hostdir[PATH_MAX];
			hostname = metadata[3];

			sprintf(hostdir, "%s/%s", rrddir, hostname);
			dropdirectory(hostdir, 1);
		}
		else if ((metacount > 4) && (strncmp(metadata[0], "@@droptest", 10) == 0)) {
			/*
			 * Not implemented. Mappings of testnames -> larrd rrd files is
			 * too complex, so on the rare occasion that a single test
			 * is deleted, they will have to delete the rrd files themselves.
			 */
		}
		else if ((metacount > 4) && (strncmp(metadata[0], "@@renamehost", 12) == 0)) {
			char oldhostdir[PATH_MAX];
			char newhostdir[PATH_MAX];
			char *newhostname;

			hostname = metadata[3];
			newhostname = metadata[4];
			sprintf(oldhostdir, "%s/%s", rrddir, hostname);
			sprintf(newhostdir, "%s/%s", rrddir, newhostname);
			rename(oldhostdir, newhostdir);
		}
		else if ((metacount > 5) && (strncmp(metadata[0], "@@renametest", 12) == 0)) {
			/* Not implemented. See "droptest". */
		}
		else if ((metacount >= 14) && (strncmp(metadata[0], "@@status", 8) == 0)) {
			/*
			 * @@status|timestamp|sender|origin|hostname|testname|expiretime|color|testflags|\
			 * prevcolor|changetime|ackexpiretime|ackmessage|disableexpiretime|disablemessage 
			 */
			int color = parse_color(metadata[7]);

			switch (color) {
			  case COL_GREEN:
			  case COL_YELLOW:
			  case COL_RED:
				tstamp = atoi(metadata[1]);
				hostname = metadata[4]; 
				testname = metadata[5];
				ldef = find_larrd_rrd(testname, metadata[8]);
				update_larrd(hostname, testname, restofmsg, tstamp, ldef);
				break;

			  default:
				/* Ignore reports with purple, blue or clear - they have no data we want. */
				break;
			}
		}
		else if ((metacount > 5) && (strncmp(metadata[0], "@@data", 6) == 0)) {
			/* @@data|timestamp|sender|origin|hostname|testname */
			tstamp = atoi(metadata[1]);
			hostname = metadata[4]; 
			testname = metadata[5];
			ldef = find_larrd_rrd(testname, "");
			update_larrd(hostname, testname, restofmsg, tstamp, ldef);
		}

		/* 
		 * We fork a subprocess when processing drophost requests.
		 * Pickup any finished child processes to avoid zombies
		 */
		while (wait3(&childstat, WNOHANG, NULL) > 0) ;
	}

	return 0;
}

