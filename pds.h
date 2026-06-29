#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS       0
#define FAILURE      -1
#define REC_NOT_FOUND 1

#define DB_OPEN  0
#define DB_CLOSE 1
#define DB_FULL  2

/* Max records per table (index array size) */
#define MAX 1000

/* ─── Data Structures ─────────────────────────────────────── */

struct Ndx {
    int key;
    int loc;
    int is_deleted;
    int old_key;
};

struct TableInfo {
    char tname[100];
    int  rec_size;
    FILE *tfile;
    FILE *ndxfile;              /* kept for API compat; not used at runtime */
    struct Ndx ndxarray[MAX];
    int  rec_count;
};

/* One relationship entry: which two table indices are linked */
struct RelInfo {
    int primary_tbl_idx;
    int related_tbl_idx;
};

/*
 * DB_Info – PDS 4.0
 *   tables : heap-allocated array of TableInfo pointers (unlimited tables)
 *   rels   : heap-allocated array of RelInfo           (unlimited rels)
 *   dbname : base name of the open database (used for .sch / .dat paths)
 */
struct DB_Info {
    char              dbname[150];
    struct TableInfo **tables;
    int               num_tables;
    struct RelInfo   *rels;
    int               num_rels;
    int               db_status;
};

extern struct DB_Info db_info;

/* ─── Table-level functions (unchanged signatures) ─────────── */
int table_create(char *tname);
int table_open(struct TableInfo *tinfo);
int table_store(int key, void *recordPtr, struct TableInfo *tinfo);
int table_get(int key, void *recordPtr, struct TableInfo *tinfo);
int table_update(int key, void *recordPtr, struct TableInfo *tinfo);
int table_delete(int key, struct TableInfo *tinfo);
int table_undelete(int key, struct TableInfo *tinfo);
int table_close(struct TableInfo *tinfo);

/*
 * table_scan – non-key field retrieval (PDS 4.0)
 *   field_offset : byte offset of the field within the stored struct
 *   field_size   : byte size of the field
 *   field_value  : pointer to the value to match
 *   result_rec   : output buffer (rec_size bytes), filled on SUCCESS
 * Returns SUCCESS on first match, REC_NOT_FOUND if none.
 */
int table_scan(struct TableInfo *tinfo,
               int field_offset, int field_size,
               void *field_value, void *result_rec);

/* ─── DB wrapper functions ──────────────────────────────────── */
int  db_create(char *dbname);
int  db_open(char *dbname, int rec_size);   /* rec_size used as fallback for legacy testers */
int  db_add_table(char *tname, int rec_size);
int  db_add_rel(int primary_tbl_idx, int related_tbl_idx);
int  db_save_schema(void);
int  db_load_schema(char *dbname);
int  db_close(void);
void db_init(void);

/* ─── Relationship functions (PDS 4.0 – explicit table indices) */
int put_rel(int primary_tbl_idx, int primary_key,
            int related_tbl_idx, int related_key);
int get_rel(int primary_tbl_idx, int search_key,
            int related_tbl_idx, void *result_rec);