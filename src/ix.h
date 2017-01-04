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
    RC GetFirstLeafPage(PF_PageHandle &leafPH, PageNum &leafPage) const;
    RC FindRecordPage(PF_PageHandle &leafPH, PageNum &leafPage, void * key);
    //RC FindNodeInsertIndex(struct IX_NodeHeader *nHeader, 
      //  void* pData, int& index, bool& isDup);
    int (*comparator) (void * , void *, int);

};

//
// IX_IndexScan : condition-based scan of indexs in the file
//
class IX_IndexScan{
	static const char UNOCCUPIED = 'u';
	static const char OCCUPIED_NEW = 'n';
	static const char OCCUPIED_DUP = 'r';
public:
	IX_IndexScan	();
	~IX_IndexScan 	();
	RC OpenScan (const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint pinHint = NO_HINT);
	RC GetNextEntry (RID &rid);
	RC CloseScan 	();

private:
    RC BeginScan(PF_PageHandle &leafPH, PageNum &pageNum);
    RC GetFirstEntryInLeaf(PF_PageHandle &leafPH);
    RC GetFirstBucketEntry(PageNum nextBucket, PF_PageHandle &bucketPH);
    RC GetAppropriateEntryInLeaf(PF_PageHandle &leafPH);
    RC FindNextValue();
    RC SetRID(bool setCurrent);
    bool (*comparator) (void *, void *, AttrType, int);
    IX_IndexHandle* indexHandle;
    const PF_FileHandle* pfh;

    ClientHint pinHint;

    PF_PageHandle ph;
    PF_PageHandle currLeafPH;
    PF_PageHandle currBucketPH;

    bool openScan;
    void *value;
    bool initializedValue;
    bool hasBucketPinned;
    bool hasLeafPinned;
    bool scanEnded;
    bool scanStarted;
    bool endOfIndexReached;
    int attrLength;
    AttrType attrType;
    CompOp compOp;
    bool foundFirstValue;
    bool foundLastValue;
    bool useFirstLeaf;
    PageNum currLeafNum;
    PageNum currBucketNum;
    PageNum nextBucketNum;
    RID currRID;
    RID nextRID;
    struct IX_NodeHeader_L *leafHeader;
    struct IX_BucketHeader *bucketHeader;
    struct Node_Entry *leafEntries;
    struct Bucket_Entry *bucketEntries;
    char *leafKeys;
    char *currKey;
    char *nextKey;
    char *nextNextKey;
    int leafSlot;
    int bucketSlot;
    PageNum currentPage;
    SlotNum currentSlot;

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

#define IX_INVALIDSCAN          (START_IX_WARN + 0) // invalic scan
#define IX_EOF					(START_IX_WARN + 1) // end of file
#define IX_INVALIDENTRY         (START_IX_WARN + 2) // the record to be insert not found
#define IX_INVALIDINDEX         (START_IX_WARN + 3) // invalid index
#define IX_LASTWARN				IX_INVALIDINDEX

#define IX_NULLFILENAME   		(START_IX_ERR - 0) // null filename pointer
#define IX_INVALIDATTRLENGTH	(START_IX_ERR - 1) // invalid attrLength
#define IX_INVALIDATTRTYPE		(START_IX_ERR - 2) // invalid attrType
#define IX_INVALIDINDEXHANDLE	(START_IX_ERR - 3) // invalid indexHandle
#define IX_INVALIDCOMPOP		(START_IX_ERR - 4) // invalid CompOp
#define IX_INVALIDPINHINT		(START_IX_ERR - 5) // invalid pinHint
#define IX_DUPLICATEENTRY		(START_IX_ERR - 6) // insert record with repetitive RID
#define IX_NULLVALUEPOINTER     (START_IX_ERR - 7) // null value pointer
#define IX_SCANNOTOPENED		(START_IX_ERR - 8) // the scan not opened


#define IX_LASTERROR			IX_SCANNOTOPENED

#endif