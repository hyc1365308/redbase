#include "rm.h"
#include <math.h>
#include <cstdio>
#include <string.h>
#define NO_MORE_FREE_PAGES -1

RM_FileHandle::RM_FileHandle(){
  // initially, it is not associated with an open file.
  headerModified = false; 
  openedFH = false;
}

RM_FileHandle::~RM_FileHandle(){
  openedFH = false; // disassociate from fileHandle from an open file
}

bool RM_FileHandle::isValidFH() const{
  return openedFH;
}

RC RM_FileHandle::CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const{
  if(bitnum > size)
    return 1;
  int chunk = bitnum / 8;
  int offset = bitnum - chunk*8;
  if ((bitmap[chunk] & (1 << offset)) != 0)
    set = true;
  else
    set = false;
  return (0);
}

RC RM_FileHandle::GetFirstZeroBit(char *bitmap, int size, int &location){
  for(int i = 0; i < size; i++){
    int chunk = i /8;
    int offset = i - chunk*8;
    if ((bitmap[chunk] & (1 << offset)) == 0){
      location = i;
      return (0);
    }
  }
  return 1;
}

RC RM_FileHandle::SetBit(char *bitmap, int size, int bitnum){
  	if (bitnum >= size)
  		return 1;
  	int chunk = bitnum /8;
  	int offset = bitnum - chunk*8;
  	bitmap[chunk] |= 1 << offset;
  	return (0);
}

RC RM_FileHandle::ResetBit(char *bitmap, int size, int bitnum){
	if (bitnum >= size)
    	return 1;
  	int chunk = bitnum / 8;
  	int offset = bitnum - chunk*8;
  	bitmap[chunk] &= ~(1 << offset);
  	return (0);
}

RC RM_FileHandle::ResetBitmap(char *bitmap, int size){
	/**
  	int char_num = NumBitsToCharSize(size);
  	for(int i=0; i < char_num; i++)
    	bitmap[i] = bitmap[i] ^ bitmap[i];*/
	memset(bitmap, 0, size);
  	return (0);
}

int RM_FileHandle::NumBitsToCharSize(int size){
  int bitmapSize = size/8;
  if(bitmapSize * 8 < size) bitmapSize++;
  return bitmapSize;
}

RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const{
	/*if (!isValidFH())
    return (RM_INVALIDFILE);*/

  // Retrieves page and slot number from RID
  RC rc = 0;
  PageNum page;
  SlotNum slot;
  if(rc = rid.isValidRID()){
  	return (rc);
  }
  rid.GetPageNum(page);
  rid.GetSlotNum(slot);
  // Retrieves the appropriate page, and its bitmap and pageheader 
  // contents
  PF_PageHandle ph;
  char *pData;
  if (rc = pf_fh.GetThisPage(page, ph))
    return (rc);
  if (rc = ph.GetData(pData))
    return (rc);
  char *bitmap;
  RM_PageHeader *pageheader;
  pageheader = (struct RM_PageHeader *) pData;
  bitmap = pData + header.bitmapOffset;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = 1;
    goto cleanup_and_exit;
  }

  // Set the record and return it
  if((rc = rec.SetRecord(rid, 
    bitmap + (header.bitmapSize) + slot * (header.recordSize), 
    header.recordSize)))
    goto cleanup_and_exit;

  // always unpin the page before returning
  cleanup_and_exit:
  //bpm->release(index);
  return (rc); 
}

RC RM_FileHandle::UpdateRec (const RM_Record &rec) {
  // only proceed if this filehandle is associated with an open file
  if (!isValidFH())
    return 1;
  RC rc = 0;

  // retrieves the page and slot number of the record
  RID rid;
  if((rc = rec.GetRid(rid)))
    return (rc);
  PageNum page;
  SlotNum slot;
  rid.GetPageNum(page);
  rid.GetSlotNum(slot);

  // gets the page, bitmap and pageheader for this page that holds the record
  PF_PageHandle ph;
  char *pData;
  if (rc = pf_fh.GetThisPage(page, ph))
    return (rc);
  if (rc = ph.GetData(pData))
    return (rc);
  char *bitmap;
  struct RM_PageHeader *pageheader;
  pageheader = (struct RM_PageHeader *) pData;
  bitmap = pData + header.bitmapOffset;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = 1;
    goto cleanup_and_exit;
  }
  
  // updates its contents
  char * recData;
  if((rc = rec.GetData(recData)))
    goto cleanup_and_exit;
  memcpy(bitmap + (header.bitmapSize) + slot*(header.recordSize),
    recData, header.recordSize);
  pf_fh.MarkDirty(page);
  // always unpin the page before returning
  cleanup_and_exit:
  //bpm->release(index);
  return (rc); 
}

RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
  // only proceed if this filehandle is associated with an open file

  if (!isValidFH())
    return 1;

  RC rc = 0;

  if(pData == NULL) // invalid record input
    return 1;
  
  // If no more free pages, allocate it. Otherwise, free get the next
  // page with free slots in it
  PageNum page;
  SlotNum slot;
  // retrieve the page's bitmap and header
  char *bitmap;
  struct RM_PageHeader *pageheader;
  PF_PageHandle ph;
  char *pData_temp;
  if (rc = pf_fh.GetThisPage(header.firstFreePage, ph))
    return (rc);
  if (rc = ph.GetData(pData_temp))
    return (rc);
  pageheader = (struct RM_PageHeader *) pData_temp;
  bitmap = pData_temp + header.bitmapOffset;

  // gets the first free slot on the page and set that bit to mark
  // it as occupied
  page = header.numRecordsPerPage;
  if((rc = GetFirstZeroBit(bitmap, page, slot)))
  	goto cleanup_and_exit;
  if((rc = SetBit(bitmap, page, slot)))
    goto cleanup_and_exit;

  // copy the contents. update the header to reflect that one more
  // record has been added
  memcpy(bitmap + (header.bitmapSize) + slot*(header.recordSize),
    pData, header.recordSize);
  (pageheader->numRecords)++;
  rid = RID(page, slot); // set the RID to return the location of record
  pf_fh.MarkDirty(page);
  // if page is full, update the free-page-list in the file header
  if(pageheader->numRecords == header.numRecordsPerPage){
    header.firstFreePage = pageheader->nextFreePage;
    headerModified = true;
    if(header.firstFreePage=-1){
    	RM_PageHeader *temppageheader;
    	char *tempbitmap;
      if (rc = pf_fh.GetThisPage(header.numPages, ph))
        return (rc);
      if (rc = ph.GetData(pData_temp))
        return (rc);
  		temppageheader = (struct RM_PageHeader *) pData_temp;
  		tempbitmap = pData_temp + header.bitmapOffset;
  		temppageheader->nextFreePage = header.firstFreePage;
  		temppageheader->numRecords = 0;
  		if((rc = ResetBitmap(tempbitmap, header.bitmapSize)))
    		return (rc);
  		//ResetBitmap(tempbitmap,header.bitmapSize);
  		pf_fh.MarkDirty(header.numPages);
  		header.firstFreePage = header.numPages;
  		header.numPages++;
    }
  }

  // always unpin the page before returning
  cleanup_and_exit:
  //bpm->release(index);
    
  return (rc); 
}

RC RM_FileHandle::DeleteRec (const RID &rid) {
  // only proceed if this filehandle is associated with an open file
  if (!isValidFH())
    return 1;
  RC rc = 0;

  // Retrieve page and slot number from the RID
  PageNum page;
  SlotNum slot;
  rid.GetPageNum(page);
  rid.GetSlotNum(slot);

  // Get this page, its bitmap, and its header
  PF_PageHandle ph;
  char *pData;
  if (rc = pf_fh.GetThisPage(page, ph))
    return (rc);
  if (rc = ph.GetData(pData))
    return (rc);
  char *bitmap;
  struct RM_PageHeader *pageheader;
  pageheader = (struct RM_PageHeader*)pData;
  bitmap = pData + header.bitmapOffset;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = 1;
    goto cleanup_and_exit;
  }

  // Reset the bit to indicate a record deletion
  if((rc = ResetBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;
  pageheader->numRecords--;
  pf_fh.MarkDirty(page);
  // update the free list if this page went from full to not full
  if(pageheader->numRecords == header.numRecordsPerPage - 1){
    pageheader->nextFreePage = header.firstFreePage;
    header.firstFreePage = page;
    headerModified=true;
 }

  // always unpin the page before returning
  cleanup_and_exit:

  return (rc); 
}


