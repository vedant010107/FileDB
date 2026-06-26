# FileDB - # DBMS PDS 4.0

This project is a layered file-based database system written in C. It extends the earlier PDS versions with table-level abstractions, related-record handling, schema persistence, and flexible retrieval support.

## Features

- Table-level database operations built on top of the base record storage layer
- Database wrappers for create, open, and close operations
- Related-record support for querying linked data
- Schema persistence in a binary `.sch` file
- Unlimited tables
- Unlimited relationships
- Retrieval based on non-key fields
- Store, search, update, delete, and undelete records

## Project Files

- `pds.c` - implementation of the database functions
- `pds.h` - public header for the database API
- `pds_tester.c` - tester for the base record type
- `pds_tester-3.c` - tester for the course-based record type
- `coursedb.h` - record definition used by the extended tester
- `public_testcase.in` - sample testcase file for the base tester
- `public_testcase-3.in` - sample testcase file for the extended tester

## Generated Files

The project creates these files while running:

- `<dbname>.dat` - binary data file
- `<dbname>.ndx` - binary index file
- `<dbname>.sch` - binary schema file for database metadata

## Build

Use GCC to compile the required tester with the library source.

### Base tester

```bash
gcc pds.c pds_tester.c -o pds_tester
```

### Extended tester

```bash
gcc pds.c pds_tester-3.c -o pds_tester_3
```

## Run

Run a tester by passing a testcase file:

```bash
./pds_tester public_testcase.in
./pds_tester_3 public_testcase-3.in
```

## API Overview

The main functions provided by the library are:

- `create_db`
- `open_db`
- `store_db`
- `read_db`
- `update_db`
- `delete_db`
- `undelete_db`
- `close_db`

The extended design also includes table and database wrapper functions for the higher PDS layers.

## Notes

- The database uses a global `db_info` structure to keep track of the open file, index entries, record count, and record size.
- The schema file stores the full database definition, including tables and relationships.
- Record access supports both key-based lookup and retrieval using non-key fields in the extended design.
