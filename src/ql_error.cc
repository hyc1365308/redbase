//
// File:        ql_error.cc
// Description: QL_PrintError functions
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ql.h"

using namespace std;

//
// Error table
//
static char *QL_WarnMsg[] = {
  (char*)"wrong value number",
  (char*)"wrong type in input record",
  (char*)"attribute name not found"
};

static char *QL_ErrorMsg[] = {
  (char*)"error in build QL_NODES",
  (char*)"QL_NODE already inited",
  (char*)"QL_NODE not inited",
  (char*)"invalid compOp",
  (char*)"invalid conditions",
  (char*)"select wrong compare function in QL_NODE",
  (char*)"QL_NODE error"
};

//
// QL_PrintError
//
// Desc: Send a message corresponding to a QL return code to cerr
//       Assumes PF_UNIX is last valid QL return code
// In:   rc - return code for which a message is desired
//
void QL_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_QL_WARN && rc <= QL_LASTWARN)
    // Print warning
    cerr << "QL warning: " << QL_WarnMsg[rc - START_QL_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_QL_ERR && -rc < -QL_LASTERROR)
    // Print error
    cerr << "QL error: " << QL_ErrorMsg[-rc + START_QL_ERR] << "\n";
  else if (rc == 0)
    cerr << "QL_PrintError called with return code of 0\n";
  else
    cerr << "QL error: " << rc << " is out of bounds\n";
}
