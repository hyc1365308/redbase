#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "pf.h"
#include "pf_internal.h"
#include "pf_hashtable.h"

using namespace std;
#define FILE1 "file1"

int main()
{
   RC rc;

   // Write out initial starting message
   cerr.flush();
   cout.flush();
   cout << "Starting PF layer test.\n";
   cout.flush();

   unlink(FILE1);

   PF_Manager pfm;
   PF_FileHandle fh1;
   PF_PageHandle ph1;
   PF_PageHandle ph2;
   PageNum pageNum;
   // Do tests
   if ((rc = pfm.CreateFile(FILE1)) ||
         (rc = pfm.OpenFile(FILE1, fh1)) ||
         (rc = fh1.AllocatePage(ph1)) ||
         (rc = fh1.GetFirstPage(ph1)) ||
         (rc = ph1.GetPageNum(pageNum)) ||
         (rc = fh1.GetNextPage(pageNum, ph2))) {
   		PF_PrintError(rc);
   		return 1;
   }
   cout << "PageNum = "<< pageNum << endl;

   // Write ending message and exit
   cout << "Ending PF layer test.\n\n";

   return (0);
}