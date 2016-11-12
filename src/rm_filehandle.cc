#include "rm.h"
#include <math.h>
#include <cstdio>
#include <string.h>
#define NO_MORE_FREE_PAGES -1

RM_FileHandle::RM_FileHandle(){
  // initially, it is not associated with an open file.
  headerModified = false; 
  openedFH = false;
  pf_fh = NULL;
  header = NULL;
}

RM_FileHandle::~RM_FileHandle(){
  delete pf_fh; // disassociate from fileHandle from an open file
  delete header;
}

bool RM_FileHandle::isValidFH() const{
  return openedFH;
}

RC RM_FileHandle::CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const{
  if(bitnum > size)
    return RM_BITNUMBOUND;
  int chunk = bitnum / 8;
  int offset = bitnum - chunk*8;
  if ((bitmap[chunk] & (1 << offset)) != 0)
    set = true;
  else
    set = false;
  return (0);
}

RC RM_FileHandle::GetFirstZeroBit(char *bitmap, int size, int &location) const{
  for(int i = 0; i < size; i++){
    int chunk = i /8;
    int offset = i - chunk*8;
    if ((bitmap[chunk] & (1 << offset)) == 0){
      location = i;
      return (0);
    }
  }
  return (RM_BITMAPEOF);
}

RC RM_FileHandle::GetNextOneBit(char *bitmap, int size, int bitnum, int &nextbit) const{
  if (bitnum >= size)
    return RM_BITNUMBOUND;
  for(int i = bitnum + 1; i < size; i++){
    int chunk = i /8;
    int offset = i - chunk*8;
    if ((bitmap[chunk] & (1 << offset)) != 0){
      nextbit = i;
      return (0);
    }
  }
  return (RM_BITMAPEOF);//Bitmap_EOF
}

RC RM_FileHandle::SetBit(char *bitmap, int size, int bitnum){
  if (bitnum >= size)
  	return RM_BITNUMBOUND;
  int chunk = bitnum /8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] |= 1 << offset;
  return (0);
}

RC RM_FileHandle::ResetBit(char *bitmap, int size, int bitnum){
	if (bitnum >= size)
  	return RM_BITNUMBOUND;
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
  if((rc = rid.isValidRID())){
  	return (rc);
  }
  rid.GetPageNum(page);
  rid.GetSlotNum(slot);
  // Retrieves the appropriate page, and its bitmap and pageheader 
  // contents
  PF_PageHandle ph;
  char *pData;
  if ((rc = pf_fh->GetThisPage(page, ph)) ||
      (rc = ph.GetData(pData)))
    return (rc);
  char *bitmap;
  bitmap = pData + header->bitmapOffset;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header->numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = RM_RECORDNOTEXISTED;
    goto cleanup_and_exit;
  }

  // Set the record and return it
  if((rc = rec.SetRecord(rid, 
    bitmap + (header->bitmapSize) + slot * (header->recordSize), 
    header->recordSize)))
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
  if ((rc = pf_fh->GetThisPage(page, ph)) ||
      (rc = ph.GetData(pData)))
    return (rc);
  char *bitmap;
  bitmap = pData + header->bitmapOffset;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header->numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = RM_RECORDNOTEXISTED;
    goto cleanup_and_exit;
  }
  
  // updates its contents
  char * recData;
  if((rc = rec.GetData(recData)))
    goto cleanup_and_exit;
  memcpy(bitmap + (header->bitmapSize) + slot*(header->recordSize),
    recData, header->recordSize);
  if ((rc = pf_fh->MarkDirty(page)) ||
      (rc = pf_fh->UnpinPage(page)))
    return (rc);
  // always unpin the page before returning
  cleanup_and_exit:
  //bpm->release(index);
  return (rc); 
}

RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
  // only proceed if this filehandle is associated with an open file

  if (!isValidFH())
    return RM_INVALIDFILEHEADER;

  RC rc = 0;

  if(pData == NULL) // invalid record input
    return RM_NULLRECPOINTER;
  
  // If no more free pages, allocate it. Otherwise, free get the next
  // page with free slots in it
  PageNum page; //save the page Num
  SlotNum slot; //save the slot Num
  page = header->firstFreePage;
  // retrieve the page's bitmap and header
  char *bitmap;
  struct RM_PageHeader *pageheader;
  PF_PageHandle ph;
  char *pData_temp;
  if ((rc = pf_fh->GetThisPage(header->firstFreePage, ph)) ||
      (rc = ph.GetData(pData_temp)))
    return (rc);
  pageheader = (struct RM_PageHeader *) pData_temp;
  bitmap = pData_temp + header->bitmapOffset;

  // gets the first free slot on the page and set that bit to mark
  // it as occupied
  if ((rc = GetFirstZeroBit(bitmap, header->numRecordsPerPage, slot)) ||
      (rc = SetBit(bitmap, header->numRecordsPerPage, slot)))
    goto cleanup_and_exit;

  // copy the contents. update the header to reflect that one more
  // record has been added
  memcpy(bitmap + (header->bitmapSize) + slot*(header->recordSize),
    pData, header->recordSize);
  (pageheader->numRecords)++;
  rid = RID(page, slot); // set the RID to return the location of record
  if ((rc = pf_fh->MarkDirty(page)) ||
      (rc = pf_fh->UnpinPage(page)))
    return (rc);
  // if page is full, update the free-page-list in the file header
  if(pageheader->numRecords == header->numRecordsPerPage){
    header->firstFreePage = pageheader->nextFreePage;
    headerModified = true;
    if(header->firstFreePage == -1){
    	RM_PageHeader *temppageheader;
    	char *tempbitmap;
      if ((rc = pf_fh->AllocatePage(ph)) ||
          (rc = ph.GetData(pData_temp)))
        return (rc);
  		temppageheader = (struct RM_PageHeader *) pData_temp;
  		tempbitmap = pData_temp + header->bitmapOffset;
  		temppageheader->nextFreePage = header->firstFreePage;
  		temppageheader->numRecords = 0;
  		if((rc = ResetBitmap(tempbitmap, header->bitmapSize)))
    		return (rc);
  		//ResetBitmap(tempbitmap,header->bitmapSize);
  		if ((rc = pf_fh->MarkDirty(header->numPages)) ||
          (rc = pf_fh->UnpinPage(header->numPages)))
        return (rc);
  		header->firstFreePage = header->numPages;
  		header->numPages++;
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
  if ((rc = pf_fh->GetThisPage(page, ph)) ||
      (rc = ph.GetData(pData)))
    return (rc);
  char *bitmap;
  struct RM_PageHeader *pageheader;
  pageheader = (struct RM_PageHeader*)pData;
  bitmap = pData + header->bitmapOffset;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header->numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = RM_RECORDNOTEXISTED;
    goto cleanup_and_exit;
  }

  // Reset the bit to indicate a record deletion
  if((rc = ResetBit(bitmap, header->numRecordsPerPage, slot)))
    goto cleanup_and_exit;
  pageheader->numRecords--;
  if ((rc = pf_fh->MarkDirty(page)) ||
      (rc = pf_fh->UnpinPage(page)))
    return (rc);
  // update the free list if this page went from full to not full
  if(pageheader->numRecords == header->numRecordsPerPage - 1){
    pageheader->nextFreePage = header->firstFreePage;
    header->firstFreePage = page;
    headerModified=true;
 }

  // always unpin the page before returning
  cleanup_and_exit:

  return (rc); 
}

bool RM_FileHandle::isValidFileHeader() const{
  if (!isValidFH())
    return false;

  if ((header->recordSize <= 0) || 
      (header->numRecordsPerPage <= 0) ||
      (header->numPages <= 0) ||
      (header->bitmapOffset <= 0) ||
      (header->bitmapSize <= 0))
    return false;

  if (header->recordSize * header->numRecordsPerPage + 
    header->bitmapSize + sizeof(RM_PageHeader) > PF_PAGE_SIZE)
    return false;

  return true;
}

//find the first record after (Page, Slot) = (currentPage, currentSlot)
RC RM_FileHandle::GetNextRecord(PageNum &currentPage, SlotNum &currentSlot, RM_Record &temprec) const{
  RC rc = 0;
  int maxPageNum = header->numPages;
  while(true){
    //find the next slot in this page
    //if fail, goto the next page
    //if the currentPage = maxPageNum return RM_EOF
    if (currentPage == maxPageNum)
      return RM_EOF;
    //First read the page
    PF_PageHandle ph;
    char *pData;
    if ((rc = pf_fh->GetThisPage(currentPage, ph)) ||
        (rc = ph.GetData(pData)))
      return (rc);
    char *bitmap;
    bitmap = pData + header->bitmapOffset;

    //if the record exist in this page
    int nextRecord;
    if ( (rc = GetNextOneBit(bitmap, header->numRecordsPerPage, 
            currentSlot, nextRecord)) ){
      //cannot find record in this page
      currentPage ++;
      currentSlot = -1;
      continue;
    }

    //for find the record in this page
    RID temprid = RID(currentPage, nextRecord); 
    currentSlot = nextRecord;
    if ((rc = GetRec(temprid, temprec)))
      return rc;

    return 0;
  }
}

RC RM_FileHandle::ForcePages(PageNum pageNum){
  RC rc;
  if ((rc = pf_fh->ForcePages(pageNum)))
    return rc;
  return 0;
}