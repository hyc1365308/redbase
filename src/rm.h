#include "redbase.h" 
#include "rm_rid.h"
#include "utils/pagedef.h"
#include "fileio/FileManager.h"
#include "bufmanager/BufPageManager.h"

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
    //rec会返回记录内容（如果调用成功）
	//直接通过rid找到目标位置获取信息存入rec 
    RC InsertRec      (const char *pData, RID &rid);       // Insert a new record,
                                                           //   return record id
    //寻找可以插入的位置 
	//插入
	//把信息保存在rid中 
    RC DeleteRec      (const RID &rid);                    // Delete a record
    //也是通过rid找到位置删除（修改bitmap的值） 
    //更新相关信息
    RC UpdateRec      (const RM_Record &rec);              // Update a record
    //通过rid找到位置然后替换 
    RC ForcePages     (PageNum pageNum = ALL_PAGES) const; // Write dirty page(s)
                                                           //   to disk                                                      
    //调用close 
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
    //避免重复 
    //调用下层的createfile
	//创建文件头
	//
                                                 // Create a new file
    RC DestroyFile (const char *fileName);       // Destroy a file
    //直接删除（要调用下层接口） 
    RC OpenFile    (const char *fileName, RM_FileHandle &fileHandle);
                                                 // Open a file
    //打开文件，读取文件头
	//把文件头的信息传递到filehandle类中保存下来
	//需要传递fm参数 
	// bpm = new BufPageManager(fm);
    RC CloseFile   (RM_FileHandle &fileHandle);  // Close a file
    // 判断头页有没有被修改，如果有就markdirty
	//调用close函数更新全部页
	//再调用bpm.fileManager的closefile 
    
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

