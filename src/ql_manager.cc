
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
        if((rc = SelectOne(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions))){
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
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[0].rhsAttr.attrName);
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
            if (!tempNode->compare(recData)){
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

    //check value type
    for (int i = 0; i < nValues; i++){
        if (values[i].type != (attrEntries + i)->attrType){
            free(attrEntries);
            return QL_WRONGTYPE;
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
    free(attrEntries);

    RID rid;
    RM_FileHandle fh;
    if ((rc = rmm.OpenFile(relName, fh)) ||
        (rc = fh.InsertRec(tempRecord, rid)) ||
        (rc = fh.ForcePages())){
        free(tempRecord);
        return (rc);
    }
    cout<<"RID: PageNum = "<<rid.Page()<<" SlotNum = "<<rid.Slot()<<endl;
    free(tempRecord);

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
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[0].rhsAttr.attrName);
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
            if (!tempNode->compare(recData)){
                compare = false;
                break;
            }
            tempNode = tempNode->nextNode;
        }
        if (!compare) continue;
        printer.Print(cout, recData);
        RID rid;
        if ((rc = rec.GetRid(rid)) ||
            (rc = fh.DeleteRec(rid)))
            return rc;
    }

    printer.PrintFooter(cout);
    if (rc != RM_EOF)
        return rc;

    rc = 0;

    if ((rc = fh.ForcePages()))
        return rc;

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
        string lhsName(conditions[0].lhsAttr.attrName);
        int attrIndex1 = attrToIndex[lhsName];
        AttrCatEntry entry1 = *(attrEntries + attrIndex1);
        int attrOffset1 = entry1.offset;
        int attrLength1 = entry1.attrLength;
        int attrIndex   = entry1.attrNum;
        int recLength   = relEntries->tupleLength;
        if (conditions[i].bRhsIsAttr){
            string rhsName(conditions[0].rhsAttr.attrName);
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
            if (!tempNode->compare(recData)){
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
        if(bIsValue == 1){
            updateData = (char*)(rhsValue.data);
        }else{
            string rhsUpdateName(rhsRelAttr.attrName);
            int rhsIndex = attrToIndex[rhsUpdateName];
            int rhsOffset = attrEntries[rhsIndex].offset;
            updateData = recData + rhsOffset;
        }
        memcpy(recData + updOffset, updateData, updLength);

        if((rc = fh.UpdateRec(rec))){
            return (rc);
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

    return rc;
}



