#include <stdio.h>
#include <math.h>
#include <string.h>
#include "rm.h"
using namespace std;

RC RM_Manager::CreateFile(const char *fileName,int recordSize){
	RC rc;
	if (fileName == NULL)
		return 1;//null filename
	if (recordSize <= 0 )
		return 2;//wrong recordSize
		
	int pageHeaderSize = sizeof(struct RM_PageHeader);
	int numRecordsPerPage = floor((PAGE_SIZE - pageHeaderSize) / ( recordSize + 1.0/8));
	if (numRecordsPerPage <= 0)
		return 3;//recordSize too large
		
	int bitmapSize = numRecordsPerPage / 8;
	if (bitmapSize * 8 < numRecordsPerPage) bitmapSize ++;//enough place for bitmap
	//bitmapSize + numRecordsPerPage * recordSize + pageHeaderSize <= PAGE_SIZE
	
	//需要判断文件是否已经存在 
		
	//create file
	if (!this->fm->createFile(fileName))
		return 4;//create fail
	
	//init file
	int fileID;
	fm->openFile(fileName,fileID);
	char *pData;
	RM_FileHeader* fh = (struct RM_FileHeader*) pData;
	fh->bitmapOffset = pageHeaderSize;
	fh->bitmapSize = bitmapSize;
	fh->firstFreePage = 1;
	fh->numPages = 2;
	fh->numRecordsPerPage = numRecordsPerPage;
	fh->recordSize = recordSize;
	//write fileHeader
	int index;
	BufType b = bpm->allocPage(fileID, 0, index, false);
	memcpy(b, pData, sizeof(struct RM_FileHeader));
	bpm->writeBack(index);
	
	//write the pageHeader for first page 
	b = bpm->allocPage(fileID, 1, index, false);
	memset(b, 0, bitmapSize + pageHeaderSize);
	RM_PageHeader *pageHeader = (struct RM_PageHeader*) b;
	pageHeader->nextFreePage = -1;
	bpm->writeBack(index);
	
	return 0;//success
}

RC RM_Manager::DestroyFile(const char *fileName){
	if (fileName == NULL)
		return 1;//NULL FILENAME
	//删除之前需要做一些检查 
	return remove(fileName);
}



RC RM_Manager::OpenFile(const char* fileName,RM_FileHandle &fileHandle){
	int fileID;
	fm->openFile(fileName, fileID);
	//read header
	
	int index;
	BufType b = bpm->allocPage(fileID, 0, index, true);
	RM_FileHeader *fh = (struct RM_FileHeader*) b;
	
	//save information into fileHandle
	fileHandle.fileID = fileID;
	fileHandle.bpm = this->bpm;
	fileHandle.fm  = this->fm;
	fileHandle.header = *fh;
	fileHandle.headerModified = false;
	fileHandle.openedFH = true;
	
	return 0;
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle){
	if (fileHandle.headerModified){//modified
		int index;
		BufType b = bpm->getPage(fileHandle.fileID, 0, index);
		memcpy(b, &(fileHandle.header), sizeof(struct RM_FileHeader));
		bpm->markDirty(index);
	}
	bpm->close();
	fm->closeFile(fileHandle.fileID);
	fileHandle.openedFH = false;
	return 0;
}


