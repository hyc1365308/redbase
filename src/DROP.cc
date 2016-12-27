#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "redbase.h"
#include <climits>


using namespace std;


//
// main
//
int main(int argc, char *argv[])
{
    char *dbname;
    char command[255] = "rm -r ";
    RC rc;


    if (argc != 3) {
        cerr << "Usage: " << argv[0] << "DATABASE DBname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[2];
    if(strlen(argv[2]) > (sizeof(command)-strlen(command) - 1)){
        cerr << argv[2] << " length exceeds maximum allowed, cannot delete database\n";
        exit(1);
    }

    // Create a subdirectory for the database
    int error = system (strcat(command,dbname));

    if(error){
        cerr << "system call to destroy directory exited with error code " << error << endl;
        exit(1);
    }

    return(0);
}
