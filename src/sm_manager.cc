
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "redbase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#undef max
#include <vector>
#include <string>
#include <set>
#include "stddef.h"
#include "statistics.h"
#include <cfloat>

using namespace std;
extern StatisticsMgr *pStatisticsMgr;
extern void PF_Statistics();

bool recInsert_int(char *location, string value, int length){
  int num;
  istringstream ss(value);
  ss >> num;
  if(ss.fail())
    return false;
  //printf("num: %d \n", num);
  memcpy(location, (char*)&num, length);
  return true;
}

bool recInsert_float(char *location, string value, int length){
    float num;
    istringstream ss(value);
    ss >> num;
    if(ss.fail()){
        return false;
    }
    memcpy(location, (char*)&num, length);
    return true;
}

bool recInsert_string(char* location, string value, int length){
  if(value.length() >= length){
    memcpy(location, value.c_str(), length);
    return true;
  }
  memcpy(location, value.c_str(), value.length()+1);
  return true;
}

SM_Manager::SM_Manager    (IX_Manager &ixm, RM_Manager &rmm) {
    printf("Constructor of sm\n");
}

SM_Manager::~SM_Manager   () {
    printf("Destructor of sm\n");
}

RC SM_Manager::OpenDb     (const char *dbName) {
    RC rc = 0;
    printf("SM_OPENDB dbName = %s", dbName);
    return rc;
}

RC SM_Manager::CloseDb    () {
    RC rc = 0;
    printf("SM_CLOSEDB\n");
    return rc;
}

RC SM_Manager::CreateTable(const char *relName,   // create relation relName
                   int        attrCount,          //   number of attributes
                   AttrInfo   *attributes) {      //   attribute data
    RC rc = 0;
    printf("SM_CREATETABLE relName = %s, attrCount = %d\n", relName, attrCount);
    return rc;
}

RC SM_Manager::CreateIndex(const char *relName,   // create an index for
                   const char *attrName) {        //   relName.attrName
    RC rc = 0;
    printf("SM_CREATEINDEX relName = %s, attrName = %s\n", relName, attrName);
    return rc;
}
RC SM_Manager::DropTable  (const char *relName) { // destroy a relation
    RC rc = 0;
    printf("SM_DROPTABLE relName = %s\n", relName);
    return rc;
}

RC SM_Manager::DropIndex  (const char *relName,   // destroy index on
                   const char *attrName) {        //   relName.attrName
    RC rc = 0;
    printf("SM_DROPINDEX relName = %s, attrName = %s\n", relName, attrName);
    return rc;
}
RC SM_Manager::Load       (const char *relName,   // load relName from
                   const char *fileName) {        //   fileName
    RC rc = 0;
    printf("SM_LOAD relName = %s, fileName = %s\n", relName, fileName);
    return rc;
}
RC SM_Manager::Help       () {                    // Print relations in db
    RC rc = 0;
    printf("SM_HELP relName = null\n");
    return rc;
}                             
RC SM_Manager::Help       (const char *relName) { // print schema of relName
    RC rc = 0;
    printf("SM_HELP relName = %s\n", relName);
    return rc;
}

RC SM_Manager::Print      (const char *relName) { // print relName contents
    RC rc = 0;
    printf("SM_PRINT relName = %s\n", relName);
    return rc;
}

RC SM_Manager::Set        (const char *paramName, // set parameter to
                   const char *value) {           //   value
    RC rc = 0;
    printf("SM_SET paramName = %s, value = %s\n", paramName, value);
    return rc;
}
