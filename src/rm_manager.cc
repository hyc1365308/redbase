#include <stdio.h>
#include <math.h>
#include <string.h>
#include "rm.h"
using namespace std;

RM_Manager::RM_Manager(PF_Manager &pfm){
	this->pfm = pfm;
}

RC RM_Manager::CreateFile(const char *fileName,int recordSize){
	RC rc = 0;
	if (fileName == NULL)
		return 1;//null filename
	if (recordSize <= 0 )
		return 2;//wrong recordSize
		
	int pageHeaderSize = sizeof(struct RM_PageHeader);
	int numRecordsPerPage = floor((PF_PAGE_SIZE - pageHeaderSize) / ( recordSize + 1.0/8));
	if (numRecordsPerPage <= 0)
		return 3;//recordSize too large
		
	int bitmapSize = numRecordsPerPage / 8;
	if (bitmapSize * 8 < numRecordsPerPage) bitmapSize ++;//enough place for bitmap
	//bitmapSize + numRecordsPerPage * recordSize + pageHeaderSize <= PAGE_SIZE
		
	//create file
	if (!this->pfm.CreateFile(fileName))
		return 4;//create fail
	
	//init file
	PF_FileHandle fh;
	PF_PageHandle ph;
	if (rc = pfm.OpenFile(fileName,fh))
		return (rc);
	if (rc = fh.AllocatePage(ph))
		return (rc);
	char *pData;
	if (rc = ph.GetData(pData))
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
	if (rc = fh.UnpinPage(0))
		return (rc);
	//write the pageHeader for first page 
	if (rc = fh.AllocatePage(ph))
		return (rc);
	if (rc = ph.GetData(pData))
		return (rc);
	memset(pData, 0, bitmapSize + pageHeaderSize);
	RM_PageHeader *pageHeader = (struct RM_PageHeader*) pData;
	pageHeader->nextFreePage = -1;
	if (rc = fh.UnpinPage(1))
		return (rc);
	
	return (rc);//success
}

RC RM_Manager::DestroyFile(const char *fileName){
	RC rc = 0;
	if (fileName == NULL)
		return 1;//NULL FILENAME

	if ((rc = pfm.DestroyFile(fileName)))
		return (rc);

	return (rc);
}



RC RM_Manager::OpenFile(const char* fileName,RM_FileHandle &fileHandle){
	RC rc = 0;
	PF_FileHandle pf_fh;
	PF_PageHandle ph;
	if (rc = pfm.OpenFile(fileName, pf_fh))
		return (rc);
	//read header
	
	PageNum firstPage = 0;
	if (rc = pf_fh.GetThisPage(firstPage, ph))
		return (rc);
	char* pData;
	if (rc = ph.GetData(pData))
		return (rc);
	RM_FileHeader *fh = (struct RM_FileHeader*) pData;
	
	//save information into fileHandle
	fileHandle.pf_fh  = pf_fh;
	fileHandle.header = *fh;
	fileHandle.headerModified = false;
	fileHandle.openedFH = true;
	


	return (rc);
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle){
	RC rc = 0;
	if (fileHandle.headerModified){//modified
		char* pData;
		PF_PageHandle ph;
		if (rc = fileHandle.pf_fh.GetThisPage(0, ph))
			return (rc);
		if (rc = ph.GetData(pData))
			return (rc);
		memcpy(pData, &(fileHandle.header), sizeof(struct RM_FileHeader));
		fileHandle.pf_fh.MarkDirty(0);
		fileHandle.pf_fh.FlushPages();
	}
	pfm.CloseFile(fileHandle.pf_fh);
	fileHandle.openedFH = false;
	return (rc);
}


