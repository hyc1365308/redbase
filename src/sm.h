//
// sm.h
//   Data Manager Component Interface
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include <stdlib.h>
#include <string.h>
#include "redbase.h"  // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "printer.h"
#include "rm_rid.h"
#include <map>
#include <string>
#include <set>

#define MAX_DB_NAME 255
typedef struct RelCatEntry{
  char relName[MAXNAME + 1];
  int tupleLength;
  int attrCount;
  int indexCount;
  int indexCurrNum;
  int numTuples;
  bool statsInitialized;
} RelCatEntry;

// Define catalog entry for an attribute
typedef struct AttrCatEntry{
  char relName[MAXNAME + 1];
  char attrName[MAXNAME +1];
  int offset;
  AttrType attrType;
  int attrLength;
  int indexNo;
  int attrNum;
  int numDistinct;
  float maxValue;
  float minValue;
  int notNull;
  int isPrimaryKey;
} AttrCatEntry;

class SM_Manager {
    friend class QL_Manager;
    static const int NO_INDEXES = -1;
    static const PageNum INVALID_PAGE = -1;
    static const SlotNum INVALID_SLOT = -1;
public:
    SM_Manager    (IX_Manager &ixm, RM_Manager &rmm);
    ~SM_Manager   ();                             // Destructor

    RC CreateDb   (const char *dbName);           // Create the database
    RC OpenDb     (const char *dbName);           // Open the database
    RC CloseDb    ();                             // close the database
    RC DropDb     (const char *dbName);           // Drop the database

    RC CreateTable(const char *relName,           // create relation relName
                   int        attrCount,          //   number of attributes
                   AttrInfo   *attributes);       //   attribute data
    RC CreateIndex(const char *relName,           // create an index for
                   const char *attrName);         //   relName.attrName
    RC DropTable  (const char *relName);          // destroy a relation

    RC DropIndex  (const char *relName,           // destroy index on
                   const char *attrName);         //   relName.attrName
    RC Load       (const char *relName,           // load relName from
                   const char *fileName);         //   fileName
    RC Help       ();                             // Print relations in db
    RC Help       (const char *relName);          // print schema of relName

    RC Print      (const char *relName);          // print relName contents

    RC Set        (const char *paramName,         // set parameter to
                   const char *value);            //   value

    RC ShowTables ();                             // show all tables in the database
    RC ShowTable  (const char *relName);          // show the information of table relName

private:
  IX_Manager &ixm;
  RM_Manager &rmm;
  RM_FileHandle relcatFH;
  RM_FileHandle attrcatFH;
  bool printIndex; // Whether to print the index or not when
                   // help is called on a specific table
  bool useQO;
  bool calcStats;
  bool printPageStats;
  bool openedDb;

  bool isValidAttrType(AttrInfo attribute);
  RC InsertAttrCat(const char* relName, AttrInfo attr, int offset, int attrNum);
  RC InsertRelCat(const char *relName, int attrCount, int recSize);
  RC GetRelEntry(const char *relName, RM_Record &relRec, RelCatEntry *&entry);
  RC FindAttr(const char *relName, const char *attrName, RM_Record &attrRec, AttrCatEntry *&entry);

};

class SM_AttrIterator{
  friend class SM_Manager;
public:
  SM_AttrIterator    ();
  ~SM_AttrIterator   ();  
  RC OpenIterator(RM_FileHandle &fh, char *relName);
  RC GetNextAttr(RM_Record &attrRec, AttrCatEntry *&entry);
  RC CloseIterator();

private:
  bool validIterator;
  RM_FileScan fs;

};


//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_LASTWARN             START_SM_WARN

#define SM_INVALIDDB            (START_SM_ERR - 0)
#define SM_INVALIDRELNAME       (START_SM_ERR - 1)
#define SM_INVALIDREL           (START_SM_ERR - 2)
#define SM_INVALIDATTR          (START_SM_ERR - 3)
#define SM_ALREADYINDEXED       (START_SM_ERR - 4)
#define SM_LASTERROR            SM_INVALIDATTR



#endif