//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"

//
// RM_FileHeader: FileHeader
//
struct RM_FileHeader{
    int recordSize;           // record size in file
    int numRecordsPerPage;    // calculated max # of recs per page
    int numPages;             // number of pages
    PageNum firstFreePage;    // pointer to first free object

    int bitmapOffset;         // location in bytes of where the bitmap starts
                              // in the page headers
    int bitmapSize;           // size of bitmaps in the page headers
    RM_FileHeader(RM_FileHeader *fh){
        recordSize = fh->recordSize;
        numRecordsPerPage = fh->numRecordsPerPage;
        numPages = fh->numPages;
        firstFreePage = fh->firstFreePage;
        bitmapOffset = fh->bitmapOffset;
        bitmapSize = fh->bitmapSize;
    }

};

//
// RM_PageHeader: PageHeader
//
struct RM_PageHeader{
    int nextFreePage;
    int numRecords;
};

//
// RM_Record: RM Record interface
//
class RM_Record {
    static const int INVALID_RECORD_SIZE = -1;
    friend class RM_FileHandle;
public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;

    // Sets the record with an RID, data contents, and its size
    RC SetRecord (RID rec_rid, char *recData, int size);

    RM_Record& operator= (const RM_Record &record);
private:
    RID rid;        // record RID
    char * data;    // pointer to record data. This is stored in the
                    // record object, where its size is malloced
    int size;       // size of the malloc
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
    friend class RM_Manager;
    friend class RM_FileScan;
public:
    RM_FileHandle ();
    ~RM_FileHandle();

    // Given a RID, return the record
    RC GetRec     (const RID &rid, RM_Record &rec) const;

    RC InsertRec  (const char *pData, RID &rid);       // Insert a new record

    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES);
private:
    bool isValidFH() const;
    RC CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const;
    RC GetFirstZeroBit(char *bitmap, int size, int &location) const;
    RC GetNextOneBit(char *bitmap, int size, int bitnum, int &nextbit) const;
    RC SetBit(char *bitmap, int size, int bitnum);
    RC ResetBit(char *bitmap, int size, int bitnum);
    RC ResetBitmap(char *bitmap, int size);
    int NumBitsToCharSize(int size);
    RM_FileHeader* header;
    bool openedFH;
    bool headerModified;
    PF_FileHandle* pf_fh;

    //only for filescan
    bool isValidFileHeader() const;
    RC GetNextRecord(PageNum &currentPage, SlotNum &currentSlot, RM_Record &temprec) const;
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan {
    static const PageNum INVALID_PAGE_NUM = -1;
    static const SlotNum INVALID_SLOT_NUM = -1;
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    RC OpenScan  (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);               // Get next matching record
    RC CloseScan ();                             // Close the scan

private:
    bool openScan;
    bool (*comparator) (void * value1, void * value2, AttrType attrType, int attrLength);
    const RM_FileHandle* fh;

    ClientHint pinHint;
    AttrType attrType;
    int attrLength;
    int attrOffset;
    void* value;


    PageNum currentPage;    //default:1
    SlotNum currentSlot;    //default:-1
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager    (PF_Manager &pfm);

    RC CreateFile (const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile  (RM_FileHandle &fileHandle);
private:
    PF_Manager *pfm; // reference to program's PF_Manager
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_EOF                 (START_RM_WARN + 0) // end of file
#define RM_BITMAPEOF           (START_RM_WARN + 1) // end of the bitmap
#define RM_LASTWARN            RM_EOF

#define RM_INVALIDFILEHEADER   (START_RM_ERR - 0) // invalid fileheader
#define RM_INVALIDRID          (START_RM_ERR - 1) // invalid RID
#define RM_INVALIDRECORDSIZE   (START_RM_ERR - 2) // invalid recordsize
#define RM_INVALIDFILEHANDLE   (START_RM_ERR - 3) // invalid fileHandle
#define RM_INVALIDCOMPOP       (START_RM_ERR - 4) // invalid CompOp
#define RM_INVALIDPINHINT      (START_RM_ERR - 5) // invalid pinHint
#define RM_INVALIDATTRLENGTH   (START_RM_ERR - 6) // invalid attrLength
#define RM_INVALIDATTRTYPE     (START_RM_ERR - 7) // invalid AttrType
#define RM_RECORDDATAERROR     (START_RM_ERR - 8) // cannot get the recorddata
#define RM_NULLRECPOINTER      (START_RM_ERR - 9) // record's pointer is null
#define RM_NULLFILENAME        (START_RM_ERR - 10) // filename is null
#define RM_RECORDEXISTED       (START_RM_ERR - 11) // the slot already has a record
#define RM_RECORDNOTEXISTED    (START_RM_ERR - 12) // the slot not have a record
#define RM_BITNUMBOUND         (START_RM_ERR - 13) // the bitnum too big( >size )
#define RM_LASTERROR           RM_BITNUMBOUND

#endif