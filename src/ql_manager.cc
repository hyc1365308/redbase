
#include <stdio.h>
#include "ql.h"

QL_Manager::QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : smm(smm), ixm(ixm), rmm(rmm){
    printf("Consturctor of ql\n");
}

QL_Manager::~QL_Manager() {
    printf("Destructor of ql\n");
}                     

RC QL_Manager::Select  (int nSelAttrs,          // # attrs in select clause
        const RelAttr selAttrs[],               // attrs in select clause
        int   nRelations,                       // # relations in from clause
        const char * const relations[],         // relations in from clause
        int   nConditions,                      // # conditions in where clause
        const Condition conditions[]) {         // conditions in where clause
    RC rc = 0;
    printf("QL_SELECT nSelAttrs = %d, nRelations = %d, nConditions = %d\n", \
        nSelAttrs, nRelations, nConditions);
    return rc;
}

RC QL_Manager::Insert  (const char *relName,    // relation to insert into
        int   nValues,                          // # values
        const Value values[]) {                 // values to insert
    RC rc = 0;
    printf("QL_INSERT relName = %s, nValue = %d\n", relName, nValues);

    relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
    RM_Record unusedRec;
    if ((rc = smm.GetRelEntry(relName, unusedRec, relEntries))){
        free(relEntries);
        return (rc);
    }
    if ((nValues != relEntries->attrCount)){
        free(relEntries);
        return QL_WRONGVALUENUMBER;
    }

    attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry));
    if ((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
        free(attrEntries);
        free(relEntries);
        return (rc);
    }

    //check value type
    for (int i = 0; i < nValues; i++){
        if (values[i].type != (attrEntries + i)->attrType){
            free(attrEntries);
            free(relEntries);
            return QL_WRONGTYPE;
        }
    }

    //prepare the insert record
    tempRecord = (char *)malloc(relEntries->tupleLength);
    int offset = 0;
    for (int i = 0; i < nValues; i++){
        memcpy(tempRecord + offset, values[i].data, attrEntries[i].attrLength);
        offset += attrEntries[i].attrLength;
    }
    free(attrEntries);
    free(relEntries);

    RID rid;
    RM_FileHandle fh;
    if ((rc = rmm.OpenFile(relName, fh)) ||
        (rc = fh.InsertRec(tempRecord, rid))){
        free(tempRecord);
        return (rc);
    }
    free(tempRecord);

    return rc;
}           

RC QL_Manager::Delete  (const char *relName,    // relation to delete from
        int   nConditions,                      // # conditions in where clause
        const Condition conditions[]) {         // conditions in where clause
    RC rc = 0;
    printf("QL_DELETE relName = %s, nConditions = %d\n", relName, nConditions);
    return rc;
}   

RC QL_Manager::Update  (const char *relName,    // relation to update
        const RelAttr &updAttr,                 // attribute to update
        const int bIsValue,                     // 1 if RHS is a value, 0 if attribute
        const RelAttr &rhsRelAttr,              // attr on RHS to set LHS equal to
        const Value &rhsValue,                  // or value to set attr equal to
        int   nConditions,                      // # conditions in where clause
        const Condition conditions[]) {         // conditions in where clause
    RC rc = 0;
    printf("QL_UPDATE relName = %s, nConditions = %d", relName, nConditions);
    return rc;
}