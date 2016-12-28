#include <stdio.h>
#include <math.h>
#include <string.h>
#include "rm.h"
using namespace std;

RM_Manager::RM_Manager(PF_Manager &pfm){
	this->pfm = &pfm;
}

RC RM_Manager::CreateFile(const char *fileName,float recordSize){
	RC rc = 0;
	if (fileName == NULL)
		return RM_NULLFILENAME;//null filename
	if (recordSize <= 0 )
		return RM_INVALIDRECORDSIZE;//wrong recordSize
		
	int pageHeaderSize = sizeof(struct RM_PageHeader);
	int numRecordsPerPage = floor((PF_PAGE_SIZE - pageHeaderSize) / ( recordSize + 1.0/8));
	if (numRecordsPerPage <= 0)
		return RM_INVALIDRECORDSIZE;//recordSize too large
		
	int bitmapSize = numRecordsPerPage / 8;
	if (bitmapSize * 8 < numRecordsPerPage) bitmapSize ++;//enough place for bitmap
	//bitmapSize + numRecordsPerPage * recordSize + pageHeaderSize <= PAGE_SIZE
		
	//create file
	if ((rc = this->pfm->CreateFile(fileName)))
		return rc;//create fail
	
	//init file
	PF_FileHandle fh;
	PF_PageHandle ph;
	char *pData;
	if ((rc = pfm->OpenFile(fileName,fh)) ||
		(rc = fh.AllocatePage(ph)) ||
		(rc = ph.GetData(pData)))
		return (rc);
	//Init FileHeader
	memset(pData, 0, sizeof(struct RM_FileHeader));
	RM_FileHeader* rm_fh = (struct RM_FileHeader*) pData;
	rm_fh->bitmapOffset = pageHeaderSize;
	rm_fh->bitmapSize = bitmapSize;
	rm_fh->firstFreePage = 1;
	rm_fh->numPages = 2;
	rm_fh->numRecordsPerPage = numRecordsPerPage;
	rm_fh->recordSize = recordSize;
	if ((rc = fh.MarkDirty(0)) ||
		(rc = fh.UnpinPage(0)))
		return (rc);
	//write the pageHeader for first page 
	if ((rc = fh.AllocatePage(ph)) ||
		(rc = ph.GetData(pData)))
		return (rc);
	memset(pData, 0, bitmapSize + pageHeaderSize);
	RM_PageHeader *pageHeader = (struct RM_PageHeader*) pData;
	pageHeader->nextFreePage = -1;
	if ((rc = fh.MarkDirty(1)) ||
		(rc = fh.UnpinPage(1)))
		return (rc);
	fh.FlushPages();
	
	return (0);//success
}

RC RM_Manager::DestroyFile(const char *fileName){
	RC rc = 0;
	if (fileName == NULL)
		return RM_NULLFILENAME;//NULL FILENAME

	if ((rc = pfm->DestroyFile(fileName)))
		return (rc);

	return (rc);
}



RC RM_Manager::OpenFile(const char* fileName,RM_FileHandle &fileHandle){
	RC rc = 0;
	fileHandle.pf_fh = new PF_FileHandle();
	PF_PageHandle ph;
	if ((rc = pfm->OpenFile(fileName, *(fileHandle.pf_fh))))
		return (rc);
	//read header
	
	PageNum firstPage = 0;
	if ((rc = fileHandle.pf_fh->GetThisPage(firstPage, ph)))
		return (rc);
	char* pData;
	if ((rc = ph.GetData(pData)))
		return (rc);
	RM_FileHeader *fh = (struct RM_FileHeader*) pData;

	//save information into fileHandle
	fileHandle.header = new RM_FileHeader(fh);
	fileHandle.headerModified = false;
	fileHandle.openedFH = true;
	return (0);
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle){
	RC rc = 0;
	if (fileHandle.headerModified){//modified
		char* pData;
		PF_PageHandle ph;
		if ((rc = fileHandle.pf_fh->GetThisPage(0, ph)) ||
			(rc = ph.GetData(pData)))
			return (rc);
		memcpy(pData, &(fileHandle.header), sizeof(struct RM_FileHeader));
		if ((rc = fileHandle.pf_fh->MarkDirty(0)) ||
			(rc = fileHandle.pf_fh->UnpinPage(0)))
			return (rc);
		fileHandle.pf_fh->FlushPages();
	}
	pfm->CloseFile(*(fileHandle.pf_fh));
	fileHandle.openedFH = false;
	return (rc);
}


