//
// ix.h
//
// Indexing Component interface

#ifndef IX_H
#define IX_H


#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"
#include <string>
#include <stdlib.h>
#include <cstring>

//
// IX_Manager : provides IX file management
//
class IX_Manager{
public:
	IX_Manager 		(PF_Manager &pfm);
	~IX_Manager		();
	RC CreateIndex	(const char *fileName,
					 int 		indexNo,
					 AttrType 	attrType,
					 int 		attrLength);

	RC DestroyIndex	(const char *fileName,
					 int 		indexNo);

	RC OpenIndex	(const char *fioeName,
					 int 		indexNo,
					 IX_IndexHandle &indexHandle);

	RC CloseIndex	(IX_IndexHandle &indexHandle);
}

//
// IX_Manager : IX File interface
//
class IX_IndexHandle{
public:
	IX_IndexHandle	();
	~IX_IndexHandle	();

	RC InsertEntry	(void *pData, const RID &rid);
	RC DeleteEntry	(void *pData, const RID &rid);

	RC ForcePages	();
}

//
// IX_IndexScan : condition-based scan of indexs in the file
//
class IX_IndexScan{
public:
	IX_IndexScan	();
	~IX_IndexScan 	();
	RC OpenScan		(const IX_IndexHandle &indexHandle,
					 CompOp		compOp,
					 void		*value,
					 ClientHint	pinHint = NO_HINT);

	RC GetNextEntry (RID &rid);
	RC CloseScan 	();
}

#endif