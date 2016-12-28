//
// File:        sm_error.cc
// Description: SM_PrintError functions
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "sm.h"

using namespace std;

//
// Error table
//
static char *SM_WarnMsg[] = {
  (char*)"cannot close db"
};

static char *SM_ErrorMsg[] = {
  (char*)"Invalid database",
  (char*)"Invalid relation name",
  (char*)"Invalid relation",
  (char*)"Already indexed"
};

//
// SM_PrintError
//
// Desc: Send a message corresponding to a SM return code to cerr
//       Assumes PF_UNIX is last valid SM return code
// In:   rc - return code for which a message is desired
//
void SM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
    // Print warning
    cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_SM_ERR && -rc < -SM_LASTERROR)
    // Print error
    cerr << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";
  else if (rc == 0)
    cerr << "SM_PrintError called with return code of 0\n";
  else
    cerr << "SM error: " << rc << " is out of bounds\n";
}
