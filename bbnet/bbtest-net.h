/*----------------------------------------------------------------------------*/
/* Big Brother network test tool.                                             */
/*                                                                            */
/* Copyright (C) 2003 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef __BBTEST_NET_H__
#define __BBTEST_NET_H__

#include <sys/time.h>

#ifdef MULTICURL
/*
 * libcurl multi interface requires longer timeouts.
 * Not quite sure why ... but it is not so important,
 * as many of these run in parallel. So the overall
 * impact on the run-time is limited.
 */
#define DEF_TIMEOUT         30
#define DEF_CONNECT_TIMEOUT 15
#else
#define DEF_TIMEOUT         10
#define DEF_CONNECT_TIMEOUT 5
#endif

#define STATUS_CONTENTMATCH_NOFILE 901
#define STATUS_CONTENTMATCH_FAILED 902
#define STATUS_CONTENTMATCH_BADREGEX 903

typedef struct svcinfo_t {
	char *svcname;
	char *sendtxt;
	char *exptext;
	unsigned int flags;
} svcinfo_t;

typedef struct service_t {
	char *testname;		/* Name of the test = column name in BB report */
	int namelen;		/* Length of name - "testname:port" has this as strlen(testname), others 0 */
	int portnum;		/* Port number this service runs on */
	int toolid;		/* Which tool to use */
	struct testitem_t *items; /* testitem_t linked list of tests for this service */
	struct service_t *next;
} service_t;

typedef struct testedhost_t {
	char *hostname;
	char ip[16];
	int conntimeout;	/* Connection timeout (http test only) */
	int timeout;		/* Host timeout setting (http test only) */
	int dialup;		/* dialup flag (if set, failed tests report as clear) */
	int testip;		/* testip flag (dont do dns lookups on hostname) */
	int nosslcert;		/* nosslcert flag */
	int dodns;              /* set while loading tests if we need to do a DNS lookup */
	int dnserror;		/* set internally if we cannot find the host's IP */
	int okexpected;		/* set internally if outside DOWNTIME period. If not, failed tests are blue */
	int repeattest;         /* Set if this host goes on the quick poll list */

	/* The following are for the connectivity test */
	int noconn;		/* noconn flag (dont report "conn" at all */
	int noping;		/* noping flag (report "conn" as clear=disabled */
	int badconn[3];		/* badconn:x:y:z flag */
	int downcount;		/* number of successive failed conn tests */
	time_t downstart;	/* time() of first conn failure */
	char *routerdeps;       /* Hosts from the "router:" tag */
	struct testedhost_t *deprouterdown;    /* Set if dependant router is down */

	/* The following is for the HTTP tests */
	struct testitem_t *firsthttp;	/* First HTTP testitem in testitem list */

	/* The following is for the LDAP tests */
	struct testitem_t *firstldap;	/* First LDAP testitem in testitem list */
	char   *ldapuser;		/* Username */
	char   *ldappasswd;		/* Password */

	/* For storing the test dependency tag. */
	char *deptests;

	struct testedhost_t *next;
} testedhost_t;

typedef struct testitem_t {
	struct testedhost_t *host;	/* Pointer to testedhost_t record for this host */
	struct service_t *service;	/* Pointer to service_t record for the service to test */

	char		*testspec;      /* Pointer to the raw testspec in bb-hosts */
	int		reverse;	/* "!testname" flag */
	int		dialup;		/* "?testname" flag */
	int		alwaystrue;	/* "~testname" flag */
	int		silenttest;	/* "testname:s" flag */

	/* These data may be filled in from the test engine private data */
	int		open;		/* Is the service open ? NB: Shows true state of service, ignores flags */
	char		*banner;
	char		*certinfo;
	time_t		certexpires;
	struct timeval	duration;

	/* For badTEST handling: Need to track downtime duration and poll count */
	int		badtest[3];
	time_t		downstart;
	int		downcount;	/* Number of polls when down. */

	/* Each test engine has its own data */
	void		*privdata;	/* Private data use by test tool */

	struct testitem_t *next;
} testitem_t;

typedef struct modembank_t {
	char		*hostname;
	unsigned int 	startip;	/* Saved as 32-bit binary */
	int		banksize;
	int		*responses;
} modembank_t;

extern char *deptest_failed(testedhost_t *host, char *testname);

#endif

