
#include <stdio.h>
#include "ql.h"

using namespace std;

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

    cout << "   nSelAttrs = " << nSelAttrs << "\n";
    for (int i = 0; i < nSelAttrs; i++)
        cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

    cout << "   nRelations = " << nRelations << "\n";
    for (int i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    //set relEntries and attrEntries
    if(nRelations <= 0 || nRelations >2){
        return (QL_INVALIDSELECT);
    }

    if(nRelations == 1){
        if((rc = SelectOne(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions))){
            return (rc);
        }
    }else if(nRelations == 2){
        if((rc = SelectTwo(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions))){
            return (rc);
        }
    }

    return rc;
}

RC QL_Manager::SelectOne  (int nSelAttrs,          // # attrs in select clause
        const RelAttr selAttrs[],               // attrs in select clause
        int   nRelations,                       // # relations in from clause
        const char * const relations[],         // relations in from clause
        int   nConditions,                      // # conditions in where clause
        const Condition conditions[]) { 

    RC rc = 0;

    RM_Record unusedRec;
    if ((rc = smm.GetRelEntry(relations[0], unusedRec, relEntries))){
        return (rc);
    }

    attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry) * (relEntries->attrCount));
    if ((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
        free(attrEntries);
        return (rc);
    }

    //set attrToIndex, attrToType
    attrToIndex.clear();
    attrToType.clear();
    for (int i = 0; i < relEntries->attrCount; i++){
        string attrString((attrEntries + i)->attrName);
        attrToIndex[attrString] = i;
        attrToType[attrString] = (attrEntries + i)->attrType;
    }

    bool isSelectAll = false;
    if(nSelAttrs == 1){
        if(strncmp("*\0", selAttrs[0].attrName, 2) == 0){
            isSelectAll = true;
        }
    }
    //check selectattrs
    DataAttrInfo *dataAttrs = (DataAttrInfo*)malloc((relEntries->attrCount)*sizeof(DataAttrInfo));
    int printerInit = 0;
    if(isSelectAll){
        for(int i = 0; i < relEntries->attrCount; i++){
            memcpy(dataAttrs[i].relName,  (attrEntries + i)->relName , MAXNAME+1);
            memcpy(dataAttrs[i].attrName, (attrEntries + i)->attrName, MAXNAME+1);
            dataAttrs[i].attrType = (attrEntries + i)->attrType;
            dataAttrs[i].attrLength = (attrEntries + i)->attrLength;
            dataAttrs[i].offset  = (attrEntries + i)->offset;
            dataAttrs[i].indexNo = (attrEntries + i)->indexNo;
        }
        printerInit = relEntries -> attrCount;
    }else{
        for(int i = 0; i < nSelAttrs; i++){
            string selAttrName(selAttrs[i].attrName);
            map<string, int>::iterator exist = attrToIndex.find(selAttrName);
            if(exist == attrToIndex.end())
                return QL_ATTRNAMENOTFOUND;
            int selAttrIndex = attrToIndex[selAttrName];
            memcpy(dataAttrs[i].relName,   (attrEntries + selAttrIndex)->relName, MAXNAME+1);
            memcpy(dataAttrs[i].attrName, (attrEntries + selAttrIndex)->attrName, MAXNAME+1);
            dataAttrs[i].attrType = (attrEntries + selAttrIndex)->attrType;
            dataAttrs[i].attrLength = (attrEntries + selAttrIndex)->attrLength;
            dataAttrs[i].offset  = (attrEntries + selAttrIndex)->offset;
            dataAttrs[i].indexNo = (attrEntries + selAttrIndex)->indexNo;
        }
        printerInit = nSelAttrs;
    }
    Printer printer(dataAttrs, printerInit);


    //check conditions,set types
    types = (AttrType *)malloc(sizeof(AttrType) * (nConditions));
    for (int i = 0; i < nConditions; i++) {
        string lhsName(conditions[i].lhsAttr.attrName);
        map<string, int>::iterator exist = attrToIndex.find(lhsName);
        if (exist == attrToIndex.end())
            return QL_ATTRNAMENOTFOUND;
        memcpy(types + i, &(attrToType[lhsName]), sizeof(AttrType));
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[i].rhsAttr.attrName);
            exist = attrToIndex.find(rhsName);
            if (exist == attrToIndex.end())
                return QL_ATTRNAMENOTFOUND;
            if (attrToType[lhsName] != attrToType[rhsName])
                return QL_WRONGTYPE;
        }
        else{
            if (conditions[i].rhsValue.data == NULL){//comp to NULL

            }
            else if (attrToType[lhsName] != conditions[i].rhsValue.type){
                return QL_WRONGTYPE;
            }
        }
    }

    //build nodes
    QL_NODE* firstNode;
    if (nConditions == 0)
        firstNode = NULL;
    else{
        firstNode = new QL_NODE(conditions[0], *(AttrType *)types);
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[0].bRhsIsAttr){
            string rhsName(conditions[0].rhsAttr.attrName);
            int attrIndex2 = attrToIndex[rhsName];
            AttrCatEntry entry2 = *(attrEntries + attrIndex2);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
        }
        else firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        firstNode->nextNode = NULL;
        firstNode->print();
    }
    QL_NODE* tempNode = firstNode;
    for (int i = 1; i < nConditions; i++) {
        tempNode->nextNode = new QL_NODE(conditions[i], *(AttrType *)(types + i));
        tempNode = tempNode->nextNode;
        string lhsName(conditions[i].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[i].rhsAttr.attrName);
            int attrIndex2 = attrToIndex[rhsName];
            AttrCatEntry entry2 = *(attrEntries + attrIndex2);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
        }
        else tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        tempNode->nextNode = NULL;
        tempNode->print();
    }
    free(types);

    
    printer.PrintHeader(cout);

    RM_FileHandle fh;
    RM_FileScan   fs;
    if ((rc = rmm.OpenFile(relations[0], fh)) ||
        (rc = fs.OpenScan(fh, INT, 4, 0, NO_OP, NULL)))
        return rc;

    RM_Record rec;
    char * recData;

    while((rc = fs.GetNextRec(rec)) == 0){
        bool compare = true;
        if ((rc = rec.GetData(recData)))
            return rc;

        tempNode = firstNode;
        while(tempNode != NULL){
            if (!tempNode->compare(recData, NULL)){
                compare = false;
                break;
            }
            tempNode = tempNode->nextNode;
        }
        if (!compare) continue;
        printer.Print(cout, recData);
    }

    printer.PrintFooter(cout);
    if (rc != RM_EOF)
        return rc;

    rc = 0;

    if ((rc = fh.ForcePages()))
        return rc;

    return (rc);
}

RC QL_Manager::SelectTwo  (int nSelAttrs,          // # attrs in select clause
        const RelAttr selAttrs[],               // attrs in select clause
        int   nRelations,                       // # relations in from clause
        const char * const relations[],         // relations in from clause
        int   nConditions,                      // # conditions in where clause
        const Condition conditions[]) { 

    RC rc = 0;

    RM_Record unusedRec;
    RM_Record unusedRec2;
    if ((rc = smm.GetRelEntry(relations[0], unusedRec, relEntries)) ||
        (rc = smm.GetRelEntry(relations[1], unusedRec2, relEntries2)) ){
        return (rc);
    }
    //printf("name: %s, %s\n", relEntries->relName, relEntries2->relName);

    attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry) * (relEntries->attrCount));
    attrEntries2 = (AttrCatEntry *)malloc(sizeof(AttrCatEntry) * (relEntries->attrCount));
    if ((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel)) ||
        (rc = smm.GetAttrForRel(relEntries2, attrEntries2, attrToRel))){
        free(attrEntries);
        free(attrEntries2);
        return (rc);
    }


    //set attrToIndex, attrToType, relToIndex
    relToIndex.clear();
    string relString(relEntries->relName);
    relToIndex[relString] = 0;
    string relString2(relEntries2->relName);
    relToIndex[relString2] = 1;

    attrToIndex.clear();
    attrToIndex2.clear();
    attrToType.clear();
    attrToType2.clear();
    for (int i = 0; i < relEntries->attrCount; i++){
        string attrString((attrEntries + i)->attrName);
        attrToIndex[attrString] = i;
        attrToType[attrString] = (attrEntries + i)->attrType;
    }
    for (int i = 0; i < relEntries2->attrCount; i++){
        string attrString((attrEntries2 + i)->attrName);
        attrToIndex2[attrString] = i;
        attrToType2[attrString] = (attrEntries2 + i)->attrType;
    }

    bool isSelectAll = false;
    if(nSelAttrs == 1){
        if(strncmp("*", selAttrs[0].attrName, 2) == 0){
            isSelectAll = true;
        }
    }
    //check selectattrs
    DataAttrInfo *dataAttrs = (DataAttrInfo*)malloc((relEntries->attrCount + relEntries2->attrCount)*sizeof(DataAttrInfo));
    int printerInit = 0;
    if(isSelectAll){
        for(int i = 0; i < relEntries->attrCount; i++){
            memcpy(dataAttrs[i].relName,  (attrEntries + i)->relName , MAXNAME+1);
            memcpy(dataAttrs[i].attrName, (attrEntries + i)->attrName, MAXNAME+1);
            dataAttrs[i].attrType   = (attrEntries + i)->attrType;
            dataAttrs[i].attrLength = (attrEntries + i)->attrLength;
            dataAttrs[i].offset     = (attrEntries + i)->offset;
            dataAttrs[i].indexNo    = (attrEntries + i)->indexNo;
        }
        int entryOffset = relEntries->attrCount;
        for(int i = 0; i < relEntries2->attrCount; i++){
            memcpy(dataAttrs[i + entryOffset].relName,  (attrEntries2 + i)->relName , MAXNAME+1);
            memcpy(dataAttrs[i + entryOffset].attrName, (attrEntries2 + i)->attrName, MAXNAME+1);
            dataAttrs[i + entryOffset].attrType   = (attrEntries2 + i)->attrType;
            dataAttrs[i + entryOffset].attrLength = (attrEntries2 + i)->attrLength;
            dataAttrs[i + entryOffset].offset     = (attrEntries2 + i)->offset + relEntries->tupleLength;
            dataAttrs[i + entryOffset].indexNo    = (attrEntries2 + i)->indexNo;
        }
        printerInit = relEntries -> attrCount + relEntries2 -> attrCount;
    }else{
        for(int i = 0; i < nSelAttrs; i++){
            AttrCatEntry* tempAttrEntry;
            int selAttrIndex;
            if ((rc = checkRelAttr(selAttrs[i], tempAttrEntry, selAttrIndex)))
                return rc;
            memcpy(dataAttrs[i].relName,   (tempAttrEntry + selAttrIndex)->relName, MAXNAME+1);
            memcpy(dataAttrs[i].attrName, (tempAttrEntry + selAttrIndex)->attrName, MAXNAME+1);
            dataAttrs[i].attrType = (tempAttrEntry + selAttrIndex)->attrType;
            dataAttrs[i].attrLength = (tempAttrEntry + selAttrIndex)->attrLength;
            if (tempAttrEntry == attrEntries)
                dataAttrs[i].offset  = (tempAttrEntry + selAttrIndex)->offset;
            else
                dataAttrs[i].offset  = (tempAttrEntry + selAttrIndex)->offset + relEntries->tupleLength;
            dataAttrs[i].indexNo = (tempAttrEntry + selAttrIndex)->indexNo;
        }
        printerInit = nSelAttrs;
    }
    Printer printer(dataAttrs, printerInit);


    //check conditions,set types
    types = (AttrType *)malloc(sizeof(AttrType) * (nConditions));
    for (int i = 0; i < nConditions; i++) {
        string lhsName(conditions[i].lhsAttr.attrName);
        map<string, int>::iterator exist = attrToIndex.find(lhsName);
        if (exist == attrToIndex.end()){
            return QL_ATTRNAMENOTFOUND;
        }
        memcpy(types + i, &(attrToType[lhsName]), sizeof(AttrType));
        if (conditions[i].bRhsIsAttr){
            //the rhsAttr can be from every table
            AttrCatEntry* tempAttrEntry;
            int tempAttrIndex;
            if ((rc = checkRelAttr(conditions[i].rhsAttr, tempAttrEntry, tempAttrIndex)))
                return rc;

            if (attrToType[lhsName] != (tempAttrEntry + tempAttrIndex)->attrType)
                return QL_WRONGTYPE;

        }
        else{
            if (conditions[i].rhsValue.data == NULL){//comp to NULL

            }
            else if (attrToType[lhsName] != conditions[i].rhsValue.type){
                return QL_WRONGTYPE;
            }
        }
    }

    //build nodes
    QL_NODE* firstNode;
    if (nConditions == 0)
        firstNode = NULL;
    else{
        AttrCatEntry* tempAttrEntry;
        int tempAttrIndex;
        if ((rc = checkRelAttr(conditions[0].lhsAttr, tempAttrEntry, tempAttrIndex)))
            return rc;

        if (tempAttrEntry != attrEntries)
            return QL_LEFTATTRERROR;

        firstNode = new QL_NODE(conditions[0], *(AttrType *)types);
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[0].bRhsIsAttr){
            //the rhsAttr can be from every table
            if ((rc = checkRelAttr(conditions[0].rhsAttr, tempAttrEntry, tempAttrIndex)))
                return rc;

            AttrCatEntry entry2 = *(tempAttrEntry + tempAttrIndex);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
            if (tempAttrEntry == attrEntries)
                firstNode->setType(3);
        }
        else firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        firstNode->nextNode = NULL;
        firstNode->print();
    }
    QL_NODE* tempNode = firstNode;
    for (int i = 1; i < nConditions; i++) {
        AttrCatEntry* tempAttrEntry;
        int tempAttrIndex;
        if ((rc = checkRelAttr(conditions[i].lhsAttr, tempAttrEntry, tempAttrIndex)))
            return rc;

        if (tempAttrEntry != attrEntries)
            return QL_LEFTATTRERROR;

        tempNode->nextNode = new QL_NODE(conditions[i], *(AttrType *)(types + i));
        tempNode = tempNode->nextNode;
        string lhsName(conditions[i].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            //the rhsAttr can be from every table
            if ((rc = checkRelAttr(conditions[i].rhsAttr, tempAttrEntry, tempAttrIndex)))
                return rc;

            AttrCatEntry entry2 = *(tempAttrEntry + tempAttrIndex);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
            if (tempAttrEntry == attrEntries)
                tempNode->setType(3);
        }
        else tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        tempNode->nextNode = NULL;
        tempNode->print();
    }
    free(types);

    
    printer.PrintHeader(cout);

    RM_FileHandle fh1, fh2;
    RM_FileScan   fs1, fs2;
    if ((rc = rmm.OpenFile(relations[0], fh1)) ||
        (rc = fs1.OpenScan(fh1, INT, 4, 0, NO_OP, NULL)) ||
        (rc = rmm.OpenFile(relations[1], fh2)) ||
        (rc = fs2.OpenScan(fh2, INT, 4, 0, NO_OP, NULL)))
        return rc;

    RM_Record rec1, rec2;
    char *recData1, *recData2, *mixedData;

    mixedData = (char *)malloc(relEntries->tupleLength + relEntries2->tupleLength);
    while((rc = fs1.GetNextRec(rec1)) == 0){
        while((rc = fs2.GetNextRec(rec2)) == 0){
            bool compare = true;
            if ((rc = rec1.GetData(recData1)) ||
                (rc = rec2.GetData(recData2))){
                free(mixedData);
                return rc;
            }

            tempNode = firstNode;
            while(tempNode != NULL){
                if (!tempNode->compare(recData1, recData2)){
                    compare = false;
                    break;
                }
                tempNode = tempNode->nextNode;
            }
            if (!compare) continue;
            memset(mixedData, 0, (relEntries->tupleLength) + (relEntries2->tupleLength));
            memcpy(mixedData, recData1, relEntries->tupleLength);
            memcpy(mixedData + relEntries->tupleLength, recData2, relEntries2->tupleLength);
            printer.Print(cout, mixedData);
        }
        if (rc != RM_EOF){
            free(mixedData);
            return rc;
        }
        if ((rc = fs2.ResetScan()))
            return rc;
    }
    free(mixedData);

    printer.PrintFooter(cout);
    if (rc != RM_EOF)
        return rc;

    rc = 0;

    if ((rc = fh1.ForcePages()) ||
        (rc = fh2.ForcePages()))
        return rc;

    return (rc);
}

RC QL_Manager::Insert  (const char *relName,    // relation to insert into
        int   nValues,                          // # values
        const Value values[]) {                 // values to insert
    RC rc = 0;
    printf("QL_INSERT relName = %s, nValue = %d\n", relName, nValues);

    RM_Record unusedRec;
    if ((rc = smm.GetRelEntry(relName, unusedRec, relEntries))){
        return (rc);
    }
    if ((nValues != relEntries->attrCount)){
        return QL_WRONGVALUENUMBER;
    }

    attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry) * nValues);
    if ((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
        free(attrEntries);
        return (rc);
    }

    int primaryKeyIndex = -1;
    //check value type
    for (int i = 0; i < nValues; i++){
        if (values[i].type != (attrEntries + i)->attrType){
            free(attrEntries);
            return QL_WRONGTYPE;
        }
        if((attrEntries + i)->isPrimaryKey == 1){
            primaryKeyIndex = i;
        }
    }

    if(primaryKeyIndex == -1){
        return (QL_INVALIDRELATION);
    }

    RID insertedRid;
    IX_IndexHandle ih;
    IX_IndexScan is;
    if((rc = ixm.OpenIndex(relName, attrEntries[primaryKeyIndex].indexNo, ih))){
        return (rc);
    }
    if((rc = is.OpenScan(ih, EQ_OP, values[primaryKeyIndex].data))){
        return (rc);
    }
    if((rc = is.GetNextEntry(insertedRid)) == 0){
        return (QL_PRIMARYINSERTED);
    }else{
        if(rc != IX_EOF){
            return (rc);
        }
    }


    //prepare the insert record
    tempRecord = (char *)malloc(relEntries->tupleLength);
    memset(tempRecord, 0, relEntries->tupleLength);
    cout<<"tupleLength="<<relEntries->tupleLength<<endl;
    int offset = 0;
    for (int i = 0; i < nValues; i++){
        cout<<offset<<' '<<attrEntries[i].attrLength<<endl;
        memcpy(tempRecord + offset, values[i].data, attrEntries[i].attrLength);
        offset += attrEntries[i].attrLength;
    }
    

    RID rid;
    RM_FileHandle fh;
    if ((rc = rmm.OpenFile(relName, fh)) ||
        (rc = fh.InsertRec(tempRecord, rid)) ||
        (rc = fh.ForcePages())){
        free(tempRecord);
        return (rc);
    }
    cout<<"RID: PageNum = "<<rid.Page()<<" SlotNum = "<<rid.Slot()<<endl;
    for(int i = 0; i < relEntries->attrCount; i++){
        if(attrEntries[i].indexNo != NO_INDEXES){
            IX_IndexHandle ih;
            if((rc = ixm.OpenIndex(relName, attrEntries[i].indexNo, ih))){
                free(tempRecord);
                free(attrEntries);
                return (rc);
            }
            if((rc = ih.InsertEntry(tempRecord+attrEntries[i].offset, rid)) ||
                (rc = ih.ForcePages())){
                free(tempRecord);
                free(attrEntries);
                return (rc);   
            }
        }
    }
    free(tempRecord);
    free(attrEntries);
    return rc;
}           

RC QL_Manager::Delete  (const char *relName,    // relation to delete from
        int   nConditions,                      // # conditions in where clause
        const Condition conditions[]) {         // conditions in where clause
    RC rc = 0;
    printf("QL_DELETE relName = %s, nConditions = %d\n", relName, nConditions);

    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    //set relEntries and attrEntries
    RM_Record unusedRec;
    if ((rc = smm.GetRelEntry(relName, unusedRec, relEntries))){
        return (rc);
    }

    attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry) * (relEntries->attrCount));
    if ((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
        free(attrEntries);
        return (rc);
    }

    //set attrToIndex, attrToType
    attrToIndex.clear();
    attrToType.clear();
    for (int i = 0; i < relEntries->attrCount; i++){
        string attrString((attrEntries + i)->attrName);
        attrToIndex[attrString] = i;
        attrToType[attrString] = (attrEntries + i)->attrType;
    }

    //check conditions,set types
    types = (AttrType *)malloc(sizeof(AttrType) * (nConditions));
    for (int i = 0; i < nConditions; i++) {
        string lhsName(conditions[i].lhsAttr.attrName);
        map<string, int>::iterator exist = attrToIndex.find(lhsName);
        if (exist == attrToIndex.end())
            return QL_ATTRNAMENOTFOUND;
        memcpy(types + i, &(attrToType[lhsName]), sizeof(AttrType));
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[i].rhsAttr.attrName);
            exist = attrToIndex.find(rhsName);
            if (exist == attrToIndex.end())
                return QL_ATTRNAMENOTFOUND;
            if (attrToType[lhsName] != attrToType[rhsName])
                return QL_WRONGTYPE;
        }
        else{
            if (conditions[i].rhsValue.data == NULL) continue;//comp to NULL
            if (attrToType[lhsName] != conditions[i].rhsValue.type){
                return QL_WRONGTYPE;
            }
        }
    }

    //build nodes
    QL_NODE* firstNode;
    if (nConditions == 0)
        firstNode = NULL;
    else{
        firstNode = new QL_NODE(conditions[0], *(AttrType *)types);
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[0].bRhsIsAttr){
            string rhsName(conditions[0].rhsAttr.attrName);
            int attrIndex2 = attrToIndex[rhsName];
            AttrCatEntry entry2 = *(attrEntries + attrIndex2);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
        }
        else firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        firstNode->nextNode = NULL;
        firstNode->print();
    }
    QL_NODE* tempNode = firstNode;
    for (int i = 1; i < nConditions; i++) {
        tempNode->nextNode = new QL_NODE(conditions[i], *(AttrType *)(types + i));
        tempNode = tempNode->nextNode;
        string lhsName(conditions[i].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[i].rhsAttr.attrName);
            int attrIndex2 = attrToIndex[rhsName];
            AttrCatEntry entry2 = *(attrEntries + attrIndex2);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
        }
        else tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        tempNode->nextNode = NULL;
        tempNode->print();
    }
    free(types);

    DataAttrInfo *dataAttrs = (DataAttrInfo*)malloc((relEntries->attrCount)*sizeof(DataAttrInfo));
    for(int i = 0; i < relEntries->attrCount; i++){
        memcpy(dataAttrs[i].relName,  (attrEntries + i)->relName , MAXNAME+1);
        memcpy(dataAttrs[i].attrName, (attrEntries + i)->attrName, MAXNAME+1);
        dataAttrs[i].attrType = (attrEntries + i)->attrType;
        dataAttrs[i].attrLength = (attrEntries + i)->attrLength;
        dataAttrs[i].offset  = (attrEntries + i)->offset;
        dataAttrs[i].indexNo = (attrEntries + i)->indexNo;
    }
    Printer printer(dataAttrs, relEntries->attrCount);
    printer.PrintHeader(cout);

    RM_FileHandle fh;
    RM_FileScan   fs;
    if ((rc = rmm.OpenFile(relName, fh)) ||
        (rc = fs.OpenScan(fh, INT, 4, 0, NO_OP, NULL)))
        return rc;

    RM_Record rec;
    char * recData;

    while((rc = fs.GetNextRec(rec)) == 0){
        bool compare = true;
        if ((rc = rec.GetData(recData)))
            return rc;

        tempNode = firstNode;
        while(tempNode != NULL){
            if (!tempNode->compare(recData, NULL)){
                compare = false;
                break;
            }
            tempNode = tempNode->nextNode;
        }
        if (!compare) continue;
        printer.Print(cout, recData);
        RID rid;
        if((rc = rec.GetRid(rid))){
            free(attrEntries);
            return (rc);
        }

        for(int i = 0; i < relEntries -> attrCount; i++){
            if(attrEntries[i].indexNo != NO_INDEXES){
                IX_IndexHandle ih;
                if((rc = ixm.OpenIndex(relName, attrEntries[i].indexNo, ih))){
                    free(attrEntries);
                    return (rc);
                }
                if((rc = ih.DeleteEntry(recData + attrEntries[i].offset, rid)) ||
                    (rc = ih.ForcePages())){
                    free(attrEntries);
                    return (rc);   
                }
            }
        }
        if ((rc = fh.DeleteRec(rid)))
            return rc;
    }

    printer.PrintFooter(cout);
    if (rc != RM_EOF)
        return rc;

    rc = 0;

    if ((rc = fh.ForcePages()))
        return rc;

    free(attrEntries);
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
    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    //判断更新是否为null
    bool updateNULL = false;
    //set relEntries and attrEntries
    RM_Record unusedRec;
    if ((rc = smm.GetRelEntry(relName, unusedRec, relEntries))){
        return (rc);
    }

    attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry) * (relEntries->attrCount));
    if ((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
        free(attrEntries);
        return (rc);
    }

    //set attrToIndex, attrToType
    attrToIndex.clear();
    attrToType.clear();
    for (int i = 0; i < relEntries->attrCount; i++){
        string attrString((attrEntries + i)->attrName);
        attrToIndex[attrString] = i;
        attrToType[attrString] = (attrEntries + i)->attrType;
    }

    //check conditions,set types
    types = (AttrType *)malloc(sizeof(AttrType) * (nConditions));
    for (int i = 0; i < nConditions; i++) {
        string lhsName(conditions[i].lhsAttr.attrName);
        map<string, int>::iterator exist = attrToIndex.find(lhsName);
        if (exist == attrToIndex.end())
            return QL_ATTRNAMENOTFOUND;
        memcpy(types + i, &(attrToType[lhsName]), sizeof(AttrType));
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[i].rhsAttr.attrName);
            exist = attrToIndex.find(rhsName);
            if (exist == attrToIndex.end())
                return QL_ATTRNAMENOTFOUND;
            if (attrToType[lhsName] != attrToType[rhsName])
                return QL_WRONGTYPE;
        }
        else{
            if (conditions[i].rhsValue.data == NULL) continue;//comp to NULL
            if (attrToType[lhsName] != conditions[i].rhsValue.type){
                return QL_WRONGTYPE;
            }
        }
    }

    //check Update Type;
    string updName(updAttr.attrName);
    map<string, int>::iterator exist = attrToIndex.find(updName);
    if(exist == attrToIndex.end()){
        return QL_ATTRNAMENOTFOUND;
    }
    if(bIsValue == 0){
        string rhsUpdateName(rhsRelAttr.attrName);
        exist = attrToIndex.find(rhsUpdateName);
        if(exist == attrToIndex.end()){
            return QL_ATTRNAMENOTFOUND;
        }
        if(attrToType[updName] != attrToType[rhsUpdateName])
            return QL_WRONGTYPE;
        if(attrEntries[attrToIndex[updName]].attrLength != attrEntries[attrToIndex[rhsUpdateName]].attrLength){
            return QL_WRONGTYPE;
        }
    }else{
        if(rhsValue.type != attrToType[updName])
            return QL_WRONGTYPE;
        if(rhsValue.data == NULL){
            int updIndex = attrToIndex[updName];
            if((attrEntries + updIndex) -> notNull == 1){
                return QL_NOTNULL;
            }else{
                updateNULL = true;
            }
        }
    }
    //build nodes
    QL_NODE* firstNode;
    if (nConditions == 0)
        firstNode = NULL;
    else{
        firstNode = new QL_NODE(conditions[0], *(AttrType *)types);
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[0].bRhsIsAttr){
            string rhsName(conditions[0].rhsAttr.attrName);
            int attrIndex2 = attrToIndex[rhsName];
            AttrCatEntry entry2 = *(attrEntries + attrIndex2);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
        }
        else firstNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        firstNode->nextNode = NULL;
        firstNode->print();
    }
    QL_NODE* tempNode = firstNode;
    for (int i = 1; i < nConditions; i++) {
        tempNode->nextNode = new QL_NODE(conditions[i], *(AttrType *)(types + i));
        tempNode = tempNode->nextNode;
        string lhsName(conditions[i].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[i].rhsAttr.attrName);
            int attrIndex2 = attrToIndex[rhsName];
            AttrCatEntry entry2 = *(attrEntries + attrIndex2);
            int attrOffset2 = entry2.offset;
            int attrLength2 = entry2.attrLength;
            tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength, attrOffset2, attrLength2);
        }
        else tempNode->setParams(attrOffset1, attrLength1, attrIndex, recLength);
        tempNode->nextNode = NULL;
        tempNode->print();
    }
    free(types);

    DataAttrInfo *dataAttrs = (DataAttrInfo*)malloc((relEntries->attrCount)*sizeof(DataAttrInfo));
    for(int i = 0; i < relEntries->attrCount; i++){
        memcpy(dataAttrs[i].relName,  (attrEntries + i)->relName , MAXNAME+1);
        memcpy(dataAttrs[i].attrName, (attrEntries + i)->attrName, MAXNAME+1);
        dataAttrs[i].attrType = (attrEntries + i)->attrType;
        dataAttrs[i].attrLength = (attrEntries + i)->attrLength;
        dataAttrs[i].offset  = (attrEntries + i)->offset;
        dataAttrs[i].indexNo = (attrEntries + i)->indexNo;
    }
    Printer printer(dataAttrs, relEntries->attrCount);
    printer.PrintHeader(cout);

    RM_FileHandle fh;
    RM_FileScan   fs;
    if ((rc = rmm.OpenFile(relName, fh)) ||
        (rc = fs.OpenScan(fh, INT, 4, 0, NO_OP, NULL)))
        return rc;

    RM_Record rec;
    char * recData;

    while((rc = fs.GetNextRec(rec)) == 0){
        bool compare = true;
        if ((rc = rec.GetData(recData)))
            return rc;

        tempNode = firstNode;
        while(tempNode != NULL){
            if (!tempNode->compare(recData, NULL)){
                compare = false;
                break;
            }
            tempNode = tempNode->nextNode;
        }
        if (!compare) continue;
        printer.Print(cout, recData);
        RID rid;
        if ((rc = rec.GetRid(rid))){
            return (rc);
        }

        int updIndex = attrToIndex[updName];
        int updOffset = attrEntries[updIndex].offset;
        int updLength = attrEntries[updIndex].attrLength;
        char* updateData;
        char nullString[10] = "_null";
        if(bIsValue == 1){
            if(updateNULL){
                updateData = (char*)nullString;
            }else{
                updateData = (char*)(rhsValue.data);
            }
        }else{
            string rhsUpdateName(rhsRelAttr.attrName);
            int rhsIndex = attrToIndex[rhsUpdateName];
            int rhsOffset = attrEntries[rhsIndex].offset;
            updateData = recData + rhsOffset;
        }


        //更新index
        IX_IndexHandle ih;
        IX_IndexScan is;
        if(attrEntries[updIndex].indexNo != NO_INDEXES){
            if((rc = ixm.OpenIndex(relName, attrEntries[updIndex].indexNo, ih))){
                free(attrEntries);
                return (rc);
            }
            if((rc = ih.DeleteEntry(recData + updOffset, rid))){
                free(attrEntries);
                return (rc);
            }
        }

        
        memcpy(recData + updOffset, updateData, updLength);

        if((rc = fh.UpdateRec(rec))){
            free(attrEntries);
            return (rc);
        }

        if(attrEntries[updIndex].indexNo != NO_INDEXES){
            if(attrEntries[updIndex].isPrimaryKey == 1){
                if((rc = is.OpenScan(ih, EQ_OP, recData + updOffset))){
                    free(attrEntries);
                    return (rc);
                }
                RID insertedRid;
                if((rc = is.GetNextEntry(insertedRid)) == 0){
                    free(attrEntries);
                    return (QL_PRIMARYINSERTED);
                }else{
                    if(rc != IX_EOF){
                        free(attrEntries);
                        return (rc);
                    }
                }
            }
            if((rc = ih.InsertEntry(recData + updOffset, rid)) || (rc = ih.ForcePages())){
                free(attrEntries);
                return(rc);
            }
        }

        //以下为调试输出
        RM_Record newRec;
        if((rc = fh.GetRec(rid, newRec))){
            return (rc);
        }
        char *newRecData;
        if((rc = newRec.GetData(newRecData))){
            return (rc);
        }
        printer.Print(cout, newRecData);
    }

    printer.PrintFooter(cout);
    if (rc != RM_EOF)
        return rc;

    rc = 0;

    if ((rc = fh.ForcePages()))
        return rc;

    free(attrEntries);
    return rc;
}


RC QL_Manager::checkRelAttr(const RelAttr relAttr, AttrCatEntry *&attrEntry, int &index){
    RC rc = 0;
    string selAttrName(relAttr.attrName);
    if (attrToRel[selAttrName].size() == 0){
        return QL_ATTRTORELERROR;
    }
    else{
        if (relAttr.relName == NULL){
            //find the default relation
            if (attrToRel[selAttrName].size() > 1) 
                return QL_ATTRNAMECONFLICT;
            int tempIndex = relToIndex[*(attrToRel[selAttrName].begin())];
            if (tempIndex == 0){
                cout<<"1"<<endl;
                attrEntry = attrEntries;
                index = attrToIndex[selAttrName];
            }
            else{
                cout<<"2"<<endl;
                attrEntry = attrEntries2;
                index = attrToIndex2[selAttrName];
            }
        }
        else{
            string relNameString(relAttr.relName);
            set<string>::iterator exist = attrToRel[selAttrName].find(relNameString);
            if(exist == attrToRel[selAttrName].end()){
                return QL_ATTRNAMENOTFOUND;
            }
            else{
                int tempIndex = relToIndex[*(exist)];
                if (tempIndex == 0){
                    cout<<"TempI="<<tempIndex<<endl;
                    attrEntry = attrEntries;
                    index = attrToIndex[selAttrName];
                }
                else{
                    cout<<"TempI="<<tempIndex<<endl;
                    attrEntry = attrEntries2;
                    index = attrToIndex2[selAttrName];
                }
            }
        }
    }
    return rc;
}

