//
// File:        ix_test.cc
// Description: Test IX component
//

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include "redbase.h"
#include "pf.h"
#include "ix.h"

using namespace std;

//
// Defines
//
const char * FILENAME =  "testrel";       // test file name
#define STRLEN      29               // length of string in testrec
#define PROG_UNIT   50               // how frequently to give progress
                                      //   reports when adding lots of recs
#define FEW_RECS  140                // number of records added in

//
// Computes the offset of a field in a record (should be in <stddef.h>)
//
#ifndef offsetof
#       define offsetof(type, field)   ((size_t)&(((type *)0) -> field))
#endif

//
// Structure of the records we will be using for the tests
//
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

//
// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
IX_Manager ixm(pfm);

//
// Function declarations
//
RC Test1(void);
RC Test2(void);

void PrintErrorAll(RC rc);
RC CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength);
RC DestroyIndex(const char *fileName, int indexNo);
RC OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &ixh);
RC CloseIndex(const char *fileName, int indexNo, IX_IndexHandle &ixh);
RC InsertEntry(IX_IndexHandle &fh, char *record, RID &rid);
RC DeleteEntry(IX_IndexHandle &fh, RID &rid);
//RC GetNextEntryScan(IX_IndexScan &fs, RM_Record &rec);


//
// Array of pointers to the test functions
//
#define NUM_TESTS       2               // number of tests
int (*tests[])() =                      // RC doesn't work on some compilers
{
    Test1
};

//
// main
//
int main(int argc, char *argv[])
{
    RC   rc;
    char *progName = argv[0];   // since we will be changing argv
    int  testNum;

    // Write out initial starting message
    cerr.flush();
    cout.flush();
    cout << "Starting IX component test.\n";
    cout.flush();

    // Delete files from last time
    unlink(FILENAME);

    // If no argument given, do all tests
    if (argc == 1) {
        for (testNum = 0; testNum < NUM_TESTS; testNum++)
            if ((rc = (tests[testNum])())) {

                // Print the error and exit
                PrintErrorAll(rc);
                return (1);
            }
    }
    else {

        // Otherwise, perform specific tests
        while (*++argv != NULL) {

            // Make sure it's a number
            if (sscanf(*argv, "%d", &testNum) != 1) {
                cerr << progName << ": " << *argv << " is not a number\n";
                continue;
            }

            // Make sure it's in range
            if (testNum < 1 || testNum > NUM_TESTS) {
                cerr << "Valid test numbers are between 1 and " << NUM_TESTS << "\n";
                continue;
            }

            // Perform the test
            if ((rc = (tests[testNum - 1])())) {

                // Print the error and exit
                PrintErrorAll(rc);
                return (1);
            }
        }
    }

    // Write ending message and exit
    cout << "Ending IX component test.\n\n";
    exit(0);
    return (0);
}


////////////////////////////////////////////////////////////////////
// The following functions may be useful in tests that you devise //
////////////////////////////////////////////////////////////////////

//
// LsFile
//
// Desc: list the filename's directory entry
//
void LsFile(const char *fileName)
{
    char command[80];
    sprintf(command, "ls -l %s", fileName);
    printf("doing \"%s\"\n", command);
    system(command);
}

//
// PrintRecord
//
// Desc: Print the TestRec record components
//
void PrintRecord(TestRec &recBuf)
{
    printf("[%s, %d, %f]\n", recBuf.str, recBuf.num, recBuf.r);
}

////////////////////////////////////////////////////////////////////////
// The following functions are wrappers for some of the RM component  //
// methods.  They give you an opportunity to add debugging statements //
// and/or set breakpoints when testing these methods.                 //
////////////////////////////////////////////////////////////////////////


//
// CreateIndex
//
// Desc: call IX_Manager::CreateIndex
//
RC CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength)
{
    printf("\ncreating %s_%d index\n", fileName, indexNo);
    return (ixm.CreateIndex(fileName, indexNo, attrType, attrLength));
}

//
// DestroyIndex
//
// Desc: call IX_Manager::DestroyIndex
//
RC DestroyIndex(const char *fileName, int indexNo)
{
    printf("\ndestroying %s_%d index\n", fileName, indexNo);
    return (ixm.DestroyIndex(fileName, indexNo));
}


//
// OpenIndex
//
// Desc: call IX_Manager::OpenIndex
//
RC OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &ixh)
{
    printf("\nopening %s_%d index\n", fileName, indexNo);
    return (ixm.OpenIndex(fileName, indexNo, ixh));
}

//
// CloseIndex
//
// Desc: call IX_Manager::CloseIndex
//
RC CloseIndex(const char *fileName, int indexNo, IX_IndexHandle &ixh)
{
    if (fileName != NULL)
        printf("\nClosing %s_%d index\n", fileName, indexNo);
    return (ixm.CloseIndex(ixh));
}


//
// InsertEntry
//
// Desc: call IX_IndexHandle::InsertEntry
//
RC InsertEntry(IX_IndexHandle &fh, char *record, RID &rid)
{
    printf("Inserting Entry...\n");
    return (fh.InsertEntry(record, rid));
}

//
// DeleteEntry
//
// Desc: call RM_IndexHandle::DeleteEntry
//
RC DeleteEntry(IX_IndexHandle &fh, char *record, RID &rid)
{
    printf("Deleting Entry...\n");
    return (fh.DeleteEntry(record, rid));
}

//
// GetNextEntryScan
//
// Desc: call RM_IndexScan::GetNextEntry
//
RC GetNextEntryScan(IX_IndexScan &fs, RID &rid)
{
    printf("Getting next Entry...\n");
    return (fs.GetNextEntry(rid));
}


/////////////////////////////////////////////////////////////////////
// Sample test functions follow.                                   //
/////////////////////////////////////////////////////////////////////

//
// Test1 tests simple creation, opening, closing, and deletion of indexs
//
RC Test1(void)
{
    RC            rc;
    IX_IndexHandle ixh;

    printf("test1 starting ****************\n");

    if (
        (rc = CreateIndex(FILENAME, 2, INT, sizeof(int)))
        || (rc = OpenIndex(FILENAME, 2, ixh))
        || (rc = CloseIndex(FILENAME, 2, ixh))
      )
        return (rc);

    LsFile(FILENAME);

    if ((rc = DestroyIndex(FILENAME, 2)))
        return (rc);

    printf("\ntest1 done ********************\n");
    return (0);
}

void PrintErrorAll(RC rc){
    PF_PrintError(rc);
    IX_PrintError(rc);
}