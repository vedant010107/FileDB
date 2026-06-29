#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "course.h" // Modify this if your filename is different
#include "pds.h"

#define TREPORT(a1,a2) printf("Status: %s - %s\n\n", a1, a2); fflush(stdout);

typedef struct Course Course;

void process_line( char *test_case );

// Acessing struct fields
int get_course_id(Course* course)
{
	return course->num; // Modify according to your struct
}
void set_course_id(Course* course, int id)
{
	course->num = id; // Modify according to your struct
}
char* get_course_name(Course* course)
{
	return course->name; // Modify according to your struct
}
void set_course_name(Course* course, char* name)
{
	strcpy(course->name, name); // Modify according to your struct
}
char* get_course_instructor(Course* course)
{
	return course->instructor; // Modify according to your struct
}
void set_course_instructor(Course* course, char* instructor)
{
	strcpy(course->instructor, instructor); // Modify according to your struct
}

// PDS function calls
int pds_create(char* repo_name)
{
	return db_create(repo_name); // Replace with your function 
}

int pds_open(char* repo_name, int rec_size)
{
	return db_open(repo_name, rec_size); // Replace with your function 
}

int add_course(int key, Course* course)
{
	return table_store(key, course, db_info.tables[0]); // Replace with your function 
}

int search_course(int key, Course* course)
{
	return table_get(key, course, db_info.tables[0]); // Replace with your function 
}

int update_course(int key, Course* course)
{
	return table_update(key, course, db_info.tables[0]); // Replace with your function 
}

int delete_course(int key)
{
	return table_delete(key, db_info.tables[0]); // Replace with your function 
}

int undelete_course(int key)
{
	return table_undelete(key, db_info.tables[0]); // Replace with your function 
}

int pds_close()
{
	return db_close(); // Replace with your function 
}


// Main
int main(int argc, char *argv[])
{
	FILE *cfptr;
	char test_case[500];
	db_init();
	if( argc != 2 ){
		fprintf(stderr, "Usage: %s testcasefile\n", argv[0]);
		exit(1);
	}

	cfptr = (FILE *) fopen(argv[1], "r");
	while(fgets(test_case, sizeof(test_case)-1, cfptr)){
		// printf("line:%s",test_case);
		if( !strcmp(test_case,"\n") || !strcmp(test_case,"") )
			continue;
		process_line( test_case );
	}
}

void process_line( char *test_case )
{
	char dbname[30];
	char command[15], param1[15], param2[15], info[1024];
	int course_id, status, rec_size, expected_status;
	struct Course testCourse;

	rec_size = sizeof(struct Course);

	sscanf(test_case, "%s%s%s", command, param1, param2);
	printf("Test case: %s", test_case); fflush(stdout);

	if( !strcmp(command,"CREATE") ){
		strcpy(dbname, param1);
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		status = pds_create(dbname);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_create returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"OPEN") ){
		strcpy(dbname, param1);
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		status = pds_open(dbname, rec_size) ;
	
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_open returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"STORE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &course_id);
		set_course_id(&testCourse, course_id);

		char course_name[50];
		char instructor[50];
		sscanf(test_case, "%s%s%s%s%s", command, param1, param2, course_name, instructor);

		set_course_name(&testCourse, course_name);
		set_course_instructor(&testCourse, instructor);
		
		status = add_course(course_id, &testCourse);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"add_course returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"SEARCH") ){
		char expected_course_name[50];
		char expected_instructor[50];
		if( !strcmp(param2,"0") )
		{
			sscanf(test_case, "%s%s%s%s%s", command, param1, param2, expected_course_name, expected_instructor);
			expected_status = SUCCESS;
		}
		else
			expected_status = REC_NOT_FOUND;

		sscanf(param1, "%d", &course_id);
		set_course_id(&testCourse, -1);
		
		status = search_course(course_id, &testCourse);
		
		if( status != expected_status ){
			sprintf(info,"search key: %d; Got status %d",course_id, status);
			TREPORT("FAIL", info);
		}
		else{
			// Check if the retrieved values match
			if( expected_status == SUCCESS ){
				if (get_course_id(&testCourse) == course_id && 
					strcmp(get_course_name(&testCourse), expected_course_name) == 0 &&
					strcmp(get_course_instructor(&testCourse), expected_instructor) == 0){
						TREPORT("PASS", "");
				}
				else{
					sprintf(info,"Course data not matching... Expected:{%d,%s,%s} Got:{%d,%s,%s}\n",
						course_id, expected_course_name, expected_instructor, 
						get_course_id(&testCourse), get_course_name(&testCourse), get_course_instructor(&testCourse)
					);
					TREPORT("FAIL", info);
				}
			}
			else
				TREPORT("PASS", "");
		}
	}
	else if( !strcmp(command,"UPDATE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &course_id);
		set_course_id(&testCourse, course_id);

		char course_name[50];
		char instructor[50];

		sscanf(test_case, "%s%s%s%s%s", command, param1, param2, course_name, instructor);

		set_course_name(&testCourse, course_name);
		set_course_instructor(&testCourse, instructor);
		
		status = update_course(course_id, &testCourse);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"update_course returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if(!strcmp(command,"DELETE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &course_id);

		status = delete_course(course_id);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"delete_course returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if(!strcmp(command,"UNDELETE") ){
		if( !strcmp(param2,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		sscanf(param1, "%d", &course_id);

		status = undelete_course(course_id);
		
		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"undelete_course returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
	else if( !strcmp(command,"CLOSE") ){
		if( !strcmp(param1,"0") )
			expected_status = SUCCESS;
		else
			expected_status = FAILURE;

		status = pds_close();

		if( status == expected_status ){
			TREPORT("PASS", "");
		}
		else{
			sprintf(info,"pds_close returned status %d",status);
			TREPORT("FAIL", info);
		}
	}
}


