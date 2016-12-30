#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ix.h"
#include "comparators.h"
#include "ix_internal.h"
using namespace std;

IX_Manager::IX_Manager(PF_Manager &pfm){
	this->pfm = &pfm;
}

IX_Manager::~IX_Manager(){

}


//compile index file name with filename and indexNo
//need to be delete

//function to compile the index filename
char* indexFileName(const char *fileName, int indexNo){
	char *newFileName = new char[strlen(fileName) + 30];
	memcpy(newFileName, fileName, strlen(fileName));
	memcpy(newFileName + strlen(fileName), "-index-", 7);
	sprintf(newFileName + strlen(fileName) + 7, "%d", indexNo);
	return newFileName;
}

RC IX_Manager::CreateIndex	(const	char 		*fileName,
					 				int 		indexNo,
					 				AttrType 	attrType,
					 				int 		attrLength){
	RC rc = 0;
	if (fileName == NULL)
		return IX_NULLFILENAME;//null filename
	//check attrType and attrLength
	switch (attrType){
		case INT  :	
		case FLOAT:	if (attrLength != 4)
						return IX_INVALIDATTRLENGTH;//Invalid value;
					break;

		case STRING:if ((attrLength < 1) || (attrLength > MAXSTRINGLEN))
						return IX_INVALIDATTRLENGTH;//Invalid value
					break;

		default :	return IX_INVALIDATTRTYPE;//Invalid AttrType
	}

	//compile the Filename
	char *newFileName = indexFileName(fileName, indexNo);
		
	//create file
	if ((rc = this->pfm->CreateFile(newFileName))){
		delete newFileName;
		return rc;
	}
	
	//init file
	PF_FileHandle fh;
	PF_PageHandle ph;
	PageNum headerPage;
	PageNum rootPage;
	char *headerPageData;
	char *rootPageData;
	if ((rc = pfm->OpenFile(newFileName,fh)) ||
		(rc = fh.AllocatePage(ph)) || (rc = ph.GetData(headerPageData)) || (rc = ph.GetPageNum(headerPage)) ||
		(rc = fh.AllocatePage(ph)) || (rc = ph.GetData(rootPageData)) || (rc = ph.GetPageNum(rootPage))){
		delete newFileName;
		return (rc);
	}
	delete newFileName;

	//Init IndexHeader
	IX_IndexHeader *ixHeader = (IX_IndexHeader *) headerPageData;
	ixHeader->attrLength = attrLength;
	ixHeader->attrType = attrType;
    ixHeader->maxKeys_N = IX_IndexHandle::CalcNumKeysNode(attrLength);
    ixHeader->maxKeys_B = IX_IndexHandle::CalcNumKeysBucket(attrLength);
    ixHeader->entryOffset_N = sizeof(IX_NodeHeader_I);
	ixHeader->entryOffset_B = sizeof(IX_BucketHeader);
    ixHeader->keysOffset_N = ixHeader->entryOffset_N + ixHeader->maxKeys_N * sizeof(struct Node_Entry);
	ixHeader->rootPage = rootPage;

	// Set up the root node
	IX_NodeHeader_L *rootHeader = (IX_NodeHeader_L *) rootPageData;
    rootHeader->isLeafNode = true;
    rootHeader->isEmpty = true;
    rootHeader->num_keys = 0;
    rootHeader->nextPage = NO_MORE_PAGES;
    rootHeader->prevPage = NO_MORE_PAGES;
    rootHeader->firstSlotIndex = NO_MORE_SLOTS;
    rootHeader->freeSlotIndex = 0;
    Node_Entry *entries = (struct Node_Entry *) ((char *)rootHeader + ixHeader->entryOffset_N);
    for(int i=0; i < ixHeader->maxKeys_N; i++){
      entries[i].isValid = UNOCCUPIED;
      entries[i].page = NO_MORE_PAGES;
      if(i == (ixHeader->maxKeys_N -1))
        entries[i].nextSlot = NO_MORE_SLOTS;
      else
        entries[i].nextSlot = i+1;
    }

	//write the pageHeader for first page 
	if ((rc = fh.MarkDirty(headerPage)) || (rc = fh.UnpinPage(headerPage))||
		(rc = fh.MarkDirty(rootPage)) || (rc = fh.UnpinPage(rootPage))){
		return (rc);
	}

	fh.FlushPages();
	
	return (0);//success
}

RC IX_Manager::DestroyIndex	(const char *fileName, int indexNo){
	RC rc = 0;
	if (fileName == NULL)
		return IX_NULLFILENAME;//NULL FILENAME

	char *indexFile = indexFileName(fileName, indexNo);
	if ((rc = pfm->DestroyFile(indexFile))){
		delete indexFile;
		return (rc);
	}

	delete indexFile;
	return (rc);
}



RC IX_Manager::OpenIndex	(const char *fileName,
					 		 int 		indexNo,
					 		 IX_IndexHandle &indexHandle){
	RC rc = 0;
	char *indexFile = indexFileName(fileName, indexNo);

	PageNum firstPage = 0;
	PF_PageHandle headerPH;
	if ((rc = pfm->OpenFile(indexFile, indexHandle.pfh)) ||
		(rc = indexHandle.pfh.GetThisPage(firstPage, headerPH))){
		delete indexFile;
		return (IX_INVALIDINDEX);
	}

	delete indexFile;
	//read header
	char *pData;
	if ((rc = headerPH.GetData(pData)))
		return (rc);
	indexHandle.header = *((IX_IndexHeader *)pData);

	if ((rc = indexHandle.pfh.UnpinPage(firstPage)))
		return (rc);
	//init comparator
	switch (indexHandle.header.attrType){
		case INT  :	indexHandle.comparator = compare_int;
					break;
		case FLOAT:	indexHandle.comparator = compare_float;
					break;
		case STRING:indexHandle.comparator = compare_string;
					break;
		default :	return IX_INVALIDATTRTYPE;//Invalid AttrType
	}
	//init rootPH
	if ((rc = indexHandle.pfh.GetThisPage(indexHandle.header.rootPage, indexHandle.rootPH))){
		return (rc);
	}

	indexHandle.isOpenHandle = true;

	return (0);
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle){
	RC rc = 0;
	if (indexHandle.header_modified){//modified
		char* pData;
		PF_PageHandle ph;
		printf("signal11\n");
		if ((rc = indexHandle.pfh.GetThisPage(0, ph)) ||
			(rc = ph.GetData(pData)))
			return (rc);
		memcpy(pData, &(indexHandle.header), sizeof(struct IX_IndexHeader));
		if ((rc = indexHandle.pfh.MarkDirty(0)) ||
			(rc = indexHandle.pfh.UnpinPage(0)))
			return (rc);
		printf("signal11\n");
	}
	if ((rc = indexHandle.pfh.UnpinPage(indexHandle.header.rootPage))){
		return (rc);
	}
	indexHandle.pfh.FlushPages();
	indexHandle.isOpenHandle = false;
	if ((rc = pfm->CloseFile(indexHandle.pfh)))
		return (rc);
	return (rc);
}



