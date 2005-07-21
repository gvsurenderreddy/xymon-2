/*----------------------------------------------------------------------------*/
/* Hobbit message daemon.                                                     */
/*                                                                            */
/* Client backend module for NetBSD                                           */
/*                                                                            */
/* Copyright (C) 2005 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char netbsd_rcsid[] = "$Id: netbsd.c,v 1.1 2005-07-21 14:54:34 henrik Exp $";

void handle_netbsd_client(char *hostname, char *sender, time_t timestamp, char *clientdata)
{
	char *timestr;
	char *uptimestr;
	char *whostr;
	char *psstr;
	char *topstr;
	char *dfstr;
	char *meminfostr;
	char *swapctlstr;
	char *netstatstr;
	char *vmstatstr;

	char *p;

	unsigned long memphystotal, memphysused, memphysfree,
		      memactused, memactfree,
		      memswaptotal, memswapused, memswapfree;

	char fromline[1024];

	sprintf(fromline, "\nStatus message received from %s\n", sender);

	splitmsg(clientdata);

	timestr = getdata("date");
	uptimestr = getdata("uptime");
	whostr = getdata("who");
	psstr = getdata("ps");
	topstr = getdata("top");
	dfstr = getdata("df");
	meminfostr = getdata("meminfo");
	swapctlstr = getdata("swapctl");
	netstatstr = getdata("netstat");
	vmstatstr = getdata("vmstat");

	combo_start();

	unix_cpu_report(hostname, fromline, timestr, uptimestr, whostr, psstr, topstr);
	unix_disk_report(hostname, fromline, timestr, dfstr);

	if (meminfostr) {
		unsigned long memphystotal, memphysfree, memphysused;
		unsigned long memswaptotal, memswapfree, memswapused;
		int found = 0;

		p = strstr(meminfostr, "Total:"); if (p) { memphystotal = atol(p+6); found++; }
		p = strstr(meminfostr, "Free:");  if (p) { memphysfree  = atol(p+5); found++; }
		memphysused = memphystotal - memphysfree;
		p = strstr(meminfostr, "Swaptotal:"); if (p) { memswaptotal = atol(p+10); found++; }
		p = strstr(meminfostr, "Swapused:");  if (p) { memswapused  = atol(p+9); found++; }
		memswapfree = memswaptotal - memswapused;

		if (found == 4) {
			unix_memory_report(hostname, fromline, timestr,
				   memphystotal, memphysused, -1, memswaptotal, memswapused);
		}
	}

	combo_end();

	unix_netstat_report(hostname, "netbsd", netstatstr);
	unix_vmstat_report(hostname, "netbsd", vmstatstr);
}

