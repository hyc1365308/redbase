#include "redbase.h" 
#include "rm_rid.h"
#include "pf.h"

struct RM_FileHeader{
    int recordSize;           // record size in file
    int numRecordsPerPage;    // calculated max # of recs per page
    int numPages;             // number of pages
    PageNum firstFreePage;    // pointer to first free object

    int bitmapOffset;         // location in bytes of where the bitmap starts
                              // in the page headers
    int bitmapSize;           // size of bitmaps in the page headers
};

struct RM_PageHeader{
	int nextFreePage;
	int numRecords;
};

class RM_Record {
    static const int INVALID_RECORD_SIZE = -1;
    friend class RM_FileHandle;
public:
    RM_Record ();
    ~RM_Record();
    RM_Record& operator= (const RM_Record &record);

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;

    // Sets the record with an RID, data contents, and its size
    RC SetRecord (RID rec_rid, char *recData, int size);
private:
    RID rid;        // record RID
    char * data;    // pointer to record data. This is stored in the
                    // record object, where its size is malloced
    int size;       // size of the malloc
};


class RM_FileHandle{
    friend class RM_Manager;
public:
    RM_FileHandle  ();                                  // Constructor
    ~RM_FileHandle ();                                  // Destructor
    RC GetRec         (const RID &rid, RM_Record &rec) const;// Get a record 
    RC InsertRec      (const char *pData, RID &rid);       // Insert a new record,
                                                           //   return record id
    RC DeleteRec      (const RID &rid);                    // Delete a record
    RC UpdateRec      (const RM_Record &rec);              // Update a record
    RC ForcePages     (PageNum pageNum = ALL_PAGES) const; // Write dirty page(s)
                                                           //   to disk                                                      
    //µ÷ÓÃclose 
private:
    bool isValidFH() const;
    RC CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const;
    RC GetFirstZeroBit(char *bitmap, int size, int &location);
    RC SetBit(char *bitmap, int size, int bitnum);
    RC ResetBit(char *bitmap, int size, int bitnum);
    RC ResetBitmap(char *bitmap, int size);
    int NumBitsToCharSize(int size);
    int fileID;
    RM_FileHeader header;
    bool openedFH;
    bool headerModified;
    BufPageManager *bpm;
    FileManager *fm;
};

class RM_Manager{
public:
       RM_Manager  (FileManager* fm){
       	this->fm = fm;
        this->bpm = new BufPageManager(fm);
    	};            // Constructor
       ~RM_Manager (){
        delete bpm;
    }; // Destructor
    RC CreateFile  (const char *fileName, int recordSize);  
                                                 // Create a new file
    RC DestroyFile (const char *fileName);       // Destroy a file
    RC OpenFile    (const char *fileName, RM_FileHandle &fileHandle);
                                                 // Open a file
    RC CloseFile   (RM_FileHandle &fileHandle);  // Close a file
    
private:
	BufPageManager* bpm;
    FileManager* fm; // reference to program's PF_Manager
};

class RM_FileScan {
public:
       RM_FileScan  ();                                // Constructor
       ~RM_FileScan ();                                // Destructor
    RC OpenScan     (const RM_FileHandle &fileHandle,  // Initialize file scan
                     AttrType      attrType,
                     int           attrLength,
                     int           attrOffset,
                     CompOp        compOp,
                     void          *value,
                     ClientHint    pinHint = NO_HINT);
    RC GetNextRec   (RM_Record &rec);                  // Get next matching record
    RC CloseScan    ();                                // Terminate file scan
private:
	void readFileHeader(const RM_FileHandle &fileHandle);
	
	bool (*comparator) (void * , void *, AttrType, int);
	bool openScan;
};

