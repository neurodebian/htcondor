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

 

#if defined(SUNOS41)

#include <stdio.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/file.h>
#include <kvm.h>

#include "debug.h"
#include "except.h"
static char *_FileName_ = __FILE__;		/* Used by EXCEPT (see except.h)     */


/*
 * Get the amount of installed RAM memory in megabytes
 *
*/
int
calc_phys_memory()
{
        unsigned int physmem;
        char *namelist = 0;
        char *corefile = 0;
        kvm_t   *kvm_des;
        struct nlist mynl[2];
 
        mynl[0].n_name = "_physmem";
        mynl[1].n_name = "";
        kvm_des = kvm_open(namelist,corefile, (char *)NULL, O_RDONLY, NULL);
        if (kvm_des == NULL) {
		/* The most likely reason for this error to occur is 
		 * we cannot read /dev/kmem.  We should either be root or
		 * in the group kmem (and permissions on /dev/kmem should be
		 * correct!) */
                return(-1);
        }
 
        if (kvm_nlist(kvm_des, mynl) < 0) {
                return(-1);
        }
 
        if ( mynl[0].n_value == 0 ) {
                return(-1);
        }

        if ( kvm_read(kvm_des, (u_long) mynl[0].n_value, &physmem, sizeof(physmem)) < 0 ) {
                return(-1);
        }
 
        kvm_close(kvm_des);
 
        physmem *= getpagesize();
 
        physmem += 300000;      /* add about 300K to offset for RAM that the
				   Sun PROM program gobbles at boot time (at 
				   least I think that's where its going).  If
				   we fail to do this the integer divide we do
				   next will incorrectly round down one meg. */
 
        physmem /= 1048576;     /* convert from bytes to megabytes */
 
 	return(physmem);
}
#elif defined(HPUX9) || defined(IRIX53) 

#include "condor_uid.h"
#include <stdio.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/file.h>

struct nlist memnl[] = {
	{ "physmem",0,0 },
    { NULL }
};

#if defined(IRIX62)
struct nlist64 memnl64[] = {
	{ "physmem",0,0 },
    { NULL }
};
#endif  /* IRIX62 */

calc_phys_memory()
{
  int kmem;
  int maxmem, physmem;
  priv_state      priv;
  priv = set_root_priv();
  /*
   *   Lookup addresses of variables.
  */
#if defined(HPUX9) && !defined(HPUX10)
  if ((nlist("/hp-ux",memnl) <0) || (memnl[0].n_type ==0)) return(-1); 
#endif
#if defined(HPUX10)
  if ((nlist("/stand/vmunix",memnl) <0) || (memnl[0].n_type ==0)) return(-1);
#endif

/* for IRIX62, we assume that the kernel is a 32 or 64-bit ELF binary, and
 * for IRIX53, we assume the kernel is a 32-bit COFF.  Todd 2/97 */
#if defined(IRIX62)
  if (_libelf_nlist("/unix",memnl) == -1)
     if (_libelf_nlist64("/unix",memnl64) == -1)
         return(-1);
#elif defined(IRIX53)
  if ((nlist("/unix",memnl) <0) || (memnl[0].n_type ==0)) return(-1); 
#endif

  /*
   *   Open kernel memory and read variables.
  */
  if ((kmem=open("/dev/kmem",0)) <0) return (-1);

#if defined(IRIX62)
  /* if IRIX62, figure out if we grabbed a 32- or 64-bit address */
  if ( memnl[0].n_type ) {
  	if (-1==lseek(kmem,(long) memnl[0].n_value,0)) return (-1);
  } else {
  	if (-1==lseek(kmem,(long) memnl64[0].n_value,0)) return (-1);
  }
#else
  if (-1==lseek(kmem,(long) memnl[0].n_value,0)) return (-1);
#endif

  if (read(kmem,(char *) &physmem, sizeof(int)) <0) return (-1);
  
  close(kmem);

  /*
   *  convert to megabytes
  */

#if defined(HPUX9)
  physmem/=256; /* *4 /1024 */
#elif defined(IRIX53)
  physmem = physmem<<2;			/* assumes that a page is 4K */
  physmem /= 1024;				/* convert from kbytes to mbytes */
#endif
  set_priv(priv);
  return(physmem);
}

#elif defined(Solaris) 

/*
 * This works for Solaris >= 2.3
 */
#include <unistd.h>

int
calc_phys_memory()
{
	unsigned long pages, pagesz, hack;
	int factor;

	pages = sysconf(_SC_PHYS_PAGES);
	pagesz = sysconf(_SC_PAGESIZE);

	if (pages == -1 || pagesz == -1)
		return -1;

		/* pagesz is in bytes, this gives us kbytes */
	factor = pagesz >> 10;

#if defined(X86)
		/* 
		   This is super-ugly.  For some reason, Intel Solaris seems
		   to have some kind of rounding error for reporting memory.
		   These values just came from a little trail and error and
		   seem to work pretty well.  -Derek Wright (1/29/98)
 	    */
	hack = (pages * factor);
	if( hack > 260000 ) {
		return (int) (hack / 1023);		
	} else if( hack > 130000 ) {
		return (int) (hack / 1020);
	} else if( hack > 65000 ) {
		return (int) (hack / 1010);
	} else {
		return (int) (hack / 1000);
	}
#else
	return (int) ((pages * factor) / 1024);
#endif
}

#elif defined(LINUX)
#include <stdio.h>

int calc_phys_memory() 
{	

	FILE        *proc;
	unsigned long phys_mem;
	char        tmp_c[20];
	long        tmp_l;
	char   		c;

	proc=fopen("/proc/meminfo","r");
	if(!proc) {
		return -1;
	}

	  /*
	  // The /proc/meminfo looks something like this:

	  //       total:    used:    free:  shared: buffers:  cached:
	  //Mem:  19578880 19374080   204800  7671808  1191936  8253440
	  //Swap: 42831872  8368128 34463744
	  //MemTotal:     19120 kB
	  //MemFree:        200 kB
	  //MemShared:     7492 kB
	  //Buffers:       1164 kB
	  //Cached:        8060 kB
	  //SwapTotal:    41828 kB
	  //SwapFree:     33656 kB
	  */	  
	while((c=fgetc(proc))!='\n');
	fscanf(proc, "%s %ul", tmp_c, &phys_mem);
	fclose(proc);
	return (int)(phys_mem/(1024000));
}

#elif defined(WIN32)

#include "condor_common.h"

int
calc_phys_memory()
{
	MEMORYSTATUS status;
	GlobalMemoryStatus(&status);
	return (int)(status.dwTotalPhys/(1024*1024));
}

#elif defined(OSF1)

#include "condor_common.h"
#include "condor_uid.h"
#include <unistd.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <nlist.h>

struct nlist nl[] = { 
    { "_physmem", },
    { 0, },
};

int calc_phys_memory()
{
    int i;
    int kmem;
    unsigned int physmem;
    priv_state      priv;
    
    priv = set_root_priv();

    i = nlist(_PATH_UNIX, nl);
    if( i == -1 )
    {
	return -1;
    }
    if(( nl[0].n_type == 0 ) || ( nl[0].n_value == 0)) 
    {
	return -1;
    }
    kmem = open(_PATH_KMEM, O_RDONLY, 0);
    if ( kmem == -1 )
    {
	return -1;
    }
    i = lseek(kmem, nl[0].n_value, SEEK_SET);
    if ( i == -1 )
    {
	return -1;
    }
    i = read(kmem, &physmem, sizeof(physmem));
    if ( i == -1 )
    {
	return -1;
    }
    physmem = ctob(physmem);
    set_priv(priv);

    return (int)(physmem / ( 1024 * 1024) );
}

#else	/* Don't know how to do this on other than SunOS and HPUX yet */
int
calc_phys_memory()
{
	return -1;
}
#endif


