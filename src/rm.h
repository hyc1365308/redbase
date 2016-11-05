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
    //rec�᷵�ؼ�¼���ݣ�������óɹ���
	//ֱ��ͨ��rid�ҵ�Ŀ��λ�û�ȡ��Ϣ����rec 
    RC InsertRec      (const char *pData, RID &rid);       // Insert a new record,
                                                           //   return record id
    //Ѱ�ҿ��Բ����λ�� 
	//����
	//����Ϣ������rid�� 
    RC DeleteRec      (const RID &rid);                    // Delete a record
    //Ҳ��ͨ��rid�ҵ�λ��ɾ�����޸�bitmap��ֵ�� 
    //���������Ϣ
    RC UpdateRec      (const RM_Record &rec);              // Update a record
    //ͨ��rid�ҵ�λ��Ȼ���滻 
    RC ForcePages     (PageNum pageNum = ALL_PAGES) const; // Write dirty page(s)
                                                           //   to disk                                                      
    //����close 
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
    //�����ظ� 
    //�����²��createfile
	//�����ļ�ͷ
	//
                                                 // Create a new file
    RC DestroyFile (const char *fileName);       // Destroy a file
    //ֱ��ɾ����Ҫ�����²�ӿڣ� 
    RC OpenFile    (const char *fileName, RM_FileHandle &fileHandle);
                                                 // Open a file
    //���ļ�����ȡ�ļ�ͷ
	//���ļ�ͷ����Ϣ���ݵ�filehandle���б�������
	//��Ҫ����fm���� 
	// bpm = new BufPageManager(fm);
    RC CloseFile   (RM_FileHandle &fileHandle);  // Close a file
    // �ж�ͷҳ��û�б��޸ģ�����о�markdirty
	//����close��������ȫ��ҳ
	//�ٵ���bpm.fileManager��closefile 
    
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

