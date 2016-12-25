
#include "redbase.h"

#ifndef RM_RID_H
#define RM_RID_H

//
// PageNum: uniquely identifies a page in a file
//
typedef int PageNum;

//
// SlotNum: uniquely identifies a record in a page
//
typedef int SlotNum;


//
// RID: Record id interface
//
class RID {
    static const PageNum INVALID_PAGE = -1;
    static const SlotNum INVALID_SLOT = -1;
public:
    RID();                                         // Default constructor
    RID(PageNum pageNum, SlotNum slotNum);
    RID& operator= (const RID &rid);               // Copies RID
    bool operator== (const RID &rid) const;

    RC GetPageNum(PageNum &pageNum) const;         // Return page number
    RC GetSlotNum(SlotNum &slotNum) const;         // Return slot number

    RC isValidRID() const; // checks if it is a valid RID
    PageNum Page() const{ return page; }
    SlotNum Slot() const{ return slot; }

private:
    PageNum page;
    SlotNum slot;
};

#endif
