# PDS 4.0 — Persistent Data Store

A file-based database engine I built from scratch in C, starting from a bare-bones single-table
key-value store and gradually evolving it into something that can handle multiple tables,
relationships between them, and searching by any field — not just the key.

The whole point was to  understand what actually happens under the hood when you store and retrieve
data — no SQL,instead using raw binary files and index arrays.

---

## What it does

- **Schema persistence** — when you close the database, the structure (table names, sizes, relationships) gets written to a `.sch` file. Open it again and everything is exactly where you left it.
- **Multiple tables** —  Add as many tables as you need at runtime.
- **Relationships** — link any two tables together, store the connection, and retrieve the related record in one call.
- **Non-key search** — `table_scan` walks through the table and matches any field you point it at.
- **Full CRUD** — store, get, update, delete, and even undelete records if you change your mind.

---

## Files

```
pds.h                    the public API — all structs and function signatures
pds.c                    where everything actually happens

pds_tester.c             official Hospital DB tester
pds_tester-3.c           official Course DB tester
public_testcase.in       test input for the Hospital tester
public_testcase-3.in     test input for the Course tester

pds4_master_tester.c     my own 271-test edge case suite — exits with ACCEPTED
course.h                 Course struct
coursedb.h               helper definitions for the Course DB
```

---

## How to build and run

```bash
# Run the official Hospital test
gcc pds.c pds_tester.c -o pds_tester
./pds_tester public_testcase.in

# Run the official Course test
gcc pds.c pds_tester-3.c -o pds_tester_3
./pds_tester_3 public_testcase-3.in

# Run the full edge-case suite
gcc pds.c pds4_master_tester.c -o pds4_master_tester
./pds4_master_tester
# last line should say: ACCEPTED
```

---

## Quick API reference

```c
// Open / close a database
db_create("university");                        // creates university.sch
db_open("university", 0);                       // loads schema, opens all tables
db_close();                                     // flushes index, saves schema

// Add tables and relationships
db_add_table("Student", sizeof(struct Student)); // tables[0]
db_add_table("Course",  sizeof(struct Course));  // tables[1]
db_add_rel(0, 1);                               // Student → Course

// Standard record operations
table_store(key, &rec, db_info.tables[0]);
table_get(key, &rec, db_info.tables[0]);
table_update(key, &rec, db_info.tables[0]);
table_delete(key, db_info.tables[0]);
table_undelete(key, db_info.tables[0]);

// Find by non-key field (e.g. find Student by name)
char name[50] = "Alice";
table_scan(db_info.tables[0],
           offsetof(struct Student, name), 50,
           name, &result);

// Link records and follow the link
put_rel(0, student_key, 1, course_key);
get_rel(0, student_key, 1, &course_result);
```

**Return values:** `SUCCESS (0)` · `REC_NOT_FOUND (1)` · `FAILURE (-1)` · `DB_FULL (2)`

---

## How the storage actually works

Each table is backed by two files:

- **`.dat`** — raw binary records written sequentially. Every entry is `[4-byte key][record bytes]`.
- **`.ndx`** — an array of `{key, file_offset, is_deleted, old_key}` entries. Loaded into memory on open, flushed back on close. This is what makes lookups fast — no scanning the data file.

Relationships are stored in a third binary file named `PrimaryTable.RelatedTable`, with pairs of `[primary_key, related_key]` written one after another.

The schema file (`.sch`) stores how many tables exist, their names and record sizes, and the list of registered relationships — enough to fully reconstruct the database state on the next open.

---

## Notes

- `db_open` is backward-compatible — if you pass a `rec_size > 0` and the schema has no tables yet (fresh database), it auto-creates one table named after the database. Old testers work without changes.
- Deleting a record marks it as deleted in the index (soft delete) rather than removing it. `table_undelete` can bring it back. `table_scan` automatically skips deleted records.
- The index array is capped at 1000 entries per table (`MAX`). Trying to store the 1001st returns `DB_FULL`.