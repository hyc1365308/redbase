//
// File:        ix_error.cc
// Description: IX_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ix.h"

using namespace std;

//
// Error table
//
static char *IX_WarnMsg[] = {
  (char*)"reach the end of file"
};

static char *IX_ErrorMsg[] = {
  (char*)"null filename pointer",
  (char*)"invalid attrLength",
  (char*)"invalid attrType"
};

//
// PF_PrintError
//
// Desc: Send a message corresponding to a PF return code to cerr
//       Assumes PF_UNIX is last valid PF return code
// In:   rc - return code for which a message is desired
//
void IX_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_IX_WARN && rc <= IX_LASTWARN)
    // Print warning
    cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_IX_ERR && -rc < -IX_LASTERROR)
    // Print error
    cerr << "IX error: " << IX_ErrorMsg[-rc + START_IX_ERR] << "\n";
  else if (rc == 0)
    cerr << "IX_PrintError called with return code of 0\n";
  else
    cerr << "IX error: " << rc << " is out of bounds\n";
}
