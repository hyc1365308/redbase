
#include <stdio.h>
#include "sm.h"

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
    for (int i = 0; i < attrCount; i++){
        printf("AttrInfo: attrName = %s, attrLength = %d, notNull = %d\n", attributes[i].attrName, attributes[i].attrLength, attributes[i].notNull);
    }
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
