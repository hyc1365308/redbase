#include "rm.h"
#include "redbase.h" 

RM_FileScan::RM_FileScan(){
	this->openScan = false;
}

void RM_FileScan::readFileHeader(const RM_FileHandle &fileHandle){
	RM_FileHeader fh = filehandle.
}

RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,  // Initialize file scan
                     AttrType      attrType,
                     int           attrLength,
                     int           attrOffset,
                     CompOp        compOp,
                     void          *value,
                     ClientHint    pinHint){
	
}
