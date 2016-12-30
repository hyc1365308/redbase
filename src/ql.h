//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H


#include <stdlib.h>
#include <string.h>
#include "redbase.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"

//
// QL_Manager: query language (DML)
//
class QL_Manager {
public:
    QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~QL_Manager();                       // Destructor

    RC Select  (int nSelAttrs,           // # attrs in select clause
        const RelAttr selAttrs[],        // attrs in select clause
        int   nRelations,                // # relations in from clause
        const char * const relations[],  // relations in from clause
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC SelectOne  (int nSelAttrs,           // # attrs in select clause
    const RelAttr selAttrs[],        // attrs in select clause
    int   nRelations,                // # relations in from clause
    const char * const relations[],  // relations in from clause
    int   nConditions,               // # conditions in where clause
    const Condition conditions[]);   // conditions in where clause

    RC SelectTwo  (int nSelAttrs,           // # attrs in select clause
    const RelAttr selAttrs[],        // attrs in select clause
    int   nRelations,                // # relations in from clause
    const char * const relations[],  // relations in from clause
    int   nConditions,               // # conditions in where clause
    const Condition conditions[]);   // conditions in where clause

    RC Insert  (const char *relName,     // relation to insert into
        int   nValues,                   // # values
        const Value values[]);           // values to insert

    RC Delete  (const char *relName,     // relation to delete from
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Update  (const char *relName,     // relation to update
        const RelAttr &updAttr,          // attribute to update
        const int bIsValue,              // 1 if RHS is a value, 0 if attribute
        const RelAttr &rhsRelAttr,       // attr on RHS to set LHS equal to
        const Value &rhsValue,           // or value to set attr equal to
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

private:
    SM_Manager &smm;
    IX_Manager &ixm;
    RM_Manager &rmm;

    //map from each AttrName to a set with Relname
    std::map<std::string, std::set<std::string> > attrToRel;
    std::map<std::string, int> attrToIndex;
    std::map<std::string, int> attrToIndexSecond;
    std::map<std::string, AttrType> attrToType;
    std::map<std::string, AttrType> attrToTypeSecond;

    AttrType *types;
    RelCatEntry *relEntries;
    RelCatEntry *relEntriesSecond;
    AttrCatEntry *attrEntries;
    AttrCatEntry *attrEntriesSecond;

    char *tempRecord;

};

class QL_NODE {
public:
    QL_NODE(const Condition condition, AttrType attrType);

    RC setParams(int attrOffset, int attrLength, int attrIndex, int recLength);
    RC setParams(int attrOffset1, int attrLength1, int attrIndex1, int recLength1,
                 int attrOffset2, int attrLength2);

    bool compare(const char *record);
    bool compare(const char *record1,const char* record2);

    void print();

    QL_NODE *nextNode;

private:
    RelAttr  lhsAttr;
    CompOp   op;
    int      bRhsIsAttr;
    RelAttr  rhsAttr;
    Value    rhsValue;

    int compareType;  //   0:not inited
                      //   1:for one record 
                      //   2:for two record
    int compareToNull;//  -1:error 
                      //   0:not compare to null 
                      //   1:judge if equal to Null 
                      //   2:judge if not equal to Null

    AttrType attrType;

    int attrOffset1 , attrOffset2 ;
    int attrLength1 , attrLength2 ;
    int attrIndex1  ;
    int recLength1  ;

    bool (*comparator)(void * value1, void * value2, AttrType attrtype, int attrLength);
};

class QL_RecordFilter {
public:
    QL_RecordFilter(const Condition conditions[]);

    RC BuildFilter();

private:
    int     opened;
    QL_NODE *firstNode;
};

//
// Print-error function
//
void QL_PrintError(RC rc);

#define QL_WRONGVALUENUMBER     (START_QL_WARN + 0)  //wrong value number
#define QL_WRONGTYPE            (START_QL_WARN + 1)  //wrong type in input record
#define QL_ATTRNAMENOTFOUND     (START_QL_WARN + 2)  //attribute name not found
#define QL_NOTNULL              (START_QL_WARN + 3)
#define QL_INVALIDSELECT        (START_QL_WARN + 4)
#define QL_LASTWARN             QL_NOTNULL

#define QL_NODEBUILDERROR       (START_QL_ERR - 0)   //error in build QL_NODES
#define QL_NODEINITED           (START_QL_ERR - 1)   //QL_NODE already inited
#define QL_NODENOTINITED        (START_QL_ERR - 2)   //QL_NODE not inited
#define QL_INVALIDCOMPOP        (START_QL_ERR - 3)   //invalid compOp
#define QL_INVALIDCONDITIONS    (START_QL_ERR - 4)   //invalid conditions
#define QL_NODEFUNCSELECTERROR  (START_QL_ERR - 5)   //select wrong compare function in QL_NODE
#define QL_NODEERROR            (START_QL_ERR - 6)   //QL_NODE error
#define QL_LASTERROR            QL_NODEERROR

#endif