//
// ix.h
//
// Indexing Component interface

#ifndef IX_H
#define IX_H


#include "redbase.h"
#include "pf.h"
#include "rm_rid.h"
#include <string>
#include <stdlib.h>
#include <cstring>

// Like FileHeader in RM
struct IX_IndexHeader{
	AttrType attrType;
	int attrLength;
	int entryOffset_N;
	int entryOffset_B;
	int keysOffset_N;
	int maxKeys_N;
	int maxKeys_B;
	PageNum rootPage; 
};

//
// IX_Manager : IX File interface
//
class IX_IndexHandle{
	friend class IX_Manager;
	friend class IX_IndexScan;
public:
	IX_IndexHandle	();
	~IX_IndexHandle	();
	static const int BEGINNING_OF_SLOTS = -2;
    static const int END_OF_SLOTS = -3;
	RC InsertEntry	(void *pData, const RID &rid);
	RC DeleteEntry	(void *pData, const RID &rid);

	RC ForcePages	();

private:
	bool isOpenHandle;     
    PF_FileHandle pfh;    
    bool header_modified;  
    PF_PageHandle rootPH;  
    struct IX_IndexHeader header;
    RC CreateNewNode(PF_PageHandle &ph, PageNum &page, char *& data, bool isLeaf);
    RC SplitPage(struct IX_NodeHeader *parentHeader, struct IX_NodeHeader *oldHeader, PageNum oldPage,
				int oldIndex, int &newIndex, PageNum &splitPage);
    RC InsertSpacefulNode(IX_NodeHeader *nodeHeader, PageNum nodePage, void *pData, const RID &rid);
    RC FindInsertPos(struct IX_NodeHeader* nodeHeader, void* pData, int &index, bool& isDup);
    RC FindPrevIndex(struct IX_NodeHeader* nodeHeader, int thisIndex, int &prevIndex);
    RC InsertIntoBucket(PageNum page, const RID &rid);
    RC InsertIntoBucket2(PageNum thisPage, const RID rid);
    RC CheckBucketRID(PageNum thisPage, const RID rid, bool &recur);
    RC CreateNewBucket(PageNum &bucketPage);
    RC DeleteFromNode(IX_NodeHeader *nodeHeader, void *pData, const RID &rid, bool &deleteThis);
    RC DeleteFromLeaf(struct IX_NodeHeader_L *nodeHeader, void *pData, const RID &rid, bool &toDelete);
    RC DeleteFromBucket(struct IX_BucketHeader *bucketHeader, const RID &rid, bool &deleteBucket, PageNum &nextBucketPage);

    bool isValidIndexHeader() const;
    static int CalcNumKeysNode(int attrLength);
    static int CalcNumKeysBucket(int attrLength);
    RC GetFirstLeafPage(PF_PageHandle &leafPH, PageNum &leafPage);
    RC FindRecordPage(PF_PageHandle &leafPH, PageNum &leafPage, void * key);
    int (*comparator) (void * , void *, int);
};

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

private:
    bool openScan;
    bool (*comparator) (void * value1, void * value2, AttrType attrType, int attrLength);
    const IX_IndexHandle* ixh;
    const PF_FileHandle* pfh;

    ClientHint pinHint;

    PF_PageHandle ph;
    PageNum currentPage;
    SlotNum currentSlot;
    void* value;
};

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
};

//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_EOF					(START_IX_WARN + 0) // end of file
#define IX_LASTWARN				IX_EOF

#define IX_NULLFILENAME   		(START_IX_ERR - 0) // null filename pointer
#define IX_INVALIDATTRLENGTH	(START_IX_ERR - 1) // invalid attrLength
#define IX_INVALIDATTRTYPE		(START_IX_ERR - 2) // invalid attrType
#define IX_INVALIDINDEXHANDLE	(START_IX_ERR - 3) // invalid indexHandle
#define IX_INVALIDCOMPOP		(START_IX_ERR - 4) // invalid CompOp
#define IX_INVALIDPINHINT		(START_IX_ERR - 5) // invalid pinHint
#define IX_DUPLICATEENTRY		(START_IX_ERR - 6) // 插入了rid重复的记录
#define IX_INVALIDENTRY			(START_IX_ERR - 7) // 没有找到要删除的记录
#define IX_NULLVALUEPOINTER     (START_IX_ERR - 8) // null value pointer
#define IX_SCANNOTOPENED		(START_IX_ERR - 9) // the scan not opened

#define IX_LASTERROR			IX_INVALIDENTRY

#endif