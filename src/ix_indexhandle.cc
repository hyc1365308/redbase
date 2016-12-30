#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <math.h>
#include <cstdio>

IX_IndexHandle::IX_IndexHandle(){
  isOpenHandle = false;       // indexhandle is initially closed
  header_modified = false;
}

IX_IndexHandle::~IX_IndexHandle(){

}
RC IX_IndexHandle::InsertEntry	(void *pData, const RID &rid){
	if(! isValidIndexHeader() || isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
	RC rc = 0;
	struct IX_NodeHeader *rHeader;
	if((rc = rootPH.GetData((char *&)rHeader))){
		return rc;
	}
	if(rHeader -> num_keys == header.maxKeys_N){
		PageNum newRootPage;
		char *newRootData;
		PF_PageHandle newRootPH;
		if((rc = CreateNewNode(newRootPH,newRootPage,newRootData,false))){
			return rc;
		}
		struct IX_NodeHeader_I *newrHeader = (struct IX_NodeHeader_I *)newRootData;
		newrHeader->firstPage = header.rootPage;
		newrHeader->isEmpty = false;

		int newIndex;
		int splitPage;
		if((rc = SplitPage((struct IX_NodeHeader *&)newrHeader, (struct IX_NodeHeader *&)rHeader, header.rootPage, 
			BEGINNING_OF_SLOTS, newIndex, splitPage))){
			return rc;
		}
		if((rc = pfh.MarkDirty(header.rootPage))||(rc = pfh.UnpinPage(header.rootPage))){
			return rc;
		}
		rootPH = newRootPH;
		header.rootPage = newRootPage;
		header_modified = true;
		
		struct IX_NodeHeader *newrootHeader;
		if((rc = newRootPH.GetData((char*&)newrootHeader))){
			return rc;
		}
		//从新的根节点重新开始插入。
		if((rc = InsertSpacefulNode(newrootHeader, newRootPage, pData ,rid))){
			return(rc);
		}
	}
	else{
		if((rc = InsertSpacefulNode(rHeader, header.rootPage, pData ,rid))){
			return(rc);
		}
	}
	
	if((rc = pfh.MarkDirty(header.rootPage)))
    	return (rc);

    //printf("PageNum = %d, SlotNum = %d, value = %d\n", rid.Page(), rid.Slot(), *(int *)pData);
    //printf("num_keys = %d, firstSlotIndex = %d\n", rHeader->num_keys, rHeader->firstSlotIndex);
  	return (rc);
}
/*
	删除一个记录
*/
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid){
	RC rc = 0;
	if(! isValidIndexHeader() || isOpenHandle == false){
    	return (IX_INVALIDINDEXHANDLE);
	}

	struct IX_NodeHeader *rHeader;
	if((rc = rootPH.GetData((char *&)rHeader))){
		return rc;
	}

	if((rHeader -> isEmpty)&&(! rHeader -> isLeafNode)){
		return IX_INVALIDENTRY;
	}
	if((rHeader -> num_keys == 0)&&(rHeader -> isLeafNode)){
		return (IX_INVALIDENTRY);
	}

	bool deleteNext = false;
	if((rc = DeleteFromNode(rHeader, pData, rid, deleteNext))){
		return (rc);
	}
	if(deleteNext){
		rHeader -> isLeafNode = true;
	}
	if((rc = pfh.MarkDirty(header.rootPage))){
		return (rc);
	}
	return (rc);

}

/*
	分裂已经满了的节点，并在父节点中增加index
*/
RC IX_IndexHandle::SplitPage(struct IX_NodeHeader *parentHeader, struct IX_NodeHeader *oldHeader, PageNum oldPage,
	int oldIndex, int &newIndex, PageNum &splitPage){
	RC rc = 0;
	bool isLeaf = oldHeader->isLeafNode;
	PageNum newPage;
	struct IX_NodeHeader *newHeader;
	PF_PageHandle newPH;
	if((rc = CreateNewNode(newPH, newPage, (char *&)newHeader, isLeaf))){
		return rc;
	}
	splitPage = newPage;
	struct Node_Entry *parentEntry = (struct Node_Entry*)((char*)parentHeader + header.entryOffset_N);
	struct Node_Entry *oldEntry = (struct Node_Entry*)((char*)oldHeader + header.entryOffset_N);
	struct Node_Entry *newEntry = (struct Node_Entry*)((char*)newHeader + header.entryOffset_N);
	char *parentKey = (char *)parentHeader + header.keysOffset_N;
	char *oldKey = (char *)oldHeader + header.keysOffset_N;
	char *newKey = (char *)newHeader + header.keysOffset_N;

	int prev = BEGINNING_OF_SLOTS;
	int current = oldHeader->firstSlotIndex;
	for(int i = 0; i < header.maxKeys_N/2; i++){
		prev = current;
		current = oldEntry[current].nextSlot;
	}
	oldEntry[prev].nextSlot = NO_MORE_SLOTS;
	char *newIndexKey = oldKey + header.attrLength*current;

	if(!isLeaf){
		struct IX_NodeHeader_I *newIHeader = (struct IX_NodeHeader_I *)newHeader;
		newIHeader->firstPage = oldEntry[current].page;
		newIHeader->isEmpty = false;
		prev = current;
		current = oldEntry[current].nextSlot;
		oldEntry[prev].nextSlot = oldHeader->freeSlotIndex;
		oldHeader->freeSlotIndex = prev;
		oldHeader->num_keys--;
	}

	int newPrev = BEGINNING_OF_SLOTS;
	int newCurrent = newHeader->freeSlotIndex;
	while(current != NO_MORE_SLOTS){
		newEntry[newCurrent].page = oldEntry[current].page;
		newEntry[newCurrent].slot = oldEntry[current].slot;
		newEntry[newCurrent].isValid = oldEntry[current].isValid;
		memcpy(newKey + header.attrLength * newCurrent, oldKey + header.attrLength * current, header.attrLength);
		newHeader->freeSlotIndex = newEntry[newCurrent].nextSlot;
		if(newPrev == BEGINNING_OF_SLOTS){
			newEntry[newCurrent].nextSlot = newHeader->firstSlotIndex;
			newHeader->firstSlotIndex = newCurrent;
		}else{
			newEntry[newCurrent].nextSlot = newEntry[newPrev].nextSlot;
			newEntry[newPrev].nextSlot = newCurrent;
		}

		newPrev = newCurrent;
		newCurrent = newHeader -> freeSlotIndex;
		prev = current;
		current = oldEntry[current].nextSlot;

		oldEntry[prev].nextSlot = oldHeader->freeSlotIndex;
		oldEntry[prev].isValid = UNOCCUPIED;
		oldHeader->freeSlotIndex = prev;
		oldHeader->num_keys--;
		newHeader->num_keys++;
	}

	newIndex = parentHeader->freeSlotIndex;
	memcpy(parentKey + header.attrLength*newIndex, newIndexKey, header.attrLength);
	parentEntry[newIndex].page = newPage;
	parentEntry[newIndex].isValid = OCCUPIED_NEW;
	parentHeader->freeSlotIndex = parentEntry[newIndex].nextSlot;
	if(oldIndex == BEGINNING_OF_SLOTS){
		parentEntry[newIndex].nextSlot = parentHeader->firstSlotIndex;
		parentHeader->firstSlotIndex = newIndex;
	}else{
		parentEntry[newIndex].nextSlot = parentEntry[oldIndex].nextSlot;
		parentEntry[oldIndex].nextSlot = newIndex;
	}
	parentHeader->num_keys++;

	if(isLeaf){
		struct IX_NodeHeader_L *newLHeader = (struct IX_NodeHeader_L *)newHeader;
		struct IX_NodeHeader_L *oldLHeader = (struct IX_NodeHeader_L *)oldHeader;
		newLHeader -> prevPage = oldPage;
		newLHeader->nextPage = oldLHeader->nextPage;
		oldLHeader->nextPage = newPage;
		if(newLHeader->nextPage != NO_MORE_PAGES){
			PF_PageHandle nextPH;
			struct IX_NodeHeader_L *nextLHeader;
			if((rc = pfh.GetThisPage(newLHeader->nextPage, nextPH))||(rc = nextPH.GetData((char*&)nextLHeader))){
				return rc;
			}
			nextLHeader->prevPage = newPage;
			if((rc = pfh.MarkDirty(newLHeader->nextPage))||(rc = pfh.UnpinPage(newLHeader->nextPage))){
				return rc;
			}
		}
	}
	if((rc = pfh.MarkDirty(newPage))||(rc = pfh.UnpinPage(newPage))){
		return rc;
	}
	return rc;

}
/*
	创建新的节点
*/
RC IX_IndexHandle::CreateNewNode(PF_PageHandle &ph, PageNum &page, char *&data, bool isLeaf){
	RC rc = 0;
	if((rc = pfh.AllocatePage(ph))||(rc = ph.GetPageNum(page))||(rc = ph.GetData(data))){
		return rc;
	}
	struct IX_NodeHeader *nodeHeader = (struct IX_NodeHeader *)data;
	nodeHeader->isLeafNode = isLeaf;
	nodeHeader->isEmpty = true;
	nodeHeader->num_keys = 0;
	nodeHeader->firstSlotIndex = NO_MORE_SLOTS;
	nodeHeader->freeSlotIndex = 0;
	nodeHeader->notUsed1 = NO_MORE_PAGES;
	nodeHeader->notUsed2 = NO_MORE_PAGES;
	struct Node_Entry *entry = (struct Node_Entry *)(nodeHeader + header.entryOffset_N);
	for(int i = 0; i < header.maxKeys_N; i++){
		entry[i].isValid = UNOCCUPIED;
		entry[i].nextSlot = i + 1;
		entry[i].page = NO_MORE_PAGES;
		if(i == header.maxKeys_N - 1){
			entry[i].nextSlot = NO_MORE_SLOTS;
		}
	}
	return rc;
}

/*
	向有空闲空间的页里插入节点
*/
RC IX_IndexHandle::InsertSpacefulNode(IX_NodeHeader *nodeHeader, PageNum nodePage, void *pData, const RID &rid){
	RC rc = 0;
	
	struct Node_Entry *entry = (struct Node_Entry *)((char *)nodeHeader + header.entryOffset_N);
	char * key = (char *)nodeHeader + header.keysOffset_N;
	if(!(nodeHeader -> isLeafNode)){
		struct IX_NodeHeader_I * iHeader = (struct IX_NodeHeader_I*)nodeHeader;
		PageNum nextNodePage;
		int insertIndex =  BEGINNING_OF_SLOTS;
		bool isDup;
		if((rc = FindInsertPos(nodeHeader, pData, insertIndex, isDup))){
			return(rc);
		}
		if(insertIndex == BEGINNING_OF_SLOTS){
			nextNodePage = iHeader -> firstPage;
		}else{
			nextNodePage = entry[insertIndex].page;
		}

		PF_PageHandle nextNodePH;
		struct IX_NodeHeader *nextNodeHeader;
		if((rc = pfh.GetThisPage(nextNodePage, nextNodePH))||(rc = nextNodePH.GetData((char*&)nextNodeHeader))){
			return (rc);
		}
		if(nextNodeHeader -> num_keys == header.maxKeys_N){
			int newIndex;
			int splitPage;
			if((rc = SplitPage(nodeHeader, nextNodeHeader, nextNodePage, insertIndex, newIndex, splitPage))){
				return (rc);
			}
			char* newKey = key +header.attrLength * newIndex;
			int ret = comparator(pData, newKey, header.attrLength);
			if(ret >= 0){
				if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage)))
          			return (rc);
          		nextNodePage = splitPage;
          		if((rc = pfh.GetThisPage(nextNodePage, nextNodePH))||(rc = nextNodePH.GetData((char*&)nextNodeHeader))){
					return (rc);
				}
			}
		}

		if((rc = InsertSpacefulNode(nextNodeHeader, nextNodePage, pData ,rid))){
			return(rc);
		}
		if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage))){
          			return (rc);
        }
	}else{
		struct IX_NodeHeader_L *lHeader = (struct IX_NodeHeader_L*) nodeHeader;
		int insertIndex = BEGINNING_OF_SLOTS;
		bool isDup = false;
		if((rc = FindInsertPos(nodeHeader, pData, insertIndex, isDup))){
			return (rc);
		}
		if(!isDup){//无重复节点时
			int index = nodeHeader -> freeSlotIndex;
			memcpy(key + header.attrLength * index, (char*)pData, header.attrLength);
			entry[index].isValid = OCCUPIED_NEW;
			if((rc = rid.GetPageNum(entry[index].page)) || (rc = rid.GetSlotNum(entry[index].slot))){
				return rc;
			}
			nodeHeader -> isEmpty = false;
			nodeHeader -> num_keys++;
			nodeHeader -> freeSlotIndex = entry[index].nextSlot;
			if(insertIndex == BEGINNING_OF_SLOTS){
				entry[index].nextSlot = nodeHeader -> firstSlotIndex;
				nodeHeader -> firstSlotIndex = index;
			}else{
				entry[index].nextSlot = entry[insertIndex].nextSlot;
				entry[insertIndex].nextSlot = index;
			}
		}else{//当有重复节点时
			PageNum bucketPage;
			if(isDup && entry[insertIndex].isValid == OCCUPIED_NEW){
				if((rc = CreateNewBucket(bucketPage))){
					return (rc);
				}
				entry[insertIndex].isValid = OCCUPIED_DUP;
				RID rid0(entry[insertIndex].page, entry[insertIndex].slot);
				/*bool recur = false;
				if((rc = CheckBucketRID(bucketPage, rid, recur))){
					return (rc);
				}
				if(recur){
					rc = IX_DUPLICATEENTRY;
					return (rc);
				}*/
				if((rc = InsertIntoBucket(bucketPage, rid0))||(rc = InsertIntoBucket(bucketPage, rid))){
					return (rc);
				}
				entry[insertIndex].page = bucketPage;
				entry[insertIndex].slot = NO_MORE_SLOTS;
			}else{
				bucketPage = entry[insertIndex].page;
				/*bool recur = false;
				if((rc = CheckBucketRID(bucketPage, rid, recur))){
					return (rc);
				}
				if(recur){
					rc = IX_DUPLICATEENTRY;
					return (rc);
				}*/
				if((rc = InsertIntoBucket(bucketPage, rid))){
					return (rc);
				}
			}
		}
	}
	return (rc);
}
/*
	创建新的bucket页
*/
RC IX_IndexHandle::CreateNewBucket(PageNum &bucketPage){
	struct IX_BucketHeader *bucketHeader;
	PF_PageHandle ph;
	RC rc = 0;
	if((rc = pfh.AllocatePage(ph))||(rc = ph.GetPageNum(bucketPage))){
		return rc;
	}
	if((rc = ph.GetData((char *&)bucketHeader))){
		return rc;
	}
	bucketHeader -> num_keys = 0;
	bucketHeader -> firstSlotIndex = NO_MORE_SLOTS;
	bucketHeader -> freeSlotIndex = 0;
	bucketHeader -> nextBucket = NO_MORE_PAGES;
	struct Bucket_Entry *entry = (struct Bucket_Entry *)((char*)bucketHeader + header.entryOffset_B);
	for(int i = 0; i < header.maxKeys_B; i++){
		if(i == (header.maxKeys_B - 1)){
			entry[i].nextSlot = NO_MORE_SLOTS;
		}else{
			entry[i].nextSlot = i + 1;
		}
	}
	if((rc = pfh.MarkDirty(bucketPage))||(rc = pfh.UnpinPage(bucketPage))){
		return (rc);
	}

	return (rc);
}
/*
	简化版的向bucket中插入记录
*/
RC IX_IndexHandle::InsertIntoBucket(PageNum page, const RID &rid){
	RC rc = 0;
	PageNum ridPage;
	SlotNum ridSlot;
	if((rc = rid.GetPageNum(ridPage)) || (rc = rid.GetSlotNum(ridSlot))){
		return (rc);
	}

	bool notEnd = true;
	PageNum currPage = page;
	PF_PageHandle bucketPH;
	struct IX_BucketHeader *bucketHeader;
	while(notEnd){

		if((rc = pfh.GetThisPage(currPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bucketHeader))){
			return (rc);
		}

		struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bucketHeader + header.entryOffset_B);
		int prev_idx = BEGINNING_OF_SLOTS;
		int curr_idx = bucketHeader->firstSlotIndex;
		while(curr_idx != NO_MORE_SLOTS){

			if(entries[curr_idx].page == ridPage && entries[curr_idx].slot == ridSlot){
				RC rc2 = 0; 
				if((rc2 = pfh.UnpinPage(currPage)))
					return (rc2);
				return (IX_DUPLICATEENTRY);
			}
			prev_idx = curr_idx;
			curr_idx = entries[prev_idx].nextSlot;
		}

		if(bucketHeader->nextBucket == NO_MORE_PAGES && bucketHeader->num_keys == header.maxKeys_B){
			notEnd = false; 
			PageNum newBucketPage;
			PF_PageHandle newBucketPH;
			if((rc = CreateNewBucket(newBucketPage)))
				return (rc);
			bucketHeader->nextBucket = newBucketPage;
			if((rc = pfh.MarkDirty(currPage)) || (rc = pfh.UnpinPage(currPage)))
				return (rc);

			currPage = newBucketPage; 
			if((rc = pfh.GetThisPage(currPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bucketHeader)))
				return (rc);
			entries = (struct Bucket_Entry *)((char *)bucketHeader + header.entryOffset_B);
		}

		if(bucketHeader->nextBucket == NO_MORE_PAGES){
			notEnd = false; // end search
			int loc = bucketHeader->freeSlotIndex;  
			entries[loc].slot = ridSlot;
			entries[loc].page = ridPage;
			bucketHeader->freeSlotIndex = entries[loc].nextSlot;
			entries[loc].nextSlot = bucketHeader->firstSlotIndex;
			bucketHeader->firstSlotIndex = loc;
			bucketHeader->num_keys++;
		}

		PageNum nextPage = bucketHeader->nextBucket;
		if((rc = pfh.MarkDirty(currPage)) || (rc = pfh.UnpinPage(currPage)))
			return (rc);
		currPage = nextPage;
	}
	return (0);
}

RC IX_IndexHandle::InsertIntoBucket2(PageNum thisPage, const RID rid){
	RC rc = 0;
	PF_PageHandle thisPH;
	struct IX_BucketHeader *thisHeader;
	if((rc = pfh.GetThisPage(thisPage, thisPH))||(rc = thisPH.GetData((char*&)thisHeader))){
		return (rc);
	}
	struct Bucket_Entry *thisEntry = (struct Bucket_Entry *)((char *)thisHeader + header.entryOffset_B);
	if(thisHeader -> num_keys == header.maxKeys_B){
		if(thisHeader -> nextBucket == NO_MORE_PAGES){
			PageNum  newBucketPage = NO_MORE_PAGES;
			if((rc = CreateNewBucket(newBucketPage))){
				return (rc); 
			}
			thisHeader -> nextBucket = newBucketPage;
			if((rc = InsertIntoBucket(thisHeader -> nextBucket, rid))){
				return (rc);
			}
			if((rc = pfh.MarkDirty(thisPage))||(rc = pfh.UnpinPage(thisPage))){
				return (rc);
			}
		}else{
			if((rc = InsertIntoBucket(thisHeader -> nextBucket, rid))){
				return (rc);
			}
			if((rc = pfh.UnpinPage(thisPage))){
				return (rc);
			}
		}
	}else{
		int insertIndex;
		insertIndex = thisHeader -> freeSlotIndex;
		thisHeader -> freeSlotIndex = thisEntry[insertIndex].nextSlot;
		thisEntry[insertIndex].nextSlot = thisHeader -> firstSlotIndex;
		thisHeader -> firstSlotIndex = insertIndex;
		thisHeader -> num_keys++;
		if((rc = pfh.MarkDirty(thisPage))||(rc = pfh.UnpinPage(thisPage))){
			return (rc);
		}
	}
	return (rc);
}

/*
	从节点中删除一条记录
*/
RC IX_IndexHandle::DeleteFromNode(struct IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete){
	RC rc = 0;
	toDelete = false;
	if(nHeader->isLeafNode){ 
		if((rc = DeleteFromLeaf((struct IX_NodeHeader_L *)nHeader, pData, rid, toDelete))){
			return (rc);
		}
	}

	else{
		int prevIndex, currIndex;
		bool isDup;
		if((rc = FindInsertPos(nHeader, pData, currIndex, isDup)))
			return (rc);
		struct IX_NodeHeader_I *iHeader = (struct IX_NodeHeader_I *)nHeader;
		struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
		PageNum nextNodePage;
		bool useFirstPage = false;
		if(currIndex == BEGINNING_OF_SLOTS){
			useFirstPage = true; 
			nextNodePage = iHeader->firstPage;
			prevIndex = currIndex;
		}
		else{
			if((rc = FindPrevIndex(nHeader, currIndex, prevIndex)))
				return (rc);
			nextNodePage = entries[currIndex].page;
			}

		PF_PageHandle nextNodePH;
		struct IX_NodeHeader *nextHeader;
		if((rc = pfh.GetThisPage(nextNodePage, nextNodePH)) || (rc = nextNodePH.GetData((char *&)nextHeader)))
			return (rc);
		bool toDeleteNext = false;
		rc = DeleteFromNode(nextHeader, pData, rid, toDeleteNext); 
		RC rc2 = 0;

		if((rc2 = pfh.MarkDirty(nextNodePage)) || (rc2 = pfh.UnpinPage(nextNodePage)))
			return (rc2);
		if(rc == IX_INVALIDENTRY)
			return (rc);

		if(toDeleteNext){
			if((rc = pfh.DisposePage(nextNodePage))){
				return (rc);
			}
			if(useFirstPage == false){
				if(prevIndex == BEGINNING_OF_SLOTS)
					nHeader->firstSlotIndex = entries[currIndex].nextSlot;
				else
					entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
				entries[currIndex].nextSlot = nHeader->freeSlotIndex;
				nHeader->freeSlotIndex = currIndex;
			}
			else{
				if(nHeader -> num_keys > 0){
					int firstslot = nHeader->firstSlotIndex;
					nHeader->firstSlotIndex = entries[firstslot].nextSlot;
					iHeader->firstPage = entries[firstslot].page;
					entries[firstslot].nextSlot = nHeader->freeSlotIndex;
					nHeader->freeSlotIndex = firstslot;
				}else{
					iHeader -> firstPage = NO_MORE_PAGES;
				}
			}

			if(nHeader->num_keys == 0){
				nHeader->isEmpty = true;
				toDelete = true;
			}

			else
				nHeader->num_keys--;
      
      	}
	}

	return (rc);
}

/*
	从叶级页中删除记录
*/
RC IX_IndexHandle::DeleteFromLeaf(struct IX_NodeHeader_L *nodeHeader, void *pData, const RID &rid, bool &toDelete){
	RC rc = 0;
	int prev, current;
	bool isDup;
	if((rc = FindInsertPos((struct IX_NodeHeader*)nodeHeader, pData, current, isDup))){
		return (rc);
	}
	if(isDup == false){
		return (IX_INVALIDENTRY);
	}
	struct Node_Entry *entry = (struct Node_Entry*)((char *)nodeHeader + header.entryOffset_N);
	char * key = (char*)nodeHeader + header.keysOffset_N;

	if(current == nodeHeader -> firstSlotIndex){
		prev = current;
	}else{
		if((rc = FindPrevIndex((struct IX_NodeHeader*)nodeHeader, current, prev))){
			return (rc);
		}
	}
	if(entry[current].isValid == OCCUPIED_NEW){
		PageNum ridPage;
		SlotNum ridSlot;
		if((rc = rid.GetPageNum(ridPage))||(rc = rid.GetSlotNum(ridSlot))){
			return (rc);
		}
		if((entry[current].slot != ridSlot)||(entry[current].page != ridPage)){
			return (IX_INVALIDENTRY);
		}
		if(current == nodeHeader -> firstSlotIndex){
			nodeHeader -> firstSlotIndex = entry[current].nextSlot;
		}else{
			entry[prev].nextSlot = entry[current].nextSlot;
		}
		entry[current].nextSlot = nodeHeader -> freeSlotIndex;
		nodeHeader -> freeSlotIndex = current;
		entry[current].isValid = UNOCCUPIED;
		nodeHeader -> num_keys--;
	}else if(entry[current].isValid == OCCUPIED_DUP){
		PageNum bucketPage = entry[current].page;
		PF_PageHandle bucketPH;
		struct IX_BucketHeader *bucketHeader;
		bool deleteBucket = false;
		PageNum nextBucketPage;
		if((rc = pfh.GetThisPage(bucketPage, bucketPH))||(rc = bucketPH.GetData((char*&)bucketHeader))){
			return (rc);
		}
		rc = DeleteFromBucket(bucketHeader, rid, deleteBucket, nextBucketPage);
		RC rc2 = 0;
		if((rc2 = pfh.MarkDirty(bucketPage))||(rc = pfh.UnpinPage(bucketPage))){
			return (rc2);
		}
		if(rc == IX_INVALIDENTRY){
			return (rc);
		}
		if(deleteBucket){
			if((rc = pfh.DisposePage(bucketPage))){
				return (rc);
			}
			if(nextBucketPage == NO_MORE_PAGES){
				if(current == nodeHeader -> firstSlotIndex){
					nodeHeader -> firstSlotIndex = entry[current].nextSlot;
				}else{
					entry[prev].nextSlot = entry[current].nextSlot;
				}
				entry[current].nextSlot = nodeHeader -> freeSlotIndex;
				nodeHeader -> freeSlotIndex = current;
				entry[current].isValid = UNOCCUPIED;
				nodeHeader -> num_keys--;
			}else{
				entry[current].page = nextBucketPage;
			}
		}
	}

	if(nodeHeader -> num_keys == 0){
		toDelete = true;
		PageNum prevPage = nodeHeader -> prevPage;
		PageNum nextPage = nodeHeader -> nextPage;
		if(prevPage != NO_MORE_PAGES){
			PF_PageHandle prevPH;
			struct IX_NodeHeader_L *prevHeader;
			if((rc = pfh.GetThisPage(prevPage, prevPH))||(rc = prevPH.GetData((char*&)prevHeader))){
				return (rc);
			}
			prevHeader -> nextPage = nextPage;
			if((rc =pfh.MarkDirty(prevPage))||(rc = pfh.UnpinPage(prevPage))){
				return (rc);
			}
		}
		if(nextPage != NO_MORE_PAGES){
			PF_PageHandle nextPH;
			struct IX_NodeHeader_L *nextHeader;
			if((rc = pfh.GetThisPage(nextPage, nextPH))||(rc = nextPH.GetData((char*&)nextHeader))){
				return (rc);
			}
			nextHeader -> prevPage = prevPage;
			if((rc =pfh.MarkDirty(nextPage))||(rc = pfh.UnpinPage(nextPage))){
				return (rc);
			}
		}
	}

	return (rc);

}
/*
	从bucket中删除一条记录
*/
RC IX_IndexHandle::DeleteFromBucket(struct IX_BucketHeader *bucketHeader, const RID &rid, bool &deleteBucket, PageNum &nextBucketPage){
	RC rc = 0;
	PageNum nextBucketPage2 = bucketHeader -> nextBucket;
	nextBucketPage = bucketHeader -> nextBucket;
	struct Bucket_Entry *entry =  (struct Bucket_Entry*)((char*)bucketHeader + header.entryOffset_B);
	if(nextBucketPage2 != NO_MORE_PAGES){
		bool deleteNextBucket = false;
		PF_PageHandle nextBucketPH;
		struct IX_BucketHeader *nextBucketHeader;
		PageNum nextNextBucketPage;
		if((rc = pfh.GetThisPage(nextBucketPage2, nextBucketPH))||(rc = nextBucketPH.GetData((char *&)nextBucketHeader))){
			return (rc);
		}

		rc = DeleteFromBucket(nextBucketHeader, rid, deleteNextBucket, nextNextBucketPage);
		int nextNumKeys = nextBucketHeader -> num_keys;
		RC rc2;
		if((rc2 = pfh.MarkDirty(nextBucketPage2))||(rc = pfh.UnpinPage(nextBucketPage2))){
			return (rc2);
		}
		if(deleteNextBucket && (nextNumKeys == 0)){
			if((rc = pfh.DisposePage(nextBucketPage))){
				return (rc);
			}
			bucketHeader -> nextBucket = nextNextBucketPage;
			nextBucketPage = bucketHeader -> nextBucket;
		}
		if(rc == 0){
			return (0);
		}
	}
	PageNum ridPage;
	SlotNum ridSlot;
	if((rc = rid.GetPageNum(ridPage))||(rc = rid.GetSlotNum(ridSlot))){
		return (rc);
	}
	int prev = BEGINNING_OF_SLOTS;
	int current = bucketHeader -> firstSlotIndex;
	bool found = false;
	while(current != NO_MORE_SLOTS){
		if((entry[current].slot == ridSlot) && (entry[current].page == ridPage)){
			found = true;
			break;
		}
		prev = current;
		current = entry[current].nextSlot;
	}
	if(found){
		if(prev == BEGINNING_OF_SLOTS){
			bucketHeader -> firstSlotIndex = entry[current].nextSlot;
		}else{
			entry[prev].nextSlot = entry[current].nextSlot;
		}
		entry[current].nextSlot = bucketHeader -> freeSlotIndex;
		bucketHeader -> freeSlotIndex = current;
		bucketHeader -> num_keys--;
		
		if(bucketHeader -> num_keys == 0){
			deleteBucket = true;
		}

		return (rc); 
	}

	rc = IX_INVALIDENTRY;
	return (rc);
}
/*
	找到之前的记录
*/
RC IX_IndexHandle::FindPrevIndex(struct IX_NodeHeader* nodeHeader, int thisIndex, int &prevIndex){
	RC rc = 0;
	int prev = BEGINNING_OF_SLOTS;
	int current = nodeHeader -> firstSlotIndex;
	struct Node_Entry * entry = (struct Node_Entry*)((char*)nodeHeader + header.entryOffset_N);
	while(current != thisIndex){
		prev = current;
		current = entry[current].nextSlot;
	}
	prevIndex = prev;
	return (rc);
}
/*
	检查bucket里是否已有与要插入的rid重复的记录
*/
RC IX_IndexHandle::CheckBucketRID(PageNum thisPage, const RID rid, bool &recur){
	RC rc = 0;
	PF_PageHandle thisPH;
	struct IX_BucketHeader *thisHeader;
	PageNum ridPage;
	SlotNum ridSlot;
	if((rc = rid.GetPageNum(ridPage)) || (rc = rid.GetSlotNum(ridSlot))){
		return (rc);
	}
	if((rc = pfh.GetThisPage(thisPage, thisPH))||(rc = thisPH.GetData((char*&)thisHeader))){
		return (rc);
	}
	struct Bucket_Entry *thisEntry = (struct Bucket_Entry *)((char *)thisHeader + header.entryOffset_B);
	int prev = BEGINNING_OF_SLOTS;
	int current = thisHeader -> firstSlotIndex;
	while(current != NO_MORE_SLOTS){
		if((thisEntry[current].page == ridPage)&&(thisEntry[current].slot == ridSlot)){
			recur = true;
			break;
		}
		prev = current;
		current = thisEntry[current].nextSlot;
	}
	PageNum next = thisHeader -> nextBucket;
	if((rc = pfh.UnpinPage(thisPage))){
		return (rc);
	}
	if((next != NO_MORE_PAGES)&&(!recur)){
		if((rc = CheckBucketRID(next, rid, recur))){
			return (rc);
		}
	}
	return (rc);
}
/*
	返回插入的位置和是否是重复节点。
*/
RC IX_IndexHandle::FindInsertPos(struct IX_NodeHeader* nodeHeader, void* pData, int &index, bool& isDup){
	struct Node_Entry *entry = (struct Node_Entry*)((char *)nodeHeader + header.entryOffset_N);
	char *key = (char *)nodeHeader + header.keysOffset_N;
	int prev = BEGINNING_OF_SLOTS;
	int current = nodeHeader ->firstSlotIndex;
	isDup = false;
	while(current != NO_MORE_SLOTS){
		char *currentKey = key + header.attrLength * current;
		int ret = comparator(pData, (void *)currentKey, header.attrLength);
		if(ret == 0){
			isDup =true;
		}
		if(ret < 0){
			break;
		}
		prev = current;
		current = entry[current].nextSlot;
	}
	index = prev;
	return 0;
}
/*
	检查这个indexhandle的各个参数是否正确
*/
bool IX_IndexHandle::isValidIndexHeader() const{
  if(header.maxKeys_N <= 0 || header.maxKeys_B <= 0){
    printf("error 1");
    return false;
  }
  if(header.entryOffset_N != sizeof(struct IX_NodeHeader) || 
    header.entryOffset_B != sizeof(struct IX_BucketHeader)){
    printf("error 2");
    return false;
  }

  int attrLength2 = (header.keysOffset_N - header.entryOffset_N)/(header.maxKeys_N);
  if(attrLength2 != sizeof(struct Node_Entry)){
    printf("error 3");
    return false;
  }
  if((header.entryOffset_B + header.maxKeys_B * sizeof(Bucket_Entry) > PF_PAGE_SIZE) ||
    header.keysOffset_N + header.maxKeys_N * header.attrLength > PF_PAGE_SIZE)
    return false;
  return true;
}

/*
 计算一个node里的最大记录数量
*/
int IX_IndexHandle::CalcNumKeysNode(int attrLength){
	int keySpace = PF_PAGE_SIZE - sizeof(struct IX_NodeHeader);
	return floor(1.0 * keySpace / (sizeof(struct Node_Entry) + attrLength));
}

/*
	计算一个bucket里的最大记录数
*/
int IX_IndexHandle::CalcNumKeysBucket(int attrLength){
	int body_size = PF_PAGE_SIZE - sizeof(struct IX_BucketHeader);
	return floor(1.0*body_size / (sizeof(Bucket_Entry)));
}

RC IX_IndexHandle::GetFirstLeafPage(PF_PageHandle &leafPH, PageNum &leafPage) const{
	RC rc = 0;
	struct IX_NodeHeader* rootHeader;
	if((rc = rootPH.GetData((char*&)rootHeader))){
		return (rc);
	}
	if(rootHeader -> isLeafNode == true){
		leafPH = rootPH;
		leafPage = header.rootPage;
		return (rc);
	}

	struct IX_NodeHeader_I *nodeHeader = (struct IX_NodeHeader_I*)rootHeader;
	PageNum nextPage = nodeHeader -> firstPage;
	PF_PageHandle nextPH;
	if(nextPage == NO_MORE_PAGES)
		return (IX_EOF);
	if((rc = pfh.GetThisPage(nextPage, nextPH)) || (rc = nextPH.GetData((char*&)nodeHeader))){
		return (rc);
	}
	while(nodeHeader -> isLeafNode == false){
		PageNum prevPage = nextPage;
		nextPage = nodeHeader -> firstPage;
		if((rc = pfh.UnpinPage(prevPage))){
			return (rc);
		}
		if(nextPage == NO_MORE_PAGES)
			return (IX_EOF);
		if((rc = pfh.GetThisPage(nextPage, nextPH)) || (rc = nextPH.GetData((char*&)nodeHeader))){
			return (rc);
		}
	}
	leafPH = nextPH;
	leafPage = nextPage;

	return (rc);
}

RC IX_IndexHandle::FindRecordPage(PF_PageHandle &leafPH, PageNum &leafPage, void *key){
	RC rc = 0;
	struct IX_NodeHeader *rootHeader;
	if((rc = rootPH.GetData((char*&) rootHeader))){
		return (rc);
	}
	if(rootHeader -> isLeafNode == true){
		leafPH = rootPH;
		leafPage = header.rootPage;
		return (rc);
	}
	struct IX_NodeHeader_I *nodeHeader = (struct IX_NodeHeader_I*)rootHeader;
	int index = BEGINNING_OF_SLOTS;
	bool isDup = false;
	PageNum nextPage;
	PF_PageHandle nextPH;
	if((rc = FindInsertPos((struct IX_NodeHeader*)nodeHeader, key, index, isDup))){
		return (rc);
	}
	struct Node_Entry* entry = (struct Node_Entry*)((char*)nodeHeader + header.entryOffset_N);
	if(index == BEGINNING_OF_SLOTS){
		nextPage = nodeHeader -> firstPage;
	}else{
		nextPage = entry[index].page;
	}
	if(nextPage == NO_MORE_PAGES){
		return (IX_EOF);
	}
	if((rc = pfh.GetThisPage(nextPage, nextPH)) || (rc = nextPH.GetData((char*&)nodeHeader))){
		return (rc);
	}
	while(nodeHeader -> isLeafNode == false){
		if((rc = FindInsertPos((struct IX_NodeHeader*)nodeHeader, key, index, isDup))){
			return (rc);
		}
		entry = (struct Node_Entry*)((char*)nodeHeader + header.entryOffset_N);
		PageNum prevPage = nextPage;
		if(index == BEGINNING_OF_SLOTS){
			nextPage = nodeHeader -> firstPage;
		}else{
			nextPage = entry[index].page;
		}
		if((rc = pfh.UnpinPage(prevPage)))
			return (rc);
		if(nextPage == NO_MORE_PAGES){
			return (IX_EOF);
		}
		if((rc = pfh.GetThisPage(nextPage, nextPH)) || (rc = nextPH.GetData((char*&)nodeHeader))){
			return (rc);
		}
	}
	leafPage = nextPage;
	leafPH = nextPH;
	return (rc);
}

RC IX_IndexHandle::ForcePages(){
  RC rc = 0;
  if (isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
  pfh.ForcePages();
  return (rc);
}


