#!/usr/bin/env perl
##**************************************************************
##
## Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
## University of Wisconsin-Madison, WI.
## 
## Licensed under the Apache License, Version 2.0 (the "License"); you
## may not use this file except in compliance with the License.  You may
## obtain a copy of the License at
## 
##    http://www.apache.org/licenses/LICENSE-2.0
## 
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
##**************************************************************

######################################################################
# script to drive Condor build steps (user tasks)
######################################################################
use strict;
use warnings;

use Getopt::Long;
use vars qw/ $opt_coverity_analysis /;
GetOptions(
            'coverity-analysis' => \$opt_coverity_analysis,
);

my $EXTERNALS_TASK        = "remote_task.externals";
my $BUILD_TASK            = "remote_task.build";
my $TAR_TASK              = "remote_task.create_tar";
my $CHECK_TAR_TASK        = "remote_task.check_tar";
my $UNSTRIPPED_TASK       = "remote_task.create_unstripped_tar";
my $CHECK_UNSTRIPPED_TASK = "remote_task.check_unstripped_tar";
my $NATIVE_DEBUG_TASK     = "remote_task.create_native_unstripped";
my $NATIVE_TASK           = "remote_task.create_native";
my $CHECK_NATIVE_TASK     = "remote_task.check_native";
my $BUILD_TESTS_TASK      = "remote_task.build_tests";
my $RUN_UNIT_TESTS        = "remote_task.run_unit_tests";
my $COVERITY_ANALYSIS     = "remote_task.coverity_analysis";

# autoflush our STDOUT
$| = 1;

my $tasklist_file = "tasklist.nmi";

######################################################################
# Detecting platform and generating task list
######################################################################
print "Generating tasklist...\n";

open(TASKLIST, '>', $tasklist_file) or die "Can't open $tasklist_file for writing: $!\n";

if ($ENV{NMI_PLATFORM} =~ /_win/i) {
    #Windows
    print TASKLIST "$EXTERNALS_TASK 4h\n";
    print TASKLIST "$BUILD_TASK 4h\n";
    print TASKLIST "$BUILD_TESTS_TASK 4h\n";
    print TASKLIST "$NATIVE_TASK 4h\n";
    print TASKLIST "$TAR_TASK 4h\n";
    print TASKLIST "$RUN_UNIT_TESTS 4h\n";
}    
else {
    # Everything else
    print TASKLIST "$EXTERNALS_TASK 4h\n";
    print TASKLIST "$BUILD_TASK 4h\n";
    print TASKLIST "$BUILD_TESTS_TASK 4h\n";
    print TASKLIST "$UNSTRIPPED_TASK 4h\n";
    print TASKLIST "$CHECK_UNSTRIPPED_TASK 4h\n";
    if (!($ENV{NMI_PLATFORM} =~ /(x86_RedHat6|x86_64_RedHat6|x86_64_RedHat7)/)) {
        print TASKLIST "$NATIVE_DEBUG_TASK 4h\n";
    }
    print TASKLIST "$NATIVE_TASK 4h\n";
    print TASKLIST "$CHECK_NATIVE_TASK 4h\n";
    print TASKLIST "$TAR_TASK 4h\n";
    print TASKLIST "$CHECK_TAR_TASK 4h\n";
    print TASKLIST "$RUN_UNIT_TESTS 4h\n";
}

if (defined($opt_coverity_analysis)) {
    print TASKLIST "$COVERITY_ANALYSIS 4h\n";
}

close (TASKLIST);

######################################################################
# Print content of tasklist
######################################################################
print "Generated tasklist:\n";
open(TASKFILE, '<', $tasklist_file) or die "Can't open $tasklist_file for reading: $!\n";
while(<TASKFILE>) {
    print "\t$_";
}
close (TASKFILE);
exit 0;
