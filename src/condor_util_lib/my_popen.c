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

 

#define _POSIX_SOURCE
#include "condor_common.h"
#include "condor_debug.h"
#include <signal.h>
#include <sys/wait.h>

static char *_FileName_ = __FILE__;

extern int	errno;

static pid_t	ChildPid;

static int	READ_END = 0;
static int	WRITE_END = 1;

#if defined(__STDC__)
static void	sigchld_eater( int );
#else
static void	sigchld_eater();
#endif

static struct sigaction	OldAct;

/*
  This is just like popen(3) except it isn't quite as smart in some
  ways, but is smarter in others.  This one is less smart in that it
  only allows for one process at a time.  It is smarter than at least
  some implementation of standard popen() in that it handles
  termination of the child process more carefully.  First, it is aware
  that the calling code may be set up to catch SIGCHLD.  Since one
  instance of SIGCHLD will be generated by each call to my_popen(),
  each call must eat one instance of SIGCHLD.  Secondly, it is careful
  how it waits for it's child's status so that it doesn't reap status
  information for other processes which the calling code may want to
  reap.
*/
FILE *
my_popen( cmd, mode )
char *cmd;
char *mode;
{
	int		pipe_d[2];
	int		parent_reads;
	struct sigaction act;
	sigset_t	mask;

		/* Figure out who reads and who writes on the pipe */
	parent_reads = mode[0] == 'r';

		/* Creat the pipe */
	if( pipe(pipe_d) < 0 ) {
		return NULL;
	}


		/* Set up our own handler for SIGCHLD */
	act.sa_handler = sigchld_eater;
	sigemptyset( &act.sa_mask );
	act.sa_flags = 0;
	if( sigaction(SIGCHLD,&act,&OldAct) < 0 ) {
		EXCEPT( "sigaction" );
		exit( 1 );
	}


		/* Create a new process */
	if( (ChildPid=fork()) < 0 ) {
			/* Clean up file descriptors */
		close( pipe_d[0] );
		close( pipe_d[1] );
			/* Restore original handler for SIGCHLD */
		if( sigaction(SIGCHLD,&OldAct,0) < 0 ) {
			EXCEPT( "sigaction" );
		}
		return NULL;
	}

		/* The child */
	if( ChildPid == 0 ) {
		if( parent_reads ) {
				/* Close stdin, dup pipe to stdout */
			close( pipe_d[READ_END] );
			if( pipe_d[WRITE_END] != 1 ) {
				dup2( pipe_d[WRITE_END], 1 );
				close( pipe_d[WRITE_END] );
			}
		} else {
				/* Close stdout, dup pipe to stdin */
			close( pipe_d[WRITE_END] );
			if( pipe_d[READ_END] != 0 ) {
				dup2( pipe_d[READ_END], 0 );
				close( pipe_d[READ_END] );
			}
		}
		execl( "/bin/sh", "sh", "-c", cmd, 0 );
		_exit( ENOEXEC );		/* This isn't safe ... */
	}

		/* The parent */
	if( parent_reads ) {
		close( pipe_d[WRITE_END] );
		return( fdopen(pipe_d[READ_END],mode) );
	} else {
		close( pipe_d[READ_END] );
		return( fdopen(pipe_d[WRITE_END],mode) );
	}
}
		
int
my_pclose(fp)
FILE	*fp;
{
	int			status;
	sigset_t	mask;

		/* Close the pipe */
	(void)fclose( fp );

		/* Wait for child process to exit and get its status */
	while( waitpid(ChildPid,&status,0) < 0 ) {
		if( errno != EINTR ) {
			status = -1;
			break;
		}
	}

	/* Note: by the time waitpid() returns we should have already gotten
	one occurrence of SIGCHLD, and the hanlder for that signal should
	already have been restored. */

		/* Now return status from child process */
	return status;
}


/*
  Eat up just one instance of the signal SIGCHLD.  We set the
  SIGCHLD handler back to its original state, so that after catching
  this one occurrence we don't mess with any further occurrences.
*/
static void
sigchld_eater( sig )
int sig;
{
		/* Restore original handler for SIGCHLD */
	if( sigaction(SIGCHLD,&OldAct,0) < 0 ) {
		EXCEPT( "sigaction" );
		exit( 1 );
	}
}
