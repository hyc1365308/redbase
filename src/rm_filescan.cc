#include "rm.h"
#include "redbase.h" 
#include "comparators2.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

RM_FileScan::RM_FileScan(){
	this->openScan = false;
	this->fh = NULL;
	this->value = NULL;
}

RM_FileScan::~RM_FileScan(){
	if (this->value != NULL)
		free(this->value);
}



RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,  // Initialize file scan
                     AttrType      attrType,
                     int           attrLength,
                     int           attrOffset,
                     CompOp        compOp,
                     void          *value,
                     ClientHint    pinHint){
	//check if the input is Invalid
	//check fileHandle
	if (!fileHandle.isValidFH())
		return RM_INVALIDFILEHANDLE;
	//check attrType & attrLength
	switch (attrType){
		case INT  :	
		case FLOAT:	if (attrLength != 4)
						return RM_INVALIDATTRLENGTH;//Invalid value;
					break;

		case STRING:if ((attrLength < 1) || (attrLength > MAXSTRINGLEN))
						return RM_INVALIDATTRLENGTH;//Invalid value
					break;

		default :	return RM_INVALIDATTRTYPE;//Invalid AttrType
	}
	//check attrOffset
	int maxValidRecordSize = attrLength + attrOffset;
	if (maxValidRecordSize > fileHandle.header->recordSize)
		return RM_INVALIDFILEHEADER;//Invalid attrLength and Offset

	//check compOp
	switch (compOp){
		case NO_OP : this->comparator = &default_comp; break;
		case EQ_OP : this->comparator = &equal; break;
		case NE_OP : this->comparator = &not_equal; break;
		case LT_OP : this->comparator = &less_than; break;
		case GT_OP : this->comparator = &greater_than; break;
		case LE_OP : this->comparator = &less_than_or_eq_to; break;
		case GE_OP : this->comparator = &greater_than_or_eq_to; break;
		default:	 return RM_INVALIDCOMPOP;//Invalid AttrType
	}
	//check value
	if (value == NULL){
		if (compOp != NO_OP) return RM_NULLRECPOINTER;
	}else{
		this->value = (void *) malloc(attrLength);
		memcpy(this->value, value, attrLength);
	}

	//check pinHint
	switch (pinHint){
		case NO_HINT : this->pinHint = pinHint;break;
		default		 : return RM_INVALIDPINHINT;//Invalid ClientHint
	}

	this->attrType = attrType;
	this->attrOffset = attrOffset;
	this->attrLength = attrLength;
	if ( !fileHandle.isValidFileHeader() ){
		return RM_INVALIDFILEHEADER;//Invalid fileheader
	}
	this->fh = &fileHandle;



	//init scan params
	openScan = true;
	currentPage = 1;
	currentSlot = -1;
	return 0;
}

RC RM_FileScan::GetNextRec(RM_Record &rec){
	RC rc = 0;
	//test if the scan already opened
	if (!openScan) 
		return (RM_SCANNOTOPENED);
	//first get the record
	//ignore ClientHint
	RM_Record temprec;
	char* pData;
	while(true){
		if ((rc = fh->GetNextRecord(currentPage, currentSlot, temprec)))
			return (rc);
		if ((rc = temprec.GetData(pData)))
			return (rc);
		if (comparator(pData + this->attrOffset, this->value,
						 this->attrType, this->attrLength)){
			rec = temprec;
			return 0;
		}

	}
}

RC RM_FileScan::CloseScan(){
	openScan = false;
	return 0;
}