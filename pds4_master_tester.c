/*
 * pds4_master_tester.c  –  PDS 4.0 Master Test Suite
 *
 * Compile: gcc pds.c pds4_master_tester.c -o pds4_master_tester
 * Run    : ./pds4_master_tester   (or  .\pds4_master_tester on Windows)
 *
 * Expected final line: ACCEPTED
 *
 * Sections
 * ────────
 *  A.  Official Hospital DB  (mirrors public_testcase.in)
 *  B.  Official Course DB    (mirrors public_testcase-3.in with UNDELETE)
 *  C.  NULL / Invalid parameter tests
 *  D.  DB lifecycle order tests
 *  E.  Key value edge cases
 *  F.  CRUD correctness
 *  G.  Delete / Undelete interactions
 *  H.  DB_FULL boundary (MAX = 1000)
 *  I.  table_scan comprehensive
 *  J.  Relationship edge cases
 *  K.  Schema persistence
 *  L.  Multi-table / unlimited tables
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "pds.h"

/* ═══════════════════════════════════════════════════════════
 *  Test infrastructure
 * ═══════════════════════════════════════════════════════════ */
static int g_total = 0, g_pass = 0, g_fail = 0;

#define CHECK(label, cond) do {                                         \
    g_total++;                                                           \
    if (cond) { g_pass++; }                                             \
    else {                                                               \
        g_fail++;                                                        \
        printf("  FAIL: %s\n", (label));                                \
    }                                                                    \
} while(0)

#define CHECK_EQ(label, got, want) do {                                 \
    g_total++;                                                           \
    if ((int)(got) == (int)(want)) { g_pass++; }                        \
    else {                                                               \
        g_fail++;                                                        \
        printf("  FAIL: %s  [expected=%d got=%d]\n",                   \
               (label), (int)(want), (int)(got));                        \
    }                                                                    \
} while(0)

#define SECTION(s) printf("\n[%s]\n", s)

/* Reset db_info to a clean slate */
static void reset(void) { db_init(); }

/* Create + open a fresh single-table DB */
static void fresh_db(const char *name, int rec_size) {
    db_init();
    db_create((char *)name);
    db_open((char *)name, rec_size);
}

/* ═══════════════════════════════════════════════════════════
 *  Structs used in tests
 * ═══════════════════════════════════════════════════════════ */

/* ── Official test structs ─────────────────────────────── */
typedef struct {
    int  hospital_id;
    char name[100];
    char address[200];
    char email[50];
} Hospital;

typedef struct {
    int  num;
    char name[50];
    char instructor[50];
} Course;

/* ── Edge-case struct (compact, known offsets) ──────────── */
/*    id    : offset= 0, size=4
 *    name  : offset= 4, size=20
 *    score : offset=24, size=4
 *    sizeof: 28                                            */
typedef struct {
    int  id;
    char name[20];
    int  score;
} Rec;

static Rec make_rec(int id, const char *name, int score) {
    Rec r;
    r.id = id;  r.score = score;
    memset(r.name, 0, sizeof(r.name));
    strncpy(r.name, name, sizeof(r.name) - 1);
    return r;
}

/* fully-zeroed name comparison buffer */
static void set_name(char buf[20], const char *s) {
    memset(buf, 0, 20);
    strncpy(buf, s, 19);
}

/* ═══════════════════════════════════════════════════════════
 *  A. Official Hospital DB  (mirrors public_testcase.in)
 * ═══════════════════════════════════════════════════════════ */
static void test_official_hospital(void) {
    SECTION("A. Official Hospital DB Test");

    /* ── helpers ──────────────────────────────────────── */
#define H_STORE(key, n, addr, em) do {              \
    Hospital _h; _h.hospital_id = (key);            \
    strcpy(_h.name, n);                             \
    strcpy(_h.address, addr);                       \
    strcpy(_h.email, em);                           \
    CHECK_EQ("STORE " #key, table_store((key), &_h, db_info.tables[0]), SUCCESS); \
} while(0)

#define H_SEARCH_OK(key, n, addr, em) do {          \
    Hospital _h; memset(&_h, 0, sizeof(_h));        \
    int _s = table_get((key), &_h, db_info.tables[0]); \
    CHECK_EQ("SEARCH " #key " found",  _s, SUCCESS); \
    CHECK("SEARCH " #key " name",  strcmp(_h.name,    n)    == 0); \
    CHECK("SEARCH " #key " addr",  strcmp(_h.address, addr) == 0); \
    CHECK("SEARCH " #key " email", strcmp(_h.email,   em)   == 0); \
} while(0)

#define H_SEARCH_MISS(key) do {                     \
    Hospital _h; memset(&_h, 0, sizeof(_h));        \
    CHECK_EQ("SEARCH " #key " miss", \
        table_get((key), &_h, db_info.tables[0]), REC_NOT_FOUND); \
} while(0)

#define H_UPDATE(key, n, addr, em) do {             \
    Hospital _h; _h.hospital_id = (key);            \
    strcpy(_h.name, n);                             \
    strcpy(_h.address, addr);                       \
    strcpy(_h.email, em);                           \
    CHECK_EQ("UPDATE " #key, table_update((key), &_h, db_info.tables[0]), SUCCESS); \
} while(0)

#define H_DELETE(key, want) \
    CHECK_EQ("DELETE " #key, table_delete((key), db_info.tables[0]), (want))

    /* ── Session 1 ────────────────────────────────────── */
    reset();
    CHECK_EQ("CREATE newdemo", db_create("newdemo"),        SUCCESS);
    CHECK_EQ("OPEN newdemo",   db_open("newdemo", sizeof(Hospital)), SUCCESS);

    H_STORE(10000, "Name-of-10000", "Address-of-10000", "Email-of-10000");
    H_STORE(10001, "Name-of-10001", "Address-of-10001", "Email-of-10001");
    H_STORE(10002, "Name-of-10002", "Address-of-10002", "Email-of-10002");

    H_SEARCH_OK(10000, "Name-of-10000", "Address-of-10000", "Email-of-10000");
    H_SEARCH_OK(10000, "Name-of-10000", "Address-of-10000", "Email-of-10000");
    H_SEARCH_MISS(90000);

    H_UPDATE(10001, "Updated-Name-of-10001", "Updated-Address-of-10001", "Updated-Email-of-10001");
    H_SEARCH_OK(10001, "Updated-Name-of-10001", "Updated-Address-of-10001", "Updated-Email-of-10001");

    CHECK_EQ("CLOSE 1", db_close(), SUCCESS);

    /* ── Session 2 ────────────────────────────────────── */
    db_init();
    CHECK_EQ("OPEN session 2", db_open("newdemo", 0), SUCCESS);
    H_STORE(10003, "Name-of-10003", "Address-of-10003", "Email-of-10003");
    H_DELETE(10002, SUCCESS);
    CHECK_EQ("CLOSE 2", db_close(), SUCCESS);

    /* ── Session 3 ────────────────────────────────────── */
    db_init();
    CHECK_EQ("OPEN session 3", db_open("newdemo", 0), SUCCESS);
    H_SEARCH_MISS(10002);
    H_DELETE(10004, FAILURE);
    H_SEARCH_OK(10003, "Name-of-10003", "Address-of-10003", "Email-of-10003");
    H_SEARCH_OK(10000, "Name-of-10000", "Address-of-10000", "Email-of-10000");
    H_SEARCH_OK(10001, "Updated-Name-of-10001", "Updated-Address-of-10001", "Updated-Email-of-10001");
    H_SEARCH_MISS(90000);
    CHECK_EQ("CLOSE 3", db_close(), SUCCESS);
}

/* ═══════════════════════════════════════════════════════════
 *  B. Official Course DB  (mirrors public_testcase-3.in)
 * ═══════════════════════════════════════════════════════════ */
static void test_official_course(void) {
    SECTION("B. Official Course DB Test");

#define C_STORE(key, n, instr) do {                 \
    Course _c; _c.num = (key);                      \
    strcpy(_c.name, n); strcpy(_c.instructor, instr);\
    CHECK_EQ("C-STORE " #key, table_store((key), &_c, db_info.tables[0]), SUCCESS); \
} while(0)

#define C_SEARCH_OK(key, n, instr) do {             \
    Course _c; memset(&_c, 0, sizeof(_c));          \
    int _s = table_get((key), &_c, db_info.tables[0]); \
    CHECK_EQ("C-SEARCH " #key " found", _s, SUCCESS); \
    CHECK("C-SEARCH " #key " name",  strcmp(_c.name, n)       == 0); \
    CHECK("C-SEARCH " #key " instr", strcmp(_c.instructor, instr) == 0); \
} while(0)

#define C_SEARCH_MISS(key) do {                     \
    Course _c; memset(&_c, 0, sizeof(_c));          \
    CHECK_EQ("C-SEARCH " #key " miss", \
        table_get((key), &_c, db_info.tables[0]), REC_NOT_FOUND); \
} while(0)

#define C_UPDATE(key, n, instr) do {                \
    Course _c; _c.num = (key);                      \
    strcpy(_c.name, n); strcpy(_c.instructor, instr);\
    CHECK_EQ("C-UPDATE " #key, table_update((key), &_c, db_info.tables[0]), SUCCESS); \
} while(0)

#define C_DELETE(key, want)   CHECK_EQ("C-DELETE " #key,   table_delete((key),   db_info.tables[0]), (want))
#define C_UNDELETE(key, want) CHECK_EQ("C-UNDELETE " #key, table_undelete((key), db_info.tables[0]), (want))

    /* ── Session 1 ────────────────────────────────────── */
    reset();
    CHECK_EQ("CREATE newdemo", db_create("newdemo"),              SUCCESS);
    CHECK_EQ("OPEN newdemo",   db_open("newdemo", sizeof(Course)), SUCCESS);

    C_STORE(10000, "Name-of-10000", "Instructor-of-10000");
    C_STORE(10001, "Name-of-10001", "Instructor-of-10001");
    C_STORE(10002, "Name-of-10002", "Instructor-of-10002");

    C_SEARCH_OK(10000, "Name-of-10000", "Instructor-of-10000");
    C_SEARCH_OK(10000, "Name-of-10000", "Instructor-of-10000");
    C_SEARCH_MISS(90000);

    C_UPDATE(10001, "Updated-Name-of-10000", "Updated-Instructor-of-10000");
    C_SEARCH_OK(10001, "Updated-Name-of-10000", "Updated-Instructor-of-10000");

    CHECK_EQ("CLOSE 1", db_close(), SUCCESS);

    /* ── Session 2 ────────────────────────────────────── */
    db_init();
    CHECK_EQ("OPEN session 2", db_open("newdemo", 0), SUCCESS);
    C_STORE(10003, "Name-of-10003", "Instructor-of-10003");
    C_DELETE(10002, SUCCESS);
    C_UNDELETE(10002, SUCCESS);
    CHECK_EQ("CLOSE 2", db_close(), SUCCESS);

    /* ── Session 3 ────────────────────────────────────── */
    db_init();
    CHECK_EQ("OPEN session 3", db_open("newdemo", 0), SUCCESS);
    C_DELETE(10004, FAILURE);
    C_SEARCH_OK(10003, "Name-of-10003", "Instructor-of-10003");
    C_SEARCH_OK(10000, "Name-of-10000", "Instructor-of-10000");
    C_SEARCH_OK(10001, "Updated-Name-of-10000", "Updated-Instructor-of-10000");
    C_SEARCH_MISS(90000);
    CHECK_EQ("CLOSE 3", db_close(), SUCCESS);
}

/* ═══════════════════════════════════════════════════════════
 *  C. NULL / Invalid Parameter Tests
 * ═══════════════════════════════════════════════════════════ */
static void test_null_params(void) {
    SECTION("C. NULL / Invalid Parameter Tests");
    int s;

    reset();
    CHECK_EQ("db_create(NULL)",       db_create(NULL), FAILURE);
    CHECK_EQ("db_create(\"\")",        db_create(""),   FAILURE);
    CHECK_EQ("db_open(NULL,100)",     db_open(NULL, 100), FAILURE);
    CHECK_EQ("db_open(\"\",100)",      db_open("", 100),  FAILURE);
    CHECK_EQ("db_open(ghost)",        db_open("__ghost__xyz__", 100), FAILURE);

    Rec r = make_rec(1, "x", 1);
    CHECK_EQ("table_store(NULL tinfo)",    table_store(1, &r, NULL),    FAILURE);
    CHECK_EQ("table_get(NULL tinfo)",      table_get(1, &r, NULL),      FAILURE);
    CHECK_EQ("table_update(NULL tinfo)",   table_update(1, &r, NULL),   FAILURE);
    CHECK_EQ("table_delete(NULL tinfo)",   table_delete(1, NULL),       FAILURE);
    CHECK_EQ("table_undelete(NULL tinfo)", table_undelete(1, NULL),     FAILURE);
    CHECK_EQ("table_scan(NULL tinfo)",     table_scan(NULL,0,4,&r,&r),  FAILURE);

    /* table_scan field param validation */
    fresh_db("np_db", sizeof(Rec));
    struct TableInfo *t = db_info.tables[0];
    Rec out = {0};  char fv[20] = "test";

    CHECK_EQ("table_scan offset<0",      table_scan(t,-1,4,fv,&out),                        FAILURE);
    CHECK_EQ("table_scan size==0",       table_scan(t, 0,0,fv,&out),                        FAILURE);
    CHECK_EQ("table_scan size<0",        table_scan(t, 0,-1,fv,&out),                       FAILURE);
    CHECK_EQ("table_scan offset+size>rs",table_scan(t,(int)sizeof(Rec)-1,2,fv,&out),        FAILURE);
    CHECK_EQ("table_scan offset==rs",    table_scan(t,(int)sizeof(Rec),1,fv,&out),           FAILURE);
    CHECK_EQ("table_scan NULL fval",     table_scan(t, 0,4,NULL,&out),                      FAILURE);
    CHECK_EQ("table_scan NULL result",   table_scan(t, 0,4,fv,NULL),                        FAILURE);

    /* db_add_table param validation */
    CHECK_EQ("db_add_table(NULL)",      db_add_table(NULL,sizeof(Rec)), FAILURE);
    CHECK_EQ("db_add_table(\"\")",       db_add_table("",  sizeof(Rec)), FAILURE);
    CHECK_EQ("db_add_table(size=0)",    db_add_table("ok", 0),           FAILURE);
    CHECK_EQ("db_add_table(size=-1)",   db_add_table("ok",-1),           FAILURE);

    /* put_rel / get_rel invalid indices (DB open with 1 table at index 0) */
    Rec ro = {0};
    CHECK_EQ("put_rel(primary=-1)",  put_rel(-1,1, 0,2), FAILURE);
    CHECK_EQ("put_rel(related=-1)",  put_rel( 0,1,-1,2), FAILURE);
    CHECK_EQ("put_rel(primary=99)",  put_rel(99,1, 0,2), FAILURE);
    CHECK_EQ("put_rel(related=99)",  put_rel( 0,1,99,2), FAILURE);
    CHECK_EQ("get_rel(primary=-1)",  get_rel(-1,1, 0,&ro), FAILURE);
    CHECK_EQ("get_rel(related=-1)",  get_rel( 0,1,-1,&ro), FAILURE);
    CHECK_EQ("get_rel(primary=99)",  get_rel(99,1, 0,&ro), FAILURE);
    CHECK_EQ("get_rel(related=99)",  get_rel( 0,1,99,&ro), FAILURE);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  D. DB Lifecycle Order Tests
 * ═══════════════════════════════════════════════════════════ */
static void test_lifecycle(void) {
    SECTION("D. DB Lifecycle Order Tests");
    int s;

    reset();
    CHECK_EQ("close before open",       db_close(), FAILURE);
    CHECK_EQ("open non-existent",       db_open("__no_db__", 100), FAILURE);

    db_create("lc_db");
    db_open("lc_db", sizeof(Rec));
    CHECK_EQ("double open",             db_open("lc_db", sizeof(Rec)), FAILURE);
    db_close();
    CHECK_EQ("double close",            db_close(), FAILURE);

    /* ops with DB closed */
    reset();
    Rec d = {0};
    CHECK_EQ("put_rel DB closed",       put_rel(0,1,0,2),     FAILURE);
    CHECK_EQ("get_rel DB closed",       get_rel(0,1,0,&d),    FAILURE);
    CHECK_EQ("db_add_rel DB closed",    db_add_rel(0,1),      FAILURE);

    /* create → store → close → reopen → data intact */
    db_create("lc2");
    db_open("lc2", sizeof(Rec));
    Rec r = make_rec(42, "hello", 99);
    table_store(42, &r, db_info.tables[0]);
    db_close();

    db_init();
    CHECK_EQ("reopen existing",         db_open("lc2", 0), SUCCESS);
    Rec got = {0};
    CHECK_EQ("data after reopen",       table_get(42, &got, db_info.tables[0]), SUCCESS);
    CHECK_EQ("correct score",           got.score, 99);
    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  E. Key Value Edge Cases
 * ═══════════════════════════════════════════════════════════ */
static void test_key_edges(void) {
    SECTION("E. Key Value Edge Cases");

    fresh_db("key_db", sizeof(Rec));
    struct TableInfo *t = db_info.tables[0];
    Rec out = {0};

    /* key = 0 */
    Rec r0 = make_rec(0, "zero", 0);
    CHECK_EQ("store key=0",             table_store(0, &r0, t), SUCCESS);
    CHECK_EQ("get   key=0",             table_get(0, &out, t),  SUCCESS);
    CHECK_EQ("key=0 id",                out.id, 0);

    /* key = -2 (valid non-sentinel negative) */
    Rec rm = make_rec(-2, "neg", 55);
    CHECK_EQ("store key=-2",            table_store(-2, &rm, t), SUCCESS);
    CHECK_EQ("get   key=-2",            table_get(-2, &out, t),  SUCCESS);
    CHECK_EQ("key=-2 id",               out.id, -2);

    /* key = INT_MAX */
    Rec rl = make_rec(2147483647, "max", 77);
    CHECK_EQ("store key=INT_MAX",       table_store(2147483647, &rl, t), SUCCESS);
    CHECK_EQ("get   key=INT_MAX",       table_get(2147483647, &out, t),  SUCCESS);
    CHECK_EQ("INT_MAX id",              out.id, 2147483647);

    /* non-existent key */
    CHECK_EQ("get non-existent",        table_get(99999, &out, t), REC_NOT_FOUND);

    /* duplicate key: get returns first stored */
    Rec rd1 = make_rec(500, "first", 10);
    Rec rd2 = make_rec(500, "second", 20);
    table_store(500, &rd1, t);
    table_store(500, &rd2, t);
    table_get(500, &out, t);
    CHECK_EQ("dup key: first wins",     out.score, 10);

    /* update on duplicate: updates first occurrence */
    Rec upd = make_rec(500, "updated", 99);
    CHECK_EQ("update dup",              table_update(500, &upd, t), SUCCESS);
    table_get(500, &out, t);
    CHECK_EQ("dup after update",        out.score, 99);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  F. CRUD Correctness
 * ═══════════════════════════════════════════════════════════ */
static void test_crud(void) {
    SECTION("F. CRUD Correctness");

    fresh_db("crud_db", sizeof(Rec));
    struct TableInfo *t = db_info.tables[0];
    Rec out = {0};

    Rec r1 = make_rec(10, "Alice",  95);
    Rec r2 = make_rec(20, "Bob",    80);
    Rec r3 = make_rec(30, "Carol",  70);
    table_store(10, &r1, t);
    table_store(20, &r2, t);
    table_store(30, &r3, t);

    table_get(20, &out, t);
    CHECK_EQ("get id",          out.id,    20);
    CHECK_EQ("get score",       out.score, 80);
    CHECK("get name",           strcmp(out.name, "Bob") == 0);

    Rec upd = make_rec(20, "Bobby", 88);
    CHECK_EQ("update ok",       table_update(20, &upd, t), SUCCESS);
    table_get(20, &out, t);
    CHECK_EQ("after upd score", out.score, 88);
    CHECK("after upd name",     strcmp(out.name, "Bobby") == 0);

    CHECK_EQ("update missing",  table_update(999, &upd, t), REC_NOT_FOUND);

    CHECK_EQ("delete ok",       table_delete(30, t), SUCCESS);
    CHECK_EQ("get deleted",     table_get(30, &out, t), REC_NOT_FOUND);
    CHECK_EQ("sibling ok",      table_get(10, &out, t), SUCCESS);

    CHECK_EQ("delete again",    table_delete(30, t), FAILURE);
    CHECK_EQ("delete missing",  table_delete(99999, t), FAILURE);
    CHECK_EQ("update deleted",  table_update(30, &upd, t), REC_NOT_FOUND);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  G. Delete / Undelete Interactions
 * ═══════════════════════════════════════════════════════════ */
static void test_del_undel(void) {
    SECTION("G. Delete / Undelete Interactions");

    fresh_db("du_db", sizeof(Rec));
    struct TableInfo *t = db_info.tables[0];
    Rec g = {0};

    Rec r1 = make_rec(1, "One",   10);
    Rec r2 = make_rec(2, "Two",   20);
    Rec r3 = make_rec(3, "Three", 30);
    table_store(1, &r1, t);
    table_store(2, &r2, t);
    table_store(3, &r3, t);

    CHECK_EQ("undel non-deleted",  table_undelete(1, t),   REC_NOT_FOUND);
    CHECK_EQ("undel non-existent", table_undelete(999, t), REC_NOT_FOUND);

    table_delete(1, t);
    CHECK_EQ("get after del",      table_get(1, &g, t),    REC_NOT_FOUND);
    CHECK_EQ("undelete ok",        table_undelete(1, t),   SUCCESS);
    CHECK_EQ("get after undel",    table_get(1, &g, t),    SUCCESS);
    CHECK_EQ("undel id correct",   g.id, 1);
    CHECK_EQ("undel twice",        table_undelete(1, t),   REC_NOT_FOUND);

    /* delete all, then selectively undelete one */
    table_delete(1, t);
    table_delete(2, t);
    table_delete(3, t);
    CHECK_EQ("all gone: 1",        table_get(1, &g, t), REC_NOT_FOUND);
    CHECK_EQ("all gone: 2",        table_get(2, &g, t), REC_NOT_FOUND);
    CHECK_EQ("all gone: 3",        table_get(3, &g, t), REC_NOT_FOUND);
    table_undelete(2, t);
    CHECK_EQ("after undel2: 1",    table_get(1, &g, t), REC_NOT_FOUND);
    CHECK_EQ("after undel2: 2",    table_get(2, &g, t), SUCCESS);
    CHECK_EQ("after undel2: 3",    table_get(3, &g, t), REC_NOT_FOUND);

    /* delete → update fails → undelete → update succeeds */
    table_delete(2, t);
    Rec upd = make_rec(2, "TwoNew", 200);
    CHECK_EQ("upd while del",      table_update(2, &upd, t), REC_NOT_FOUND);
    table_undelete(2, t);
    CHECK_EQ("upd after undel",    table_update(2, &upd, t), SUCCESS);
    table_get(2, &g, t);
    CHECK_EQ("score after upd",    g.score, 200);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  H. DB_FULL Boundary  (MAX = 1000)
 * ═══════════════════════════════════════════════════════════ */
static void test_db_full(void) {
    SECTION("H. DB_FULL Boundary (MAX=1000)");

    fresh_db("full_db", sizeof(Rec));
    struct TableInfo *t = db_info.tables[0];
    Rec r;

    /* store MAX-1 records */
    int ok = 1;
    for (int i = 0; i < MAX - 1; i++) {
        r = make_rec(i, "fill", i);
        if (table_store(i, &r, t) != SUCCESS) { ok = 0; break; }
    }
    CHECK("store MAX-1 records", ok);

    /* boundary: MAX-th must succeed */
    r = make_rec(MAX - 1, "boundary", MAX - 1);
    CHECK_EQ("store MAX-th",        table_store(MAX - 1, &r, t), SUCCESS);

    /* MAX+1 and MAX+2 must return DB_FULL */
    r = make_rec(MAX,     "over",  0);
    CHECK_EQ("store MAX+1 → FULL", table_store(MAX,     &r, t), DB_FULL);
    r = make_rec(MAX + 1, "over2", 0);
    CHECK_EQ("store MAX+2 → FULL", table_store(MAX + 1, &r, t), DB_FULL);

    /* existing records still readable */
    Rec got = {0};
    CHECK_EQ("read key 0 after full",     table_get(0,       &got, t), SUCCESS);
    CHECK_EQ("read key MAX-1 after full", table_get(MAX - 1, &got, t), SUCCESS);
    CHECK_EQ("key MAX-1 score correct",   got.score, MAX - 1);

    /* delete does NOT free a slot (rec_count doesn't shrink) */
    table_delete(0, t);
    r = make_rec(MAX + 5, "after_del", 0);
    CHECK_EQ("store after delete → FULL", table_store(MAX + 5, &r, t), DB_FULL);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  I. table_scan Comprehensive
 * ═══════════════════════════════════════════════════════════ */
static void test_scan(void) {
    SECTION("I. table_scan Comprehensive");

    fresh_db("scan_db", sizeof(Rec));
    struct TableInfo *t = db_info.tables[0];
    Rec out = {0};

    Rec r1 = make_rec(1, "Alpha", 10);
    Rec r2 = make_rec(2, "Beta",  20);
    Rec r3 = make_rec(3, "Gamma", 30);
    Rec r4 = make_rec(4, "Alpha", 40);   /* name duplicated from r1 */
    table_store(1, &r1, t);
    table_store(2, &r2, t);
    table_store(3, &r3, t);
    table_store(4, &r4, t);

    /* helper: zero-padded name target */
    char tn[20];

    /* scan by name */
    set_name(tn, "Beta");
    memset(&out, 0, sizeof(out));
    CHECK_EQ("scan 'Beta' found",        table_scan(t, 4, 20, tn, &out), SUCCESS);
    CHECK_EQ("scan 'Beta' id",           out.id, 2);

    /* duplicate name → first record returned */
    set_name(tn, "Alpha");
    memset(&out, 0, sizeof(out));
    CHECK_EQ("scan dup 'Alpha' found",   table_scan(t, 4, 20, tn, &out), SUCCESS);
    CHECK_EQ("scan dup: first id=1",     out.id, 1);

    /* scan by score (offset=24, size=4) */
    int ts = 30;
    memset(&out, 0, sizeof(out));
    CHECK_EQ("scan score=30 found",      table_scan(t, 24, 4, &ts, &out), SUCCESS);
    CHECK_EQ("scan score=30 id",         out.id, 3);

    /* scan by id field at offset 0 */
    int ti = 2;
    memset(&out, 0, sizeof(out));
    CHECK_EQ("scan id=2 found",          table_scan(t, 0, 4, &ti, &out), SUCCESS);
    CHECK_EQ("scan id=2 correct",        out.id, 2);

    /* exact boundary: offset+size == rec_size */
    int tb = 40;
    memset(&out, 0, sizeof(out));
    CHECK_EQ("scan at exact boundary",   table_scan(t, 24, 4, &tb, &out), SUCCESS);  /* 24+4=28=sizeof(Rec) */
    CHECK_EQ("boundary result id=4",     out.id, 4);

    /* non-existent values */
    int tmiss = 9999;
    CHECK_EQ("scan missing score",       table_scan(t, 24, 4, &tmiss, &out), REC_NOT_FOUND);
    set_name(tn, "NOSUCH");
    CHECK_EQ("scan missing name",        table_scan(t, 4, 20, tn, &out), REC_NOT_FOUND);

    /* deleted record is skipped */
    table_delete(2, t);
    set_name(tn, "Beta");
    CHECK_EQ("scan deleted → miss",      table_scan(t, 4, 20, tn, &out), REC_NOT_FOUND);

    /* undelete → found again */
    table_undelete(2, t);
    memset(&out, 0, sizeof(out));
    set_name(tn, "Beta");
    CHECK_EQ("scan after undel found",   table_scan(t, 4, 20, tn, &out), SUCCESS);
    CHECK_EQ("scan after undel score",   out.score, 20);

    /* scan after update */
    Rec upd = make_rec(2, "BetaNew", 25);
    table_update(2, &upd, t);
    set_name(tn, "Beta");
    CHECK_EQ("scan old name gone",       table_scan(t, 4, 20, tn, &out), REC_NOT_FOUND);
    memset(&out, 0, sizeof(out));
    set_name(tn, "BetaNew");
    CHECK_EQ("scan new name found",      table_scan(t, 4, 20, tn, &out), SUCCESS);
    CHECK_EQ("scan new score",           out.score, 25);

    /* empty table */
    db_close();
    fresh_db("scan_empty", sizeof(Rec));
    struct TableInfo *te = db_info.tables[0];
    set_name(tn, "anything");
    CHECK_EQ("scan empty table",         table_scan(te, 4, 20, tn, &out), REC_NOT_FOUND);

    /* all-deleted table */
    Rec rx = make_rec(99, "lone", 1);
    table_store(99, &rx, te);
    table_delete(99, te);
    set_name(tn, "lone");
    CHECK_EQ("scan all-deleted table",   table_scan(te, 4, 20, tn, &out), REC_NOT_FOUND);

    /* full record integrity */
    Rec ry = make_rec(77, "FullScan", 123);
    table_store(77, &ry, te);
    memset(&out, 0, sizeof(out));
    set_name(tn, "FullScan");
    CHECK_EQ("full record scan",         table_scan(te, 4, 20, tn, &out), SUCCESS);
    CHECK_EQ("full: id",                 out.id,    77);
    CHECK_EQ("full: score",              out.score, 123);
    CHECK("full: name",                  strcmp(out.name, "FullScan") == 0);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  J. Relationship Edge Cases
 * ═══════════════════════════════════════════════════════════ */
static void test_relationships(void) {
    SECTION("J. Relationship Edge Cases");
    int s;

    reset();
    db_create("rel_db");
    db_open("rel_db", 0);
    db_add_table("RelA", sizeof(Rec));   /* tables[0] */
    db_add_table("RelB", sizeof(Rec));   /* tables[1] */
    db_add_rel(0, 1);

    Rec a1 = make_rec(100, "A1", 1);
    Rec b1 = make_rec(200, "B1", 2);
    Rec b2 = make_rec(201, "B2", 3);
    table_store(100, &a1, db_info.tables[0]);
    table_store(200, &b1, db_info.tables[1]);
    table_store(201, &b2, db_info.tables[1]);

    /* basic put/get */
    CHECK_EQ("put_rel A→B",             put_rel(0, 100, 1, 200), SUCCESS);
    Rec got = {0};
    CHECK_EQ("get_rel A→B",             get_rel(0, 100, 1, &got), SUCCESS);
    CHECK_EQ("get_rel id",              got.id, 200);

    /* key not in rel file */
    CHECK_EQ("get_rel miss",            get_rel(0, 999, 1, &got), REC_NOT_FOUND);

    /* second mapping same primary key – get returns FIRST */
    put_rel(0, 100, 1, 201);
    memset(&got, 0, sizeof(got));
    get_rel(0, 100, 1, &got);
    CHECK_EQ("two mappings: first wins", got.id, 200);

    /* linked record deleted → get_rel returns REC_NOT_FOUND */
    table_delete(200, db_info.tables[1]);
    memset(&got, 0, sizeof(got));
    CHECK_EQ("get_rel → del'd linked",  get_rel(0, 100, 1, &got), REC_NOT_FOUND);

    /* undelete → works again */
    table_undelete(200, db_info.tables[1]);
    memset(&got, 0, sizeof(got));
    CHECK_EQ("get_rel after undel",     get_rel(0, 100, 1, &got), SUCCESS);

    /* put_rel without db_add_rel (file created on-the-fly) */
    db_add_table("RelC", sizeof(Rec));  /* tables[2] */
    Rec c1 = make_rec(300, "C1", 5);
    table_store(300, &c1, db_info.tables[2]);
    CHECK_EQ("put_rel no db_add_rel",   put_rel(0, 100, 2, 300), SUCCESS);
    memset(&got, 0, sizeof(got));
    CHECK_EQ("get_rel no db_add_rel",   get_rel(0, 100, 2, &got), SUCCESS);
    CHECK_EQ("ad-hoc rel id",           got.id, 300);

    /* self-relationship (table → itself) */
    db_add_rel(0, 0);
    Rec a2 = make_rec(101, "A2", 7);
    table_store(101, &a2, db_info.tables[0]);
    put_rel(0, 100, 0, 101);
    memset(&got, 0, sizeof(got));
    CHECK_EQ("self-rel get",            get_rel(0, 100, 0, &got), SUCCESS);
    CHECK_EQ("self-rel id",             got.id, 101);

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  K. Schema Persistence
 * ═══════════════════════════════════════════════════════════ */
static void test_persistence(void) {
    SECTION("K. Schema Persistence");
    int s;

    /* build a 3-table DB with 2 relationships */
    reset();
    db_create("pers_db");
    db_open("pers_db", 0);
    db_add_table("PA", sizeof(Rec));   /* [0] */
    db_add_table("PB", sizeof(Rec));   /* [1] */
    db_add_table("PC", sizeof(Rec));   /* [2] */
    db_add_rel(0, 1);
    db_add_rel(1, 2);

    Rec a = make_rec(10, "RecA", 1);
    Rec b = make_rec(20, "RecB", 2);
    Rec c = make_rec(30, "RecC", 3);
    table_store(10, &a, db_info.tables[0]);
    table_store(20, &b, db_info.tables[1]);
    table_store(30, &c, db_info.tables[2]);
    put_rel(0, 10, 1, 20);
    put_rel(1, 20, 2, 30);
    CHECK_EQ("close",               db_close(), SUCCESS);

    /* reopen 1 */
    db_init();
    CHECK_EQ("reopen 1",            db_open("pers_db", 0), SUCCESS);
    CHECK_EQ("3 tables",            db_info.num_tables, 3);
    CHECK_EQ("2 rels",              db_info.num_rels,   2);

    Rec ga = {0}, gb = {0}, gc = {0};
    CHECK_EQ("PA key 10",           table_get(10, &ga, db_info.tables[0]), SUCCESS);
    CHECK_EQ("PA score",            ga.score, 1);
    CHECK_EQ("PB key 20",           table_get(20, &gb, db_info.tables[1]), SUCCESS);
    CHECK_EQ("PC key 30",           table_get(30, &gc, db_info.tables[2]), SUCCESS);

    Rec got = {0};
    CHECK_EQ("get_rel A→B",         get_rel(0, 10, 1, &got), SUCCESS);
    CHECK_EQ("get_rel A→B id",      got.id, 20);
    memset(&got, 0, sizeof(got));
    CHECK_EQ("get_rel B→C",         get_rel(1, 20, 2, &got), SUCCESS);
    CHECK_EQ("get_rel B→C id",      got.id, 30);

    /* second session: add data → close */
    Rec a2 = make_rec(11, "RecA2", 99);
    table_store(11, &a2, db_info.tables[0]);
    db_close();

    /* reopen 2: new data readable */
    db_init();
    db_open("pers_db", 0);
    Rec ga2 = {0};
    CHECK_EQ("new data session 3",  table_get(11, &ga2, db_info.tables[0]), SUCCESS);
    CHECK_EQ("new data score",      ga2.score, 99);

    /* delete persists across sessions */
    table_delete(10, db_info.tables[0]);
    db_close();
    db_init();
    db_open("pers_db", 0);
    Rec gdel = {0};
    CHECK_EQ("del persists",        table_get(10, &gdel, db_info.tables[0]), REC_NOT_FOUND);

    /* undelete persists */
    table_undelete(10, db_info.tables[0]);
    db_close();
    db_init();
    db_open("pers_db", 0);
    Rec gundel = {0};
    CHECK_EQ("undel persists",      table_get(10, &gundel, db_info.tables[0]), SUCCESS);
    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  L. Multi-Table / Unlimited Tables
 * ═══════════════════════════════════════════════════════════ */
static void test_multi_table(void) {
    SECTION("L. Multi-Table / Unlimited Tables");
    int s;
    const int N = 6;

    reset();
    db_create("multi_db");
    db_open("multi_db", 0);

    /* add N tables */
    char tname[20];
    for (int i = 0; i < N; i++) {
        sprintf(tname, "MT%d", i);
        CHECK_EQ("add table", db_add_table(tname, sizeof(Rec)), SUCCESS);
    }
    CHECK_EQ("num_tables == N", db_info.num_tables, N);

    /* store one record per table */
    for (int i = 0; i < N; i++) {
        Rec r = make_rec(i * 100, "item", i * 10);
        CHECK_EQ("store in table[i]", table_store(i * 100, &r, db_info.tables[i]), SUCCESS);
    }

    /* get from each */
    for (int i = 0; i < N; i++) {
        Rec out = {0};
        CHECK_EQ("get from table[i]", table_get(i * 100, &out, db_info.tables[i]), SUCCESS);
        CHECK_EQ("correct id",        out.id, i * 100);
    }

    /* add N-1 relationships (adjacent chain) */
    for (int i = 0; i < N - 1; i++) {
        CHECK_EQ("add_rel chain", db_add_rel(i, i + 1), SUCCESS);
    }
    CHECK_EQ("num_rels == N-1", db_info.num_rels, N - 1);

    /* link + traverse chain */
    for (int i = 0; i < N - 1; i++) {
        put_rel(i, i * 100, i + 1, (i + 1) * 100);
    }
    for (int i = 0; i < N - 1; i++) {
        Rec got = {0};
        CHECK_EQ("get_rel chain", get_rel(i, i * 100, i + 1, &got), SUCCESS);
        CHECK_EQ("chain link id", got.id, (i + 1) * 100);
    }

    /* scan correct/wrong table */
    char tn[20]; set_name(tn, "item");
    Rec so = {0};
    CHECK_EQ("scan correct table",  table_scan(db_info.tables[2], 4, 20, tn, &so), SUCCESS);
    CHECK_EQ("scan correct id",     so.id, 200);

    set_name(tn, "MT2Item");   /* name doesn't exist; item is the name */
    CHECK_EQ("scan wrong table",    table_scan(db_info.tables[0], 0, 4, (void*)"\x64\x00\x00\x00", &so), REC_NOT_FOUND);

    /* close + reopen: schema persists */
    db_close();
    db_init();
    CHECK_EQ("multi reopen",        db_open("multi_db", 0), SUCCESS);
    CHECK_EQ("N tables restored",   db_info.num_tables, N);
    CHECK_EQ("N-1 rels restored",   db_info.num_rels,   N - 1);

    for (int i = 0; i < N; i++) {
        Rec out = {0};
        CHECK_EQ("data after multi reopen", table_get(i * 100, &out, db_info.tables[i]), SUCCESS);
    }

    db_close();
}

/* ═══════════════════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════════════════ */
int main(void) {
    printf("PDS 4.0 Master Test Suite\n");
    printf("─────────────────────────────────────────────────\n");

    test_official_hospital();
    test_official_course();
    test_null_params();
    test_lifecycle();
    test_key_edges();
    test_crud();
    test_del_undel();
    test_db_full();
    test_scan();
    test_relationships();
    test_persistence();
    test_multi_table();

    printf("\n─────────────────────────────────────────────────\n");
    printf("Total  : %d\n", g_total);
    printf("Passed : %d\n", g_pass);
    printf("Failed : %d\n", g_fail);
    printf("─────────────────────────────────────────────────\n");

    if (g_fail == 0) {
        printf("\nACCEPTED\n\n");
        return 0;
    } else {
        printf("\nWRONG ANSWER  (%d test(s) failed)\n\n", g_fail);
        return 1;
    }
}
