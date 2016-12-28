
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

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) : ixm(ixm), rmm(rmm){
  printIndex = false;
  useQO = true;
  calcStats = false;
  printPageStats = true;
}

SM_Manager::~SM_Manager   () {
    printf("Destructor of sm\n");
}

RC SM_Manager::CreateDb   (const char *dbName) {
    RC rc = 0;
    printf("SM_CREATEDB dbName = %s\n", dbName);
    return rc;
}

RC SM_Manager::OpenDb     (const char *dbName) {
    RC rc = 0;
    printf("SM_OPENDB dbName = %s", dbName);
    if(strlen(dbName) > MAX_DB_NAME){
        return (SM_INVALIDDB);
    }
    if(chdir(dbName) < 0){
        cerr << "无法切换到" << dbName << endl;
        return (SM_INVALIDDB);
    }
    if((rc = rmm.OpenFile("relcat", relcatFH))){
        return (SM_INVALIDDB);
    }
    if((rc = rmm.OpenFile("attrcat", attrcatFH))){
        return (SM_INVALIDDB);
    }
    openedDb = true;
    return rc;
}

RC SM_Manager::CloseDb    () {
    RC rc = 0;
    printf("SM_CLOSEDB\n");
    if(!openedDb)
        return rc;

    if((rc = rmm.CloseFile(relcatFH))){
        return rc;
    }
    if((rc = rmm.CloseFile(attrcatFH))){
        return rc;
    }
    return rc;
}

RC SM_Manager::DropDb   (const char *dbName) {
    RC rc = 0;
    printf("SM_DROPDB dbName = %s\n", dbName);
    return rc;
}

RC SM_Manager::CreateTable(const char *relName,   // create relation relName
                   int        attrCount,          //   number of attributes
                   AttrInfo   *attributes) {      //   attribute data
    RC rc = 0;
    printf("SM_CREATETABLE relName = %s, attrCount = %d\n", relName, attrCount);
    for (int i = 0; i < attrCount; i++){
        printf("AttrInfo: attrName = %s, attrLength = %d, notNull = %d, isPrimaryKey = %d\n", 
            attributes[i].attrName, attributes[i].attrLength, attributes[i].notNull, attributes[i].isPrimaryKey);
    }
    set<string> relAttrbutes;
    if(attrCount > MAXATTRS || attrCount < 1){
        return SM_INVALIDREL;
    }
    if(strlen(relName) > MAXNAME){ // Check for valid relName size
        return (SM_INVALIDRELNAME);
    }
    int totalRecSize = 0;
    for(int i = 0;i < attrCount; i++){
        if(strlen(attributes[i].attrName) > MAXNAME){
            return (SM_INVALIDATTR);
        }
        if(! isValidAttrType(attributes[i])){
            return (SM_INVALIDATTR);
        }
        string attrString(attributes[i].attrName);
        bool exits = (relAttrbutes.find(attrString) != relAttrbutes.end());
        if(exits){
            return (SM_INVALIDREL);
        }else{
            totalRecSize += attributes[i].attrLength;
            relAttrbutes.insert(attrString);
        }
    }
    //totalRecSize += attrCount*1.0/8.0;
    if((rc = rmm.CreateFile(relName, totalRecSize)))
        return (SM_INVALIDRELNAME);

    RID rid;
    int currentOffset = 0;
    for(int i = 0;i < attrCount; i++){
        AttrInfo attr = attributes[i];
        if((rc = InsertAttrCat(relName, attr, currentOffset, i))){
            return (rc);
        }
        currentOffset += attr.attrLength;
    } 
    if((rc = InsertRelCat(relName, attrCount, totalRecSize))){
        return (rc);
    }
    if((rc = attrcatFH.ForcePages()) || (rc = relcatFH.ForcePages())){
        return rc;
    }
    
    return rc;
}

bool SM_Manager::isValidAttrType(AttrInfo attribute){

    AttrType type = attribute.attrType;
    int length = attribute.attrLength;
    if(type == INT && length == 4)
      return true;
    if(type == FLOAT && length == 4)
      return true;
    if(type == STRING && (length > 0) && length < MAXSTRINGLEN)
      return true;
  
    return false;
}

RC SM_Manager::InsertRelCat(const char *relName, int attrCount, int recSize){
    RC rc = 0;
    RelCatEntry* relEntry = (RelCatEntry*)malloc(sizeof(RelCatEntry));
    memset((void*)relEntry, 0, sizeof(RelCatEntry));
    *relEntry  = (RelCatEntry){"\0",0,0,0,0};
    relEntry->tupleLength = recSize;
    relEntry -> attrCount = attrCount;
    memcpy(relEntry -> relName, relName, MAXNAME+1);
    relEntry -> indexCount = 0;
    relEntry -> indexCurrNum = 0;
    relEntry -> numTuples = 0;
    relEntry -> statsInitialized = false;
    RID relRID;
    if((rc = relcatFH.InsertRec((char*)relEntry, relRID))){
        return rc;
    }
    free(relEntry);
    return rc;
}

RC SM_Manager::InsertAttrCat(const char* relName, AttrInfo attr, int offset, int attrNum){
    RC rc = 0;
    AttrCatEntry* attrEntry = (AttrCatEntry *)malloc(sizeof(AttrCatEntry));
    memset((void*)attrEntry, 0,sizeof(AttrCatEntry));
    *attrEntry = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0};
    memcpy(attrEntry->relName, relName, MAXNAME+1);
    memcpy(attrEntry->attrName, attr.attrName, MAXNAME+1);
    attrEntry -> offset = offset;
    attrEntry -> attrType = attr.attrType;
    attrEntry -> attrLength = attr.attrLength;
    attrEntry -> indexNo = NO_INDEXES;
    attrEntry -> attrNum = attrNum;
    attrEntry -> numDistinct = 0;
    attrEntry -> minValue = FLT_MIN;
    attrEntry -> maxValue = FLT_MAX;
    attrEntry -> notNull = attr.notNull;
    attrEntry -> isPrimaryKey = attr.isPrimaryKey;
    RID attrrid;
    if((rc = attrcatFH.InsertRec((char*)attrEntry,attrrid))){
        return rc;
    }

    free(attrEntry);
    return rc;
}
RC SM_Manager::CreateIndex(const char *relName,   // create an index for
                   const char *attrName) {        //   relName.attrName
    RC rc = 0;
    printf("SM_CREATEINDEX relName = %s, attrName = %s\n", relName, attrName);
    RM_Record relRec;
    RelCatEntry *relEntry;
    if((rc = GetRelEntry(relName, relRec, relEntry))){
        return (rc);
    }
    RM_Record attrRec;
    AttrCatEntry *attrEntry;
    if((rc = FindAttr(relName, attrName, attrRec, attrEntry))){
        return (rc);
    }
    if(attrEntry -> indexNo != NO_INDEXES){
        return (SM_ALREADYINDEXED);
    }
    if((rc = ixm.CreateIndex(relName, relEntry->indexCurrNum, attrEntry -> attrType, attrEntry -> attrLength))){
        return (rc);
    }
    IX_IndexHandle ixIndexHandle;
    RM_FileHandle rmFileHandle;
    RM_FileScan rmFileScan;
    if((rc = ixm.OpenIndex(relName, relEntry -> indexCurrNum, ixIndexHandle))){
        return (rc);
    }
    if((rc = rmm.OpenFile(relName, rmFileHandle))){
        return (rc);
    }

    if((rc = rmFileScan.OpenScan(RM_FileHandle, INT, 4, 0, NO_OP, NULL))){
        return (rc);
    }
    RM_Record rmRec;
    while(rmFileScan.GetNextRec(rmRec) != RM_EOF){
        char *data;
        RID rid;
        if((rc = rmRec.GetData(data))||(rc = rmRec.GetRid(rid))){
            return (rc);
        }
        if((rc = ixIndexHandle.InsertEntry(data + attrEntry -> offset, rid))){
            return (rc);
        }
    }
    if((rc = rmFileScan.CloseScan()) || (rc = rmm.CloseFile(fh)) || (rc = ixm.CloseIndex(ixIndexHandle))){
        return (rc);
    }

    attrEntry -> indexNo = relEntry -> indexCurrNum;
    relEntry -> indexCurrNum++;
    relEntry -> indexCount++;

    if((rc = relcatFH.UpdateRec(relRec)) || (rc = attrcatFH.UpdateRec(attrRec))){
        return (rc);
    }

    if((rc = relcatFH.ForcePages()) || (rc = attrcatFH.ForcePages())){
        return (rc);
    }

    return rc;
}

RC SM_Manager::FindAttr(const char *relName, const char *attrName, RM_Record &attrRec, AttrCatEntry *&entry){
    RC rc = 0;
    return (rc);
}

RC SM_Manager::DropTable  (const char *relName) { // destroy a relation
    RC rc = 0;
    printf("SM_DROPTABLE relName = %s\n", relName);
    if(strlen(relName) > MAXNAME){
        return (SM_INVALIDRELNAME);
    }
    if((rc = rmm.DestroyFile(relName))){
        return (SM_INVALIDRELNAME);
    }

    RM_Record relRec;
    RelCatEntry *relEntry;
    if((rc = GetRelEntry(relName, relRec, relEntry))){
        return (rc);
    }
    int numAttr = relEntry -> attrCount;
    SM_AttrIterator attrIt;
    if((rc = attrIt.OpenIterator(attrcatFH, const_cast<char*>(relName)))){
        return (rc);
    }
    AttrCatEntry *attrEntry;
    RM_Record attrRec;
    for(int i = 0; i < numAttr; i++){
        if((rc = attrIt.GetNextAttr(attrRec, attrEntry))){
            return (rc);
        }
        if(attrEntry->indexNo != NO_INDEXES){
            if((rc = DropIndex(relName, attrEntry -> attrName))){
                return (rc);
            }
        }
        RID attrRID;
        if((rc = attrRec.GetRid(attrRID)) || (rc = attrcatFH.DeleteRec(attrRID))){
            return (rc);
        }
    }
    if((rc = attrIt.CloseIterator())){
        return (rc);
    }
    RID relRID;
    if((rc = relRec.GetRid(relRID))||(rc = relcatFH.DeleteRec(relRID))){
        return (rc);
    }

    if((rc = attrcatFH.ForcePages()) || (rc = relcatFH.ForcePages())){
        return rc;
    }
    
    return rc;
}

RC SM_Manager::GetRelEntry(const char *relName, RM_Record &relRec, RelCatEntry *&entry){
    RC rc = 0;
    RM_FileScan rmFileScan;
    if((rc = rmFileScan.OpenScan(relcatFH, STRING, MAXNAME+1, 0, EQ_OP, const_cast<char*>(relName)))){
        return (rc);
    }
    if((rc = rmFileScan.GetNextRec(relRec))){
        return (SM_INVALIDRELNAME);
    }
    if((rc = rmFileScan.CloseScan())){
        return (rc);
    }
    if((rc = relRec.GetData((char *&)entry))){
        return(rc);
    }
    return rc;
}

RC SM_Manager::DropIndex  (const char *relName,   // destroy index on
                   const char *attrName) {        //   relName.attrName
    RC rc = 0;
    printf("SM_DROPINDEX relName = %s, attrName = %s\n", relName, attrName);
    RM_Record relRec;
    RelCatEntry* relcatEntry;
    if((rc = GetRelEntry(relName, relRec, relcatEntry))){
        return (rc);
    }
    RM_Record attrRec;
    AttrCatEntry* attrcatEntry;
    if((rc = FindAttr(relName, attrName, attrRec, attrcatEntry))){
        return (rc);
    }
    if(attrcatEntry -> indexNo == NO_INDEXES){
        return (SM_NOINDEX);
    }
    if((rc = ixm.DestroyIndex(relName, attrcatEntry -> indexNo))){
        return (rc);
    }
    attrcatEntry -> indexNo = NO_INDEXES;
    relEntry -> indexCount--;

    if((rc = relcatFH.UpdateRec(relRec)) || (rc = attrcatFH.UpdateRec(attrRec))){
        return (rc);
    }
    if((rc = relcatFH.ForcePages()) || (rc = attrcatFH.ForcePages())){
        return (rc);
    }
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

RC SM_Manager::ShowTables(){
    RC rc = 0;
    DataAttrInfo *dataAttrs = (DataAttrInfo*)malloc(4*sizeof(DataAttrInfo));
    RM_FileScan rmFileScan;
    RM_Record rmRec;
    PrepareRelCatPrint(dataAttrs);
    Printer printer(dataAttrs, 4);
    printer.PrintHeader(cout);
    if((rc = rmFileScan.OpenScan(relcatFH, INT, 4, 0, NO_OP, NULL))){
        free(dataAttrs);
        return (rc);
    }
    while(rmFileScan.GetNextRec(rmRec) != RM_EOF){
        char *data;
        if((rc = rmRec.GetData(data))){
            free(dataAttrs);
            return (rc);
        }
        printer.Print(cout, data);
    }
    rmFileScan.CloseScan();
    printer.PrintFooter(cout);
    free(dataAttrs);
    return (rc);
}

RC SM_Manager::ShowTable  (const char *relName){
    RC rc = 0;
    DataAttrInfo *dataAttrs = (DataAttrInfo*)malloc(8*sizeof(DataAttrInfo));
    if((rc = PrepareRelCatPrint(dataAttrs))){
        return (rc);
    }
    RM_FileScan rmFileScan;
    RM_Record rmRec;
    if((rc = rmFileScan.OpenScan(relcatFH, STRING, MAXNAME+1, 0, EQ_OP, const_cast<char*>(relName)))){
        return (rc);
    }
    if(rmFileScan.GetNextRec(rmRec) == RM_EOF){
        rmFileScan.CloseScan();
        free(dataAttrs);
        return (SM_INVALIDRELNAME);
    }
    if((rc = rmFileScan.CloseScan())){
        free(dataAttrs);
        return (rc);
    }
    Printer printer(dataAttrs, 8);
    printer.PrintHeader(cout);
    if((rc = rmFileScan.OpenScan(attrcatFH, STRING, MAXNAME + 1, 0, EQ_OP, const_cast<char*>(relName)))){
        return (rc);
    }

    while(rmFileScan.GetNextRec(rmRec) != RM_EOF){
        char *data;
        if((rc = rmRec.GetData(data))){
            rmFileScan.CloseScan();
            free(dataAttrs);
            return (rc);
        }
        printer.Print(cout, data);
    }

    if((rc = rmFileScan.CloseScan())){
        free(dataAttrs);
        return (rc);
    }

    printer.PrintFooter(cout);
    return (rc);
}

RC SM_Manager::PrepareRelCatPrint(DataAttrInfo *dataAttrs){
    RC rc = 0;
    for(int i = 0; i < 4; i++){
        memcpy(dataAttrs[i].relName, "relcat", MAXNAME+1);
        dataAttrs[i].indexNo = 0;
    }
    memcpy(dataAttrs[0].attrName, "relName", MAXNAME+1);
    memcpy(dataAttrs[1].attrName, "tupleLength", MAXNAME+1);
    memcpy(dataAttrs[2].attrName, "attrCount", MAXNAME +1);
    memcpy(dataAttrs[3].attrName, "indexCount", MAXNAME+1);

    dataAttrs[0].attrType = STRING;
    dataAttrs[1].attrType = INT;
    dataAttrs[2].attrType = INT;
    dataAttrs[3].attrType = INT;

    dataAttrs[0].attrLength = MAXNAME + 1;
    dataAttrs[1].attrLength = 4;
    dataAttrs[2].attrLength = 4;
    dataAttrs[3].attrLength = 4;   

    dataAttrs[0].offset  = (int)offsetof(RelCatEntry,relName);
    dataAttrs[1].offset  = (int)offsetof(RelCatEntry,tupleLength);
    dataAttrs[2].offset  = (int)offsetof(RelCatEntry,attrCount);
    dataAttrs[3].offset  = (int)offsetof(RelCatEntry,indexCount);

    return (rc);
}

RC SM_Manager::PrepareAttrCatPrint(DataAttrInfo *dataAttrs){
    RC rc = 0;
    for(int i = 0; i < 8; i++){
        memcpy(dataAttrs[i].relName, "attrcat", MAXNAME+1);
        dataAttrs[i].indexNo = 0;
    }
    memcpy(dataAttrs[0].attrName, "relName", MAXNAME+1);
    memcpy(dataAttrs[1].attrName, "attrName", MAXNAME+1);
    memcpy(dataAttrs[2].attrName, "offset", MAXNAME +1);
    memcpy(dataAttrs[3].attrName, "attrType", MAXNAME+1);
    memcpy(dataAttrs[4].attrName, "attrLength", MAXNAME+1);
    memcpy(dataAttrs[5].attrName, "indexNo", MAXNAME+1);
    memcpy(dataAttrs[6].attrName, "notNull", MAXNAME +1);
    memcpy(dataAttrs[7].attrName, "isPrimaryKey", MAXNAME+1);

    dataAttrs[0].attrType = STRING;
    dataAttrs[1].attrType = STRING;
    dataAttrs[2].attrType = INT;
    dataAttrs[3].attrType = INT;
    dataAttrs[4].attrType = INT;
    dataAttrs[5].attrType = INT;
    dataAttrs[6].attrType = INT;
    dataAttrs[7].attrType = INT;

    dataAttrs[0].attrLength = MAXNAME + 1;
    dataAttrs[1].attrLength = MAXNAME + 1;
    dataAttrs[2].attrLength = 4;
    dataAttrs[3].attrLength = 4;
    dataAttrs[4].attrLength = 4;
    dataAttrs[5].attrLength = 4;  
    dataAttrs[6].attrLength = 4;
    dataAttrs[7].attrLength = 4;     

    dataAttrs[0].offset  = (int)offsetof(AttrCatEntry,relName);
    dataAttrs[1].offset  = (int)offsetof(AttrCatEntry,attrName);
    dataAttrs[2].offset  = (int)offsetof(AttrCatEntry,offset);
    dataAttrs[3].offset  = (int)offsetof(AttrCatEntry,attrType);
    dataAttrs[4].offset  = (int)offsetof(AttrCatEntry,attrLength);
    dataAttrs[5].offset  = (int)offsetof(AttrCatEntry,indexNo);
    dataAttrs[6].offset  = (int)offsetof(AttrCatEntry,notNull);
    dataAttrs[7].offset  = (int)offsetof(AttrCatEntry,isPrimaryKey);

    return (rc);
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


