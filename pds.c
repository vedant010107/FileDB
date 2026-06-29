#include "pds.h"

/* ─── Global DB state ───────────────────────────────────────── */
struct DB_Info db_info;   /* zero-initialised by the C runtime */

/* ═══════════════════════════════════════════════════════════════
 *  TABLE-LEVEL FUNCTIONS
 *  (same semantics as PDS 3.0; no changes to signatures)
 * ═══════════════════════════════════════════════════════════════ */

int table_create(char *tname) {
    if (tname == NULL || tname[0] == '\0') return FAILURE;

    char dat[160], ndx[160];
    strcpy(dat, tname); strcat(dat, ".dat");
    strcpy(ndx, tname); strcat(ndx, ".ndx");

    FILE *df = fopen(dat, "wb");
    FILE *nf = fopen(ndx, "wb");
    if (!df || !nf) {
        if (df) fclose(df);
        if (nf) fclose(nf);
        return FAILURE;
    }
    int zero = 0;
    fwrite(&zero, sizeof(int), 1, nf);   /* rec_count = 0 */
    fclose(df);
    fclose(nf);
    return SUCCESS;
}

int table_open(struct TableInfo *tinfo) {
    if (tinfo == NULL || tinfo->tname[0] == '\0') return FAILURE;

    char dat[160], ndx[160];
    strcpy(dat, tinfo->tname); strcat(dat, ".dat");
    strcpy(ndx, tinfo->tname); strcat(ndx, ".ndx");

    FILE *df = fopen(dat, "rb+");
    FILE *nf = fopen(ndx, "rb+");
    if (!df || !nf) {
        if (df) fclose(df);
        if (nf) fclose(nf);
        return FAILURE;
    }

    tinfo->rec_count = 0;
    if (fread(&tinfo->rec_count, sizeof(int), 1, nf) != 1)
        tinfo->rec_count = 0;
    if (tinfo->rec_count > 0)
        fread(tinfo->ndxarray, sizeof(struct Ndx), tinfo->rec_count, nf);

    tinfo->tfile   = df;
    tinfo->ndxfile = NULL;
    fclose(nf);
    return SUCCESS;
}

int table_store(int key, void *recordPtr, struct TableInfo *tinfo) {
    if (!tinfo || !tinfo->tfile) return FAILURE;
    if (tinfo->rec_count >= MAX)  return DB_FULL;

    fseek(tinfo->tfile, 0, SEEK_END);
    int loc = (int)ftell(tinfo->tfile);

    tinfo->ndxarray[tinfo->rec_count] = (struct Ndx){key, loc, 0, -1};
    tinfo->rec_count++;

    fwrite(&key,       sizeof(int),        1, tinfo->tfile);
    fwrite(recordPtr,  tinfo->rec_size,    1, tinfo->tfile);
    return SUCCESS;
}

int table_get(int key, void *recordPtr, struct TableInfo *tinfo) {
    if (!tinfo || !tinfo->tfile) return FAILURE;

    for (int i = 0; i < tinfo->rec_count; i++) {
        if (tinfo->ndxarray[i].key == key) {
            fseek(tinfo->tfile, tinfo->ndxarray[i].loc, SEEK_SET);
            int tmp;
            fread(&tmp,        sizeof(int),     1, tinfo->tfile);
            fread(recordPtr,   tinfo->rec_size, 1, tinfo->tfile);
            return SUCCESS;
        }
    }
    return REC_NOT_FOUND;
}

int table_update(int key, void *recordPtr, struct TableInfo *tinfo) {
    if (!tinfo || !tinfo->tfile) return FAILURE;

    for (int i = 0; i < tinfo->rec_count; i++) {
        if (tinfo->ndxarray[i].key == key) {
            fseek(tinfo->tfile, tinfo->ndxarray[i].loc, SEEK_SET);
            fwrite(&tinfo->ndxarray[i].key, sizeof(int),     1, tinfo->tfile);
            fwrite(recordPtr,               tinfo->rec_size, 1, tinfo->tfile);
            return SUCCESS;
        }
    }
    return REC_NOT_FOUND;
}

int table_delete(int key, struct TableInfo *tinfo) {
    if (!tinfo || !tinfo->tfile) return FAILURE;

    for (int i = 0; i < tinfo->rec_count; i++) {
        if (tinfo->ndxarray[i].key == key) {
            tinfo->ndxarray[i].is_deleted = 1;
            tinfo->ndxarray[i].old_key    = key;
            tinfo->ndxarray[i].key        = -1;
            return SUCCESS;
        }
    }
    return FAILURE;
}

int table_undelete(int key, struct TableInfo *tinfo) {
    if (!tinfo || !tinfo->tfile) return FAILURE;

    for (int i = 0; i < tinfo->rec_count; i++) {
        if (tinfo->ndxarray[i].old_key == key &&
            tinfo->ndxarray[i].is_deleted == 1) {
            tinfo->ndxarray[i].is_deleted = 0;
            tinfo->ndxarray[i].key        = key;
            tinfo->ndxarray[i].old_key    = -1;
            return SUCCESS;
        }
    }
    return REC_NOT_FOUND;
}

int table_close(struct TableInfo *tinfo) {
    if (!tinfo || !tinfo->tfile) return FAILURE;

    char ndx[160];
    strcpy(ndx, tinfo->tname); strcat(ndx, ".ndx");

    FILE *nf = fopen(ndx, "wb");
    if (!nf) {
        fclose(tinfo->tfile);
        tinfo->tfile = NULL;
        return FAILURE;
    }
    fwrite(&tinfo->rec_count, sizeof(int), 1, nf);
    if (tinfo->rec_count > 0)
        fwrite(tinfo->ndxarray, sizeof(struct Ndx), tinfo->rec_count, nf);
    fclose(nf);

    fclose(tinfo->tfile);
    tinfo->tfile = NULL;
    return SUCCESS;
}

/*
 * table_scan – scan all active records; return first whose bytes at
 * [field_offset .. field_offset+field_size) match field_value.
 *
 * field_offset is measured from the START of the stored struct
 * (i.e. offsetof(YourStruct, fieldName)).
 */
int table_scan(struct TableInfo *tinfo,
               int field_offset, int field_size,
               void *field_value, void *result_rec)
{
    if (!tinfo || !tinfo->tfile)   return FAILURE;
    if (!field_value || !result_rec) return FAILURE;
    if (field_offset < 0 || field_size <= 0 ||
        field_offset + field_size > tinfo->rec_size) return FAILURE;

    char *buf = (char *)malloc(tinfo->rec_size);
    if (!buf) return FAILURE;

    for (int i = 0; i < tinfo->rec_count; i++) {
        /* skip logically deleted records */
        if (tinfo->ndxarray[i].is_deleted) continue;

        /* seek past the leading key int to reach the struct bytes */
        fseek(tinfo->tfile,
              tinfo->ndxarray[i].loc + (int)sizeof(int),
              SEEK_SET);
        if (fread(buf, tinfo->rec_size, 1, tinfo->tfile) != 1) continue;

        if (memcmp(buf + field_offset, field_value, field_size) == 0) {
            memcpy(result_rec, buf, tinfo->rec_size);
            free(buf);
            return SUCCESS;
        }
    }
    free(buf);
    return REC_NOT_FOUND;
}

/* ═══════════════════════════════════════════════════════════════
 *  DB-LEVEL FUNCTIONS  (PDS 4.0)
 * ═══════════════════════════════════════════════════════════════ */

/*
 * db_init – free all heap memory and reset db_info to a clean state.
 * Safe to call even when db_info has never been opened.
 */
void db_init(void) {
    if (db_info.tables != NULL) {
        for (int i = 0; i < db_info.num_tables; i++) {
            if (db_info.tables[i] != NULL) {
                /* close the file if still open (safety net) */
                if (db_info.tables[i]->tfile != NULL) {
                    fclose(db_info.tables[i]->tfile);
                    db_info.tables[i]->tfile = NULL;
                }
                free(db_info.tables[i]);
                db_info.tables[i] = NULL;
            }
        }
        free(db_info.tables);
        db_info.tables = NULL;
    }
    if (db_info.rels != NULL) {
        free(db_info.rels);
        db_info.rels = NULL;
    }
    db_info.num_tables = 0;
    db_info.num_rels   = 0;
    db_info.db_status  = DB_CLOSE;
    memset(db_info.dbname, 0, sizeof(db_info.dbname));
}

/* ── Schema I/O ─────────────────────────────────────────────── */

/*
 * db_save_schema – write the current DB structure to <dbname>.sch.
 * Binary layout:
 *   [int num_tables]
 *   for each table: [char[100] tname][int rec_size]
 *   [int num_rels]
 *   for each rel:   [int primary_tbl_idx][int related_tbl_idx]
 */
int db_save_schema(void) {
    if (db_info.dbname[0] == '\0') return FAILURE;

    char sch[160];
    strcpy(sch, db_info.dbname); strcat(sch, ".sch");

    FILE *sf = fopen(sch, "wb");
    if (!sf) return FAILURE;

    fwrite(&db_info.num_tables, sizeof(int), 1, sf);
    for (int i = 0; i < db_info.num_tables; i++) {
        fwrite(db_info.tables[i]->tname,    sizeof(char), 100, sf);
        fwrite(&db_info.tables[i]->rec_size, sizeof(int),  1,  sf);
    }

    fwrite(&db_info.num_rels, sizeof(int), 1, sf);
    for (int i = 0; i < db_info.num_rels; i++) {
        fwrite(&db_info.rels[i].primary_tbl_idx, sizeof(int), 1, sf);
        fwrite(&db_info.rels[i].related_tbl_idx, sizeof(int), 1, sf);
    }

    fclose(sf);
    return SUCCESS;
}

/*
 * db_load_schema – read <dbname>.sch and populate db_info
 * (tables and rels arrays allocated on heap; files NOT opened yet).
 */
int db_load_schema(char *dbname) {
    char sch[160];
    strcpy(sch, dbname); strcat(sch, ".sch");

    FILE *sf = fopen(sch, "rb");
    if (!sf) return FAILURE;

    strcpy(db_info.dbname, dbname);

    /* ── tables ── */
    int num_tables = 0;
    fread(&num_tables, sizeof(int), 1, sf);

    if (num_tables > 0) {
        db_info.tables = (struct TableInfo **)
                         calloc(num_tables, sizeof(struct TableInfo *));
        if (!db_info.tables) { fclose(sf); return FAILURE; }

        for (int i = 0; i < num_tables; i++) {
            db_info.tables[i] = (struct TableInfo *)
                                 calloc(1, sizeof(struct TableInfo));
            if (!db_info.tables[i]) {
                /* rollback */
                for (int j = 0; j < i; j++) free(db_info.tables[j]);
                free(db_info.tables);
                db_info.tables = NULL;
                fclose(sf);
                return FAILURE;
            }
            fread(db_info.tables[i]->tname,    sizeof(char), 100, sf);
            fread(&db_info.tables[i]->rec_size, sizeof(int),  1,  sf);
        }
        db_info.num_tables = num_tables;
    }

    /* ── relationships ── */
    int num_rels = 0;
    fread(&num_rels, sizeof(int), 1, sf);

    if (num_rels > 0) {
        db_info.rels = (struct RelInfo *)
                       malloc(num_rels * sizeof(struct RelInfo));
        if (!db_info.rels) {
            for (int i = 0; i < db_info.num_tables; i++) free(db_info.tables[i]);
            free(db_info.tables);
            db_info.tables     = NULL;
            db_info.num_tables = 0;
            fclose(sf);
            return FAILURE;
        }
        for (int i = 0; i < num_rels; i++) {
            fread(&db_info.rels[i].primary_tbl_idx, sizeof(int), 1, sf);
            fread(&db_info.rels[i].related_tbl_idx, sizeof(int), 1, sf);
        }
        db_info.num_rels = num_rels;
    }

    fclose(sf);
    return SUCCESS;
}

/* ── Database lifecycle ─────────────────────────────────────── */

/*
 * db_create – create a new, empty schema file (<dbname>.sch).
 * No tables are created upfront; use db_add_table after db_open.
 * Legacy testers work because db_open auto-creates a default table
 * when the schema has 0 tables and rec_size > 0.
 */
int db_create(char *dbname) {
    if (!dbname || dbname[0] == '\0') return FAILURE;

    char sch[160];
    strcpy(sch, dbname); strcat(sch, ".sch");

    FILE *sf = fopen(sch, "wb");
    if (!sf) return FAILURE;

    int zero = 0;
    fwrite(&zero, sizeof(int), 1, sf);   /* num_tables = 0 */
    fwrite(&zero, sizeof(int), 1, sf);   /* num_rels   = 0 */
    fclose(sf);
    return SUCCESS;
}

/*
 * db_open – open an existing database.
 *   1. Reads schema from <dbname>.sch.
 *   2. Opens all registered table files.
 *   3. If the schema had 0 tables AND rec_size > 0, auto-creates one
 *      table named <dbname> (backward compat with legacy testers).
 */
int db_open(char *dbname, int rec_size) {
    if (db_info.db_status == DB_OPEN) return FAILURE;
    if (!dbname || dbname[0] == '\0')  return FAILURE;

    if (db_load_schema(dbname) != SUCCESS) return FAILURE;

    /* open all tables that were in the schema */
    for (int i = 0; i < db_info.num_tables; i++) {
        if (table_open(db_info.tables[i]) != SUCCESS) {
            /* roll back already-opened tables */
            for (int j = 0; j < i; j++) table_close(db_info.tables[j]);
            db_init();
            return FAILURE;
        }
    }

    /*
     * Backward-compat path: schema had 0 tables (created by legacy db_create
     * or the new db_create), but caller supplied a rec_size.
     * Auto-add a single table named <dbname>.
     */
    if (db_info.num_tables == 0 && rec_size > 0) {
        db_info.db_status = DB_OPEN;          /* needed so db_add_table can save schema */
        if (db_add_table(dbname, rec_size) != SUCCESS) {
            db_init();
            return FAILURE;
        }
    } else {
        db_info.db_status = DB_OPEN;
    }

    return SUCCESS;
}

/*
 * db_add_table – dynamically add a new table to the open database.
 *   Creates <tname>.dat / <tname>.ndx on disk, allocates a TableInfo
 *   on the heap, opens it, appends to db_info.tables, and saves schema.
 */
int db_add_table(char *tname, int rec_size) {
    if (!tname || tname[0] == '\0' || rec_size <= 0) return FAILURE;
    if (db_info.dbname[0] == '\0') return FAILURE;   /* no DB open */

    if (table_create(tname) != SUCCESS) return FAILURE;

    struct TableInfo *t = (struct TableInfo *)calloc(1, sizeof(struct TableInfo));
    if (!t) return FAILURE;

    strncpy(t->tname, tname, 99);
    t->rec_size = rec_size;

    if (table_open(t) != SUCCESS) { free(t); return FAILURE; }

    /* grow the tables pointer array */
    struct TableInfo **newt = (struct TableInfo **)realloc(
        db_info.tables,
        (db_info.num_tables + 1) * sizeof(struct TableInfo *));
    if (!newt) { table_close(t); free(t); return FAILURE; }

    db_info.tables = newt;
    db_info.tables[db_info.num_tables] = t;
    db_info.num_tables++;

    return db_save_schema();
}

/*
 * db_add_rel – register a relationship between two tables.
 *   Creates / truncates the binary relationship file
 *   <primary_tname>.<related_tname> and records the pair in the schema.
 */
int db_add_rel(int primary_tbl_idx, int related_tbl_idx) {
    if (db_info.db_status != DB_OPEN) return FAILURE;
    if (primary_tbl_idx  < 0 || primary_tbl_idx  >= db_info.num_tables) return FAILURE;
    if (related_tbl_idx < 0 || related_tbl_idx >= db_info.num_tables) return FAILURE;

    /* create / truncate the relationship file */
    char rel[260];
    sprintf(rel, "%s.%s",
            db_info.tables[primary_tbl_idx]->tname,
            db_info.tables[related_tbl_idx]->tname);
    FILE *rf = fopen(rel, "wb");
    if (!rf) return FAILURE;
    fclose(rf);

    /* grow the rels array */
    struct RelInfo *newr = (struct RelInfo *)realloc(
        db_info.rels,
        (db_info.num_rels + 1) * sizeof(struct RelInfo));
    if (!newr) return FAILURE;

    db_info.rels = newr;
    db_info.rels[db_info.num_rels].primary_tbl_idx = primary_tbl_idx;
    db_info.rels[db_info.num_rels].related_tbl_idx = related_tbl_idx;
    db_info.num_rels++;

    return db_save_schema();
}

/*
 * db_close – flush all index files, save schema, free heap memory.
 */
int db_close(void) {
    if (db_info.db_status != DB_OPEN) return FAILURE;

    int overall = SUCCESS;
    for (int i = 0; i < db_info.num_tables; i++) {
        if (table_close(db_info.tables[i]) != SUCCESS)
            overall = FAILURE;
    }

    /* persist schema (tname / rec_size still valid; tfile is now NULL) */
    db_save_schema();

    db_init();   /* frees heap and zeros all fields */
    return overall;
}

/* ── Relationship functions ─────────────────────────────────── */

/*
 * put_rel – append a (primary_key, related_key) pair to the
 * relationship file <primary_tname>.<related_tname>.
 */
int put_rel(int primary_tbl_idx, int primary_key,
            int related_tbl_idx, int related_key)
{
    if (db_info.db_status != DB_OPEN) return FAILURE;
    if (primary_tbl_idx  < 0 || primary_tbl_idx  >= db_info.num_tables) return FAILURE;
    if (related_tbl_idx < 0 || related_tbl_idx >= db_info.num_tables) return FAILURE;

    char rel[260];
    sprintf(rel, "%s.%s",
            db_info.tables[primary_tbl_idx]->tname,
            db_info.tables[related_tbl_idx]->tname);

    FILE *rf = fopen(rel, "ab");
    if (!rf) return FAILURE;

    fwrite(&primary_key, sizeof(int), 1, rf);
    fwrite(&related_key, sizeof(int), 1, rf);
    fclose(rf);
    return SUCCESS;
}

/*
 * get_rel – scan the relationship file for search_key, then retrieve
 * the matching record from the related table via table_get.
 */
int get_rel(int primary_tbl_idx, int search_key,
            int related_tbl_idx, void *result_rec)
{
    if (db_info.db_status != DB_OPEN) return FAILURE;
    if (primary_tbl_idx  < 0 || primary_tbl_idx  >= db_info.num_tables) return FAILURE;
    if (related_tbl_idx < 0 || related_tbl_idx >= db_info.num_tables) return FAILURE;

    char rel[260];
    sprintf(rel, "%s.%s",
            db_info.tables[primary_tbl_idx]->tname,
            db_info.tables[related_tbl_idx]->tname);

    FILE *rf = fopen(rel, "rb");
    if (!rf) return REC_NOT_FOUND;

    int pk, rk, found = 0;
    while (fread(&pk, sizeof(int), 1, rf) == 1) {
        if (fread(&rk, sizeof(int), 1, rf) != 1) break;
        if (pk == search_key) { found = 1; break; }
    }
    fclose(rf);

    if (!found) return REC_NOT_FOUND;
    return table_get(rk, result_rec, db_info.tables[related_tbl_idx]);
}