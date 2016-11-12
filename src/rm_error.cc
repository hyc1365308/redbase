//
// File:        rm_error.cc
// Description: RM_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
  (char*)"reach the end of file",
  (char*)"reach the end of the bitmap"
};

static char *RM_ErrorMsg[] = {
  (char*)"invalid fileheader",
  (char*)"invalid RID",
  (char*)"invalid recordsize",
  (char*)"invalid fileHandle",
  (char*)"invalid CompOp",
  (char*)"invalid pinHint",
  (char*)"invalid attrLength",
  (char*)"invalid AttrType",
  (char*)"cannot get the recorddata",
  (char*)"record's pointer is null",
  (char*)"filename is null",
  (char*)"the slot already has a record",
  (char*)"the slot not have a record",
  (char*)"the bitnum too big( > size)"
};

//
// PF_PrintError
//
// Desc: Send a message corresponding to a PF return code to cerr
//       Assumes PF_UNIX is last valid PF return code
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
    // Print warning
    cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_RM_ERR && -rc < -RM_LASTERROR)
    // Print error
    cerr << "RM error: " << RM_ErrorMsg[-rc + START_RM_ERR] << "\n";
  else if (rc == 0)
    cerr << "RM_PrintError called with return code of 0\n";
  else
    cerr << "RM error: " << rc << " is out of bounds\n";
}
