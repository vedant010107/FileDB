# FileDB - DBMS PDS

This project is a lightweight file-based database system written in C. It stores records in a data file and maintains a separate index file for fast key-based access.

## Features

- Create a new database repository
- Open an existing database with a fixed record size
- Store records with a unique key
- Search records by key
- Update existing records
- Delete and undelete records
- Persist the index across program runs

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

## Notes

- The database uses a global `db_info` structure to keep track of the open file, index entries, record count, and record size.
- Record access is currently key-based and backed by an in-memory index that is written back to disk on close.
