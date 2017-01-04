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
  (char*)"Invalid IndexScan",
  (char*)"reach the end of file",
  (char*)"the record to be deleted not found",
  (char*)"Invalid index"
};

static char *IX_ErrorMsg[] = {
  (char*)"null filename pointer",
  (char*)"invalid attrLength",
  (char*)"invalid attrType",
  (char*)"invalid indexHandle",
  (char*)"invalid CompOp",
  (char*)"invalid pinHint",
  (char*)"insert record with repetitive RID",
  (char*)"null value pointer",
  (char*)"the scan not opened",
};

//
// IX_PrintError
//
// Desc: Send a message corresponding to a IX return code to cerr
//       Assumes IX_UNIX is last valid IX return code
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
