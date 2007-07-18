/*----------------------------------------------------------------------------*/
/* Hobbit message daemon.                                                     */
/*                                                                            */
/* Client backend module for BBWin/Windoes client                             */
/*                                                                            */
/* Copyright (C) 2006-2007 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char bbwin_rcsid[] = "$Id: bbwin.c,v 1.2 2007-07-18 21:20:15 henrik Exp $";

void handle_win32_bbwin_client(char *hostname, char *clienttype, enum ostype_t os, 
				void *hinfo, char *sender, time_t timestamp,
				char *clientdata)
{
	/* Stub routine for the BBWin backend parser. Waiting for Etienne to provide this. */
}

