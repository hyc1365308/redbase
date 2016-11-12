#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ix.h"
using namespace std;

IX_Manager::IX_Manager(PF_Manager &pfm){
	this->pfm = &pfm;
}


//compile index file name with filename and indexNo
//need to be delete
char* indexFileName(char *fileName, int indexNo);

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
	char *newFileName = indexFileName()
		
	//create file
	if ((rc = this->pfm->CreateFile(fileName)))
		return rc;
	
	//init file
	PF_FileHandle fh;
	PF_PageHandle ph;
	char *pData;
	if ((rc = pfm->OpenFile(fileName,fh)) ||
		(rc = fh.AllocatePage(ph)) ||
		(rc = ph.GetData(pData))){
		delete newFileName;
		return (rc);
	}
	delete newFileName;
	//Init FileHeader
	
	//write the pageHeader for first page 
	
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


	indexHandle.pf_fh = new PF_FileHandle();
	PF_PageHandle ph;
	if ((rc = pfm->OpenFile(indexFile, *(fileHandle.pf_fh))))
		return (rc);
	//read header

	//save information into fileHandle

	return (0);
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle){
	RC rc = 0;
	if (fileHandle.headerModified){//modified

		//do something
	}
	//adjust the filename
	pfm->CloseFile(filename);

	//do something
	indexHandle.openedFH = false;
	return (rc);
}


//function to compile the index filename
char* indexFileName(char *fileName, int indexNo){
	char *newFileName = new char[strlen(fileName) + 30];
	memcpy(newFileName, fileName, strlen(fileName));
	memcpy(newFileName + strlen(fileName), "-index-", 7);
	sprintf(newFileName + strlen(fileName) + 7, "%d", indexNo);
	return newFileName;
}
