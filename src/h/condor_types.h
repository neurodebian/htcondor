/***************************Copyright-DO-NOT-REMOVE-THIS-LINE**
 * CONDOR Copyright Notice
 *
 * See LICENSE.TXT for additional notices and disclaimers.
 *
 * Copyright (c)1990-1998 CONDOR Team, Computer Sciences Department, 
 * University of Wisconsin-Madison, Madison, WI.  All Rights Reserved.  
 * No use of the CONDOR Software Program Source Code is authorized 
 * without the express consent of the CONDOR Team.  For more information 
 * contact: CONDOR Team, Attention: Professor Miron Livny, 
 * 7367 Computer Sciences, 1210 W. Dayton St., Madison, WI 53706-1685, 
 * (608) 262-0856 or miron@cs.wisc.edu.
 *
 * U.S. Government Rights Restrictions: Use, duplication, or disclosure 
 * by the U.S. Government is subject to restrictions as set forth in 
 * subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer 
 * Software clause at DFARS 252.227-7013 or subparagraphs (c)(1) and 
 * (2) of Commercial Computer Software-Restricted Rights at 48 CFR 
 * 52.227-19, as applicable, CONDOR Team, Attention: Professor Miron 
 * Livny, 7367 Computer Sciences, 1210 W. Dayton St., Madison, 
 * WI 53706-1685, (608) 262-0856 or miron@cs.wisc.edu.
****************************Copyright-DO-NOT-REMOVE-THIS-LINE**/

 


#ifndef CONDOR_TYPES_INCLUDED
#define CONDOR_TYPES_INCLUDED


#include "condor_expressions.h"

#if !defined(AIX31) && !defined(AIX32)
#define NFDS(x) (x)
#endif


#if !defined(LINUX)
struct qelem {
	struct qelem *q_forw;
	struct qelem *q_back;
	char	q_data[1];
};
#endif

/* struct bucket definition removed -- duplicate of definition in config.h */

#if !defined(ULTRIX42) || defined(ULTRIX43)
struct fs_data_req {
	dev_t	dev;
	char	*devname;
	char	*path;
};
struct fs_data {
	struct fs_data_req fd_req;
};
#endif


#if !defined(RUSAGE_SELF)

#if defined(_POSIX_SOURCE) && !defined(WIN32)
#define SAVE_POSIX_DEF _POSIX_SOURCE
#define SAVE_POSIX_4DEF _POSIX_4SOURCE
#undef _POSIX_SOURCE
#undef _POSIX_4SOURCE
#include <sys/time.h>
#define _POSIX_SOURCE SAVE_POSIX_DEF
#define _POSIX_4SOURCE SAVE_POSIX_4DEF
#elif !defined(WIN32)
#include <sys/time.h>
#endif /* _POSIX_SOURCE */

#if !defined(WIN32)
#include <sys/resource.h>
#endif

#endif	/* RUSAGE_SELF */

/* XDR is not POSIX.1 conforming, and requires the following typedefs */
#if defined(_POSIX_SOURCE)
#if defined(__cplusplus) 
typedef unsigned int u_int;
typedef char * caddr_t;
#endif	/* __cplusplus */
#endif	/* _POSIX_SOURCE */

#define MALLOC_DEFINED_AS_VOID
#if defined(OSF1)
#define malloc hide_malloc
#endif
#if defined(OSF1)
#undef malloc
#define mem_alloc(bsize)        malloc(bsize)
#endif


#include "proc.h"

typedef struct status_line {
	char	*name;
	int		run;
	int		tot;
	int		prio;
	char	*state;
	float	load_avg;
	int		kbd_idle;
	char	*arch;
	char	*op_sys;
} STATUS_LINE;


#endif
