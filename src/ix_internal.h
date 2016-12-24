#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H
#include "ix.h"

#define NO_MORE_PAGES -1
#define NO_MORE_SLOTS -1

static const char UNOCCUPIED = 'u';
static const char OCCUPIED_NEW = 'n';
static const char OCCUPIED_DUP = 'r';

struct IX_NodeHeader{
	bool isLeafNode;
	bool isEmpty;
	int num_keys;
	int firstSlotIndex;
	int freeSlotIndex;
	PageNum notUsed1;
	PageNum notUsed2;
};
struct IX_NodeHeader_I{
	bool isLeafNode;
	bool isEmpty;
	int num_keys;
	int firstSlotIndex;
	int freeSlotIndex;
	PageNum firstPage;
	PageNum notUsed;
};
struct IX_NodeHeader_L{
	bool isLeafNode;
	bool isEmpty;
	int num_keys;
	int firstSlotIndex;
	int freeSlotIndex;
	PageNum prevPage;
	PageNum nextPage;
};
struct IX_BucketHeader{
	int num_keys;
	int firstSlotIndex;
	int freeSlotIndex;
	PageNum nextBucket;
};
struct Node_Entry{
	char isValid;
	int nextSlot;
	PageNum page;
	SlotNum slot;
};
struct  Bucket_Entry{
	int nextSlot;
	PageNum page;
	SlotNum slot;
};
#endif