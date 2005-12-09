#ifndef INCLUDE_SUBMIT_JOB_H
#define INCLUDE_SUBMIT_JOB_H

#include "MyString.h"

namespace classad { class ClassAd; }
class ClassAd;

enum ClaimJobResult { 
	CJR_OK,       // Job claimed (put in managed state)
	CJR_BUSY,     // The job was not IDLE or is claimed by another manager
	CJR_ERROR     // Something went wrong, probably failure to connect to the schedd.
};

/*
	Assuming an active qmgr connection to the schedd holding the job,
	attempt to the put cluster.proc into the managed state (claiming it
	for our own management).  Before doing so, does a last minute check
	to confirm the job is IDLE.  error_details is optional. If non NULL,
	error_details will contain a message explaining why claim_job didn't
	return OK.  The message will be suitable for reporting to a log file.

	my_identity is an optional string.  If non-NULL, it will be inserted
	into the job's classad as 1. a useful debugging identifier and 
	2. a double check that we're the current owner.  The identity is an
	arbitrary string, but should as uniquely as possible identify
	this process.  "$(SUBSYS) /path/to/condor_config" is recommended.
*/
ClaimJobResult claim_job(int cluster, int proc, MyString * error_details = 0,
	const char * my_identity = 0);

/*
	As the above claim_job, but will attempt to create the qmgr connection
	itself.  pool_name can be null to indicate "whatever COLLECTOR_HOST"
	is set to.  schedd_name can be null to indicate "local schedd".

	Attempt to the put cluster.proc into the managed state (claiming it
	for our own management).  Before doing so, does a last minute check
	to confirm the job is IDLE.  error_details is optional. If non NULL,
	error_details will contain a message explaining why claim_job didn't
	return OK.  The message will be suitable for reporting to a log file.
*/
ClaimJobResult claim_job(const char * pool_name, const char * schedd_name, int cluster, int proc, MyString * error_details = 0, const char * my_identity = 0);


/*
	The opposite of claim_job.  Assuming the job is claimed (managed)
	yield it.

	done - true to indiciate the job is all done, the schedd should
	       take no more efforts to manage it (short of evaluating
		   LeaveJobInQueue)
           false to indicate that the job is unfinished, the schedd
		   is free to do what it likes, including probably trying to run it.
	my_identity - If non null, will be checked against the identity registered
		as having claimed the job.  If a different identity claimed the
		job, the yield attempt fails.  See claim_job for details
		on suggested identity strings.
*/
bool yield_job(bool done, int cluster, int proc, MyString * error_details = 0, const char * my_identity = 0, bool *keep_trying = 0);
bool yield_job(const char * pool_name, const char * schedd_name,
	bool done, int cluster, int proc, MyString * error_details = 0, const char * my_identity = 0, bool *keep_trying = 0);

/* 
- src - The classad to submit.  The ad will be modified based on the needs of
  the submission.  This will include adding QDate, ClusterId and ProcId as well as putting the job on hold (for spooling)

- schedd_name - Name of the schedd to send the job to, as specified in the Name
  attribute of the schedd's classad.  Can be NULL to indicate "local schedd"

- pool_name - hostname (and possible port) of collector where schedd_name can
  be found.  Can be NULL to indicate "whatever COLLECTOR_HOST in my
  condor_config file claims".

*/
bool submit_job( ClassAd & src, const char * schedd_name, const char * pool_name, int * cluster_out = 0, int * proc_out = 0 );


/* 
- src - The classad to submit.  Will _not_ be modified.

- schedd_name - Name of the schedd to send the job to, as specified in the Name
  attribute of the schedd's classad.  Can be NULL to indicate "local schedd"

- pool_name - hostname (and possible port) of collector where schedd_name can
  be found.  Can be NULL to indicate "whatever COLLECTOR_HOST in my
  condor_config file claims".

*/
bool submit_job( classad::ClassAd & src, const char * schedd_name, const char * pool_name, int * cluster_out = 0, int * proc_out = 0 );


/*
	Push the dirty attributes in src into the queue.  Does _not_ clear
	the dirty attributes.
	Assumes the existance of an open qmgr connection (via ConnectQ).

	Likely usage:
	// original_vanilla_ad is the ad passed to VanillaToGrid
	//   (it can safely a more recent version pulled from the queue)
	// new_grid_ad is the resulting ad from VanillaToGrid, with any updates 
	//   (indeed, it should probably be the most recent version from the queue)
	original_vanilla_ad.ClearAllDirtyFlags();
	update_job_status(original_vanilla_ad, new_grid_ad);
	push_dirty_attributes(original_vanilla_ad);
*/
bool push_dirty_attributes(classad::ClassAd & src);

/*
	Push the dirty attributes in src into the queue.  Does _not_ clear
	the dirty attributes. 
	Establishes (and tears down) a qmgr connection.
	schedd_name and pool_name can be NULL to indicate "local".
*/
bool push_dirty_attributes(classad::ClassAd & src, const char * schedd_name, const char * pool_name);

/*

Pull a submit_job() job's results out of the sandbox and place them back where
they came from.  If successful, let the job leave the queue.

cluster.proc - ID of the grid (transformed) job.
schedd_name and pool_name can be NULL to indicate "local".

*/
bool finalize_job(int cluster, int proc, const char * schedd_name, const char * pool_name);

/*

Remove a job from the schedd.

 */
bool remove_job(int cluster, int proc, char const *reason, const char * schedd_name, const char * pool_name, MyString &error_desc);


#endif /* INCLUDE_SUBMIT_JOB_H*/
