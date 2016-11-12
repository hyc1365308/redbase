//
// ix.h
//
// Indexing Component interface

#ifndef IX_H
#define IX_H


#include "redbase.h"
#include "pf.h"
#include <string>
#include <stdlib.h>
#include <cstring>

// Like FileHeader in RM
struct IX_IndexHeader{
	//add component
}

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

	RC OpenIndex	(const char *fileName,
					 int 		indexNo,
					 IX_IndexHandle &indexHandle);

	RC CloseIndex	(IX_IndexHandle &indexHandle);

private:
	PF_Manager* pfm;
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

private:
	PF_FileHandle *pf_fh;
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

//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_EOF					(START_IX_WARN + 0) // end of file
#define IX_LASTWARN				0

#define IX_NULLFILENAME   		(START_IX_ERR - 0) // null filename pointer
#define IX_INVALIDATTRLENGTH	(START_IX_ERR - 1) // invalid attrLength
#define IX_INVALIDATTRTYPE		(START_IX_ERR - 2) // invalid attrType
#define IX_LASTERROR			0

#endif