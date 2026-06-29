#include <stdio.h>

// #define SUCCESS 0
// #define FAILURE -1
// #define REC_NOT_FOUND 1

// #define DB_OPEN 0
// #define DB_CLOSE 1
// #define DB_FULL 2

// // check for MAX at the time of store
// #define MAX 1000


struct Course{
    int num;
    char name[50];
    char instructor[50];
};

// struct CourseNdx{
//     int key;
//     int loc;
//     int is_deleted;
//     int old_key;
// };

// struct CoursedbInfo{
//     FILE *dbfile;
//     char dbname[50];
//     int status;
//     struct CourseNdx ndx[MAX];
//     int num_records;
// };



// extern struct CoursedbInfo cdb_info;

// struct CoursedbInfo cdb_info;



// // fopen in wb mode and then close
// int create_coursedb(char *dbname);

// // fopen in rb+ mode
// int open_coursedb(char *dbname);

// // int store_coursedb(struct Course *c);
// // fseek to end then fwrite
// int store_coursedb(int key, struct Course *recordPtr);

// // // fseek to beginning iterate over each record until matching record is found or EOF
// // // 0 - success, -1 - not found
// // int get_coursedb(struct Course *coutput, int course_num);

// // read course by key using index
// int read_course(int key, struct Course *recordPtr);


// // close the file, fclose
// int close_coursedb();

// void print_course(struct Course *c);

// // reset the global variable
// void coursedb_init();

// // update record by finding the record and then fseek back and fwrite
// int update_coursedb(int key, struct Course *recordPtr);

// // return failure under the following conditions
// // status is already open
// // db name is empty
// // fopen returns null
// int delete_coursedb(int key);
// int undelete_coursedb(int key);