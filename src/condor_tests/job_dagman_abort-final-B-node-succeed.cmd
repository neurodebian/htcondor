executable           	= /bin/echo
arguments		= job_dagman_abort-final-B-node.cmd $(nodename) OK
universe             	= scheduler
notification         	= NEVER
getenv               	= true
output			= job_dagman_abort-final-B-node.$(nodename).out
error			= job_dagman_abort-final-B-node.err
queue

