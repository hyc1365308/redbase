#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <cstdio>

bool iequal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 == *(float*)value2);
    case INT: return (*(int *)value1 == *(int *)value2) ;
    default:
      return (strncmp((char *) value1, (char *) value2, attrLength) == 0); 
  }
}

bool iless_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 < *(float*)value2);
    case INT: return (*(int *)value1 < *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) < 0);
  }
}

bool igreater_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 > *(float*)value2);
    case INT: return (*(int *)value1 > *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}

bool iless_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 <= *(float*)value2);
    case INT: return (*(int *)value1 <= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}

bool igreater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 >= *(float*)value2);
    case INT: return (*(int *)value1 >= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}

bool inot_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 != *(float*)value2);
    case INT: return (*(int *)value1 != *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}

IX_IndexScan::IX_IndexScan(){
  openScan = false;  
  value = NULL;
  initializedValue = false;
  hasBucketPinned = false;
  hasLeafPinned = false;
  scanEnded = true;
  scanStarted = false;
  endOfIndexReached = true;
  attrLength = 0;
  attrType = INT;

  foundFirstValue = false;
  foundLastValue = false;
  useFirstLeaf = false;
}

IX_IndexScan::~IX_IndexScan(){
	if(scanEnded == false && hasBucketPinned == true){
		indexHandle -> pfh.UnpinPage(currBucketNum);
	}
	if(scanEnded ==  false && hasLeafPinned == true && (currLeafNum != (indexHandle -> header).rootPage)){
		indexHandle -> pfh.UnpinPage(currLeafNum);
	}
	if(initializedValue == true){
		free(value);
		initializedValue = false;
	}
}

RC IX_IndexScan::OpenScan (const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint pinHint){
	RC rc = 0;
	if(openScan == true || compOp == NE_OP){
		return (IX_INVALIDSCAN);
	}
	if(indexHandle.isValidIndexHeader()){
		this -> indexHandle = const_cast<IX_IndexHandle*>(&indexHandle);
	}else{
		return (IX_INVALIDSCAN);
	}
	this -> value = NULL;
	useFirstLeaf = true;
	this -> compOp = compOp;
	switch(compOp){
		case EQ_OP :
			comparator = &iequal;
			useFirstLeaf = false;
			break;
		case LT_OP :
			comparator = &iless_than;
			break;
		case GT_OP :
			comparator = &igreater_than;
			useFirstLeaf = false;
		case LE_OP :
			comparator = &iless_than_or_eq_to;
			break;
		case GE_OP :
			comparator = &igreater_than_or_eq_to;
			useFirstLeaf = false;
			break;
		case NO_OP :
			comparator = NULL;
			break;
		default:
			return (IX_INVALIDSCAN);
	}
	this -> attrType = (indexHandle.header).attrType;
	this -> attrLength = ((this -> indexHandle) -> header).attrLength;
	if(compOp != NO_OP){
		this -> value = (void*)malloc(this -> attrLength);
		memcpy(this -> value, value, attrLength);
		initializedValue = true;
	}
	openScan = true;
	scanEnded = false;
	hasLeafPinned = false;
	scanStarted = false;
	endOfIndexReached = false;
	foundFirstValue = false;
	foundLastValue = false;
	return (rc);
}

RC IX_IndexScan::CloseScan(){
	RC rc = 0;
	if(openScan == false){
		return (IX_INVALIDSCAN);
	}
	if(scanEnded == false && hasBucketPinned == true){
		(indexHandle -> pfh).UnpinPage(currBucketNum);
	}
	if(scanEnded == false && hasLeafPinned == true &&(currLeafNum != (indexHandle -> header).rootPage)){
		indexHandle->pfh.UnpinPage(currLeafNum);
	}
	if(initializedValue == true){
		free(value);
		initializedValue = false;
	}
	openScan = false;
	scanStarted = false;

	return (rc);
}

RC IX_IndexScan::GetNextEntry(RID &rid){
	RC rc = 0;
	if(scanEnded == true && openScan == true){
		return (IX_EOF);
	}
	if(foundLastValue == true){
		return (IX_EOF);
	}
	if(scanEnded == true || openScan == false){
		return (IX_INVALIDSCAN);
	}
	bool notFound = true;
	while(notFound){
		if(scanEnded == false && openScan == true && scanStarted == false){
			if((rc = BeginScan(currLeafPH, currLeafNum))){
				return (rc);
			}
			currKey = nextNextKey;
			scanStarted = true;
			SetRID(true);
			if((IX_EOF == FindNextValue())){
				endOfIndexReached = true;
			}
		}else{
			currKey = nextKey;
			currRID = nextRID;
		}
		SetRID(false);
		nextKey = nextNextKey;
		if((IX_EOF == FindNextValue())){
			endOfIndexReached = true;
		}
		PageNum thisRIDPage;
		if((rc = currRID.GetPageNum(thisRIDPage))){
			return (rc);
		}
		if(thisRIDPage == -1){
			scanEnded = true;
			return (IX_EOF);
		}
		if(compOp == NO_OP){
			rid = currRID;
			notFound = false;
			foundFirstValue = true;
		}else if((comparator((void*)currKey, value, attrType, attrLength))){
			rid = currRID;
			notFound = false;
			foundFirstValue = true;
		}else if(foundFirstValue == true){
			foundLastValue = true;
			return (IX_EOF);
		}
	}
	SlotNum thisRIDPage;
	currRID.GetSlotNum(thisRIDPage);
	return rc;
}

RC IX_IndexScan::FindNextValue(){
	RC rc = 0;
	if(hasBucketPinned){
		int prevSlot = bucketSlot;
		bucketSlot = bucketEntries[prevSlot].nextSlot;
		if(bucketSlot != NO_MORE_SLOTS){
			return (0);
		}
		PageNum nextBucket = bucketHeader -> nextBucket;
		if((rc = (indexHandle -> pfh).UnpinPage(currBucketNum))){
			return (rc);
		}
		hasBucketPinned = false;
		if(nextBucket != NO_MORE_PAGES){
			if((rc = GetFirstBucketEntry(nextBucket, currBucketPH))){
				return (rc);
			}
			currBucketNum = nextBucket;
			return (0);
		}
	}
	int prevLeafSlot = leafSlot;
	leafSlot = leafEntries[prevLeafSlot].nextSlot;
	if(leafSlot != NO_MORE_SLOTS && leafEntries[leafSlot].isValid == OCCUPIED_DUP){
		nextNextKey = leafKeys + leafSlot*attrLength;
		currBucketNum = leafEntries[leafSlot].page;
		if((rc = GetFirstBucketEntry(currBucketNum, currBucketPH))){
			return (rc);
		}
		return (0);
	}
	if(leafSlot != NO_MORE_SLOTS && leafEntries[leafSlot].isValid == OCCUPIED_NEW){
		nextNextKey = leafKeys + leafSlot*attrLength;
		return (0);
	}
	PageNum nextLeafPage = leafHeader -> nextPage;
	if((currLeafNum != (indexHandle->header).rootPage)){
		if((rc = (indexHandle -> pfh).UnpinPage(currLeafNum))){
			return (rc);
		}
	}
	hasLeafPinned = false;
	if(nextLeafPage != NO_MORE_PAGES){
		currLeafNum = nextLeafPage;
		if((rc = (indexHandle->pfh).GetThisPage(currLeafNum, currLeafPH))){
			return (rc);
		}
		if((rc = GetFirstEntryInLeaf(currLeafPH))){
			return (rc);
		}
		return (0);
	}
	return (IX_EOF);
}

RC IX_IndexScan::SetRID(bool setCurrent){
	RC rc = 0;
	if(endOfIndexReached && setCurrent == false){
		RID rid1(-1, -1);
		nextRID = rid1;
		return (0);
	}
	if(setCurrent){
		if(hasBucketPinned){
			RID rid1(bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
			currRID = rid1;
		}else if(hasLeafPinned){
			RID rid1(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
			currRID = rid1;
		}
	}else{
		if(hasBucketPinned){
			RID rid1(bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
			nextRID = rid1;
		}else if(hasLeafPinned){
			RID rid1(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
			nextRID = rid1;
		}
	}
	return (rc);
}

RC IX_IndexScan::BeginScan(PF_PageHandle &leafPH, PageNum &pageNum){
	RC rc = 0;
	if(useFirstLeaf){
		if((rc = indexHandle -> GetFirstLeafPage(leafPH, pageNum))){
			return (rc);
		}
		if((rc = GetFirstEntryInLeaf(currLeafPH))){//改成leafPH会好懂一点
			if(rc == IX_EOF){
				scanEnded = true;
			}
			return (rc);
		}
	}else{
		if((rc = indexHandle -> FindRecordPage(leafPH, pageNum, value))){
			return (rc);
		}
		if((rc = GetAppropriateEntryInLeaf(currLeafPH))){
			if(rc == IX_EOF){
				scanEnded = true;
			}
			return (rc);
		}
	}
	return (rc);
}

RC IX_IndexScan::GetAppropriateEntryInLeaf(PF_PageHandle &leafPH){
	RC rc = 0;
	hasLeafPinned = true;
	if((rc = leafPH.GetData((char*&) leafHeader))){
		return (rc);
	}
	if(leafHeader -> num_keys == 0){
		return(IX_EOF);
	}
	leafEntries = (struct Node_Entry*)((char*)leafHeader + (indexHandle->header).entryOffset_N);
	leafKeys = (char*)leafHeader + (indexHandle->header).keysOffset_N;
	int index = 0;
	bool isDup = false;
	if((rc = indexHandle -> FindInsertPos((struct IX_NodeHeader*)leafHeader, value, index, isDup))){
		return (rc);
	}
	//加入特判
	if(index == NO_MORE_SLOTS){
		return (IX_INVALIDSCAN);
	}else if(compOp == GE_OP || compOp == GT_OP){
		char *tempKey = leafKeys + attrLength * index;
		if(!(comparator((void*)tempKey, value, attrType, attrLength))){
			printf("indexScan test gt ge\n");
			index = leafEntries[index].nextSlot;
		}
		if(index == NO_MORE_SLOTS){
			printf("indexScan change leaf gt ge\n");
			PageNum tempNextLeafPage = leafHeader -> nextPage;
			if((currLeafNum != (indexHandle->header).rootPage)){
				if((rc = (indexHandle -> pfh).UnpinPage(currLeafNum))){
					return (rc);
				}
			}
			hasLeafPinned = false;
			if(tempNextLeafPage == NO_MORE_PAGES){
				return (IX_EOF);
			}else{
				currLeafNum = tempNextLeafPage;
				if((rc = (indexHandle->pfh).GetThisPage(currLeafNum, currLeafPH))){
					return (rc);
				}
				hasLeafPinned = true;
				if((rc = leafPH.GetData((char*&) leafHeader))){
					return (rc);
				}
				if(leafHeader -> num_keys == 0){
					return(IX_EOF);
				}
				leafEntries = (struct Node_Entry*)((char*)leafHeader + (indexHandle->header).entryOffset_N);
				leafKeys = (char*)leafHeader + (indexHandle->header).keysOffset_N;
				index = leafHeader -> firstSlotIndex;
				if(index == NO_MORE_SLOTS){
					return (IX_INVALIDSCAN);
				}
			}
		}
	}
	//
	leafSlot = index;

	if((leafSlot != NO_MORE_SLOTS)){
		nextNextKey = leafKeys + attrLength * leafSlot;
	}else{
		return (IX_INVALIDSCAN);
	}
	if(leafEntries[leafSlot].isValid == OCCUPIED_DUP){
		currBucketNum = leafEntries[leafSlot].page;
		if((rc = GetFirstBucketEntry(currBucketNum, currBucketPH))){
			return (rc);
		}
	}
	return (rc);
}

RC IX_IndexScan::GetFirstEntryInLeaf(PF_PageHandle &leafPH){
	RC rc = 0;
	hasLeafPinned = true;
	if((rc = leafPH.GetData((char *&) leafHeader))){
		return (rc);
	}
	if(leafHeader -> num_keys == 0){
		return (IX_EOF);
	}
	leafEntries = (struct Node_Entry*)((char *)leafHeader + (indexHandle -> header).entryOffset_N);
	leafKeys = (char *)leafHeader + (indexHandle -> header).keysOffset_N;
	
	leafSlot = leafHeader -> firstSlotIndex;
	if(leafSlot != NO_MORE_SLOTS){
		nextNextKey = leafKeys + attrLength*leafSlot;
	}else{
		return (IX_INVALIDSCAN);
	}
	if(leafEntries[leafSlot].isValid == OCCUPIED_DUP){
		currBucketNum = leafEntries[leafSlot].page;
		if((rc = GetFirstBucketEntry(currBucketNum, currBucketPH))){
			return (rc);
		}
	}
	return (rc);
}

RC IX_IndexScan::GetFirstBucketEntry(PageNum nextBucket, PF_PageHandle &bucketPH){
	RC rc = 0;
	if((rc = (indexHandle->pfh).GetThisPage(nextBucket, currBucketPH))){
		return (rc);
	}
	hasBucketPinned = true;
	if((rc = bucketPH.GetData((char*&) bucketHeader))){
		return (rc);
	}
	bucketEntries = (struct Bucket_Entry*)((char *)bucketHeader + (indexHandle -> header).entryOffset_B);
	bucketSlot = bucketHeader -> firstSlotIndex;


	return (rc);
}
