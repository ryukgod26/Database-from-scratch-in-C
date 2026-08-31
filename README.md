# Database From Scratch in C

A lightweight database engine built **from scratch in C**, with the goal of understanding how database systems work internally.

Instead of relying on an existing database library or storage engine, this project implements the fundamental components of a database system manually — including page-based storage, serialization, a pager/cache layer, cursors, and a B-Tree-based indexing structure.

> **Project Status:** Experimental / Educational
> **Language:** C
> **Architecture:** Page-based storage with B-Tree indexing
> **Copyright:** © 2026 Eklavya. All rights reserved.

---

## Overview

This project is an implementation of a small database engine designed to explore the internal concepts behind traditional relational databases.

The database provides an interactive command-line shell where records can be inserted and retrieved using simplified SQL-like commands.

The current record structure contains:

* `id`
* `username`
* `email`

Records are stored in fixed-size pages and organized using a B-Tree structure. The implementation uses separate **leaf nodes** for storing records and **internal nodes** for directing searches.

The project also includes persistent storage, allowing database pages to be written to a database file when the database is closed.

---

## Features

### Core Database Operations

Currently supported:

* Insert records
* Select and display records
* Duplicate ID detection
* Input validation
* Persistent database files
* Interactive command-line interface

The parser recognizes `insert` and `select` statements, while invalid statements are rejected.

### B-Tree Storage

The database uses a B-Tree-style structure consisting of:

* Leaf nodes
* Internal nodes
* Root node
* Parent pointers
* Child pointers
* Leaf-to-leaf links

Leaf nodes store the actual records, while internal nodes store keys and references to child nodes.

### Binary Search

Searching inside both leaf and internal nodes uses binary search.

This allows the database to locate the appropriate position for a key without scanning every record sequentially.

### Node Splitting

When a leaf node becomes full, the implementation:

1. Creates a new leaf node.
2. Splits the records between the old and new nodes.
3. Updates the linked-leaf relationship.
4. Updates parent information.
5. Creates a new root when necessary.
6. Inserts the new node into its parent.

Internal nodes can also split when their capacity is reached, allowing the B-Tree to grow beyond a single level.

### Persistent Storage

Database pages are backed by a file descriptor and written to disk using low-level file operations such as `lseek()` and `write()`.

When the database closes, cached pages are flushed to the database file before memory is released.

### Page-Based Architecture

The storage layer works with fixed-size pages.

The implementation currently defines:

```text
PAGE_SIZE = 4096 bytes
TABLE_MAX_PAGES = 100
```

Records are serialized into pages rather than being stored directly as C structures on disk.

---

# Architecture

The database can be viewed as several layers:

```text
┌───────────────────────────────┐
│        Database Shell         │
│       Command Processing      │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│       Statement Parser        │
│     INSERT / SELECT           │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│       Execution Layer         │
│ Insert / Select / Validation  │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│        Cursor Layer            │
│ Search / Traversal / Position │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│          B-Tree Layer          │
│ Leaf Nodes / Internal Nodes   │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│         Pager Layer            │
│ Pages / Cache / Disk I/O      │
└───────────────┬───────────────┘
                │
┌───────────────▼───────────────┐
│       Database File            │
│        Persistent Data         │
└───────────────────────────────┘
```

---

# Data Model

The current database stores a simple `Row` structure:

| Field      | Type       |            Maximum Size |
| ---------- | ---------- | ----------------------: |
| `id`       | `uint32_t` | 32-bit unsigned integer |
| `username` | `char[]`   |           32 characters |
| `email`    | `char[]`   |          255 characters |

The source defines the username and email limits directly through constants.

The `id` acts as the key used by the B-Tree.

---

# Storage Format

Each database page is **4096 bytes**.

A leaf node contains:

```text
┌─────────────────────────────────┐
│ Common Node Header              │
├─────────────────────────────────┤
│ Number of Cells                 │
├─────────────────────────────────┤
│ Next Leaf Pointer               │
├─────────────────────────────────┤
│ Key + Row                       │
├─────────────────────────────────┤
│ Key + Row                       │
├─────────────────────────────────┤
│ ...                             │
└─────────────────────────────────┘
```

The common node header contains information such as:

* Node type
* Root status
* Parent page number

Leaf nodes additionally store their number of cells and the page number of the next leaf node.

---

# B-Tree Structure

The database distinguishes between two node types:

```text
NodeLeaf
NodeInternal
```

### Leaf Node

Leaf nodes contain:

```text
Key → Row
```

They are responsible for storing actual database records.

Leaf nodes are linked together using a `next_leaf` pointer, allowing sequential traversal across multiple leaf pages.

### Internal Node

Internal nodes contain:

```text
Child Pointer + Key
```

and a right-child pointer.

They are used to determine which child node should be searched for a particular key.

---

# Searching

A search begins at the root.

If the root is a leaf node, the database performs a binary search directly within that leaf.

If the root is an internal node, the database:

1. Performs binary search on the internal-node keys.
2. Determines the appropriate child.
3. Loads the child page.
4. Recursively continues the search.
5. Eventually reaches the appropriate leaf node.

## This behavior is implemented through `table_find()`, `internal_node_find()`, and `leaf_node_find()`.

# Insertion

An insertion follows this general process:

```text
User Input
    │
    ▼
Parse INSERT
    │
    ▼
Validate ID / Username / Email
    │
    ▼
Search B-Tree
    │
    ├── ID exists → Duplicate Key
    │
    └── ID doesn't exist
              │
              ▼
        Insert into Leaf
              │
              ▼
       Is Leaf Full?
          /       \
        No         Yes
        │           │
        ▼           ▼
     Finish     Split Leaf
                    │
                    ▼
              Update Parent
                    │
                    ▼
              Split Parent
              if necessary
```

Before insertion, the implementation checks for duplicate keys.

If the target leaf is full, the node is split and the new record is placed into the appropriate half.

---

# Root Splitting

When the root leaf becomes full, a new root is created.

The previous root is moved into a new left child, while the newly created node becomes the right child.

The root then becomes an internal node containing references to both children. Parent pointers are updated accordingly.

This allows the database to grow from:

```text
        Leaf
```

into:

```text
             Root
            /    \
        Leaf     Leaf
```

and eventually into deeper B-Tree structures.

---

# Cursor System

The database uses a `Cursor` structure to keep track of the current position:

```text
Table
Page Number
Cell Number
End-of-Table
```

Cursors are used during both searching and sequential record traversal.

When the cursor reaches the end of a leaf node, it follows the leaf's `next_leaf` pointer and continues from the next leaf.

---

# Serialization

Records are not written directly to disk as C structures.

Instead, the database serializes the individual fields into a fixed memory layout:

```text
┌──────────┬──────────────────┬──────────────────────────┐
│    ID    │     Username     │          Email           │
└──────────┴──────────────────┴──────────────────────────┘
```

The reverse operation is performed through deserialization when records are read from pages.

This separates the in-memory representation of a `Row` from its on-disk representation.

---

# Pager

The pager manages database pages between memory and the database file.

Its main responsibilities include:

* Opening the database file
* Tracking file size
* Tracking the number of pages
* Allocating page memory
* Loading pages
* Flushing pages back to disk
* Managing page references

The pager maintains an array of page pointers with a maximum of 100 pages.

---

# Command-Line Interface

When the database starts, it provides an interactive prompt:

```text
db >
```

Users can enter database statements or special meta-commands.

## SQL-like Commands

### INSERT

Insert a record using:

```text
insert <id> <username> <email>
```

Example:

```text
insert 1 eklavya eklavya@example.com
```

### SELECT

Display stored records:

```text
select
```

Example output:

```text
id:1 ,Username:eklavya, Email:eklavya@example.com
```

Records are deserialized from the database pages and printed while the cursor traverses the table.

---

# Meta Commands

The database shell currently supports:

| Command      | Description                        |
| ------------ | ---------------------------------- |
| `.exit`      | Close the database and flush pages |
| `.constants` | Display database storage constants |
| `.visualize` | Display the B-Tree structure       |

The `.constants` command exposes values such as row size, node header size, cell size, and maximum leaf cells.

The `.visualize` command recursively prints internal and leaf nodes, making it possible to inspect the B-Tree structure from the command line.

---

# Input Validation

The current implementation validates:

* Missing INSERT arguments
* Negative IDs
* Username length
* Email length
* Duplicate IDs
* Unrecognized commands

The defined preparation errors include:

```text
PREPARE_SYNTAX_ERROR
PREPARE_STRING_TOO_LONG
PREPARE_NEGATIVE_ID
PREPARE_UNRECOGNISED_COMMAND
```

---

# Error Handling

The implementation checks for errors during operations such as:

* Opening the database file
* Reading from disk
* Writing to disk
* Seeking within the database file
* Closing the database
* Invalid page access
* Invalid child references

Fatal errors terminate the program using `EXIT_FAILURE`.

---

# Persistence

The database uses a file-backed storage mechanism.

When the database is closed:

1. Cached pages are identified.
2. Each active page is flushed to disk.
3. Allocated page memory is released.
4. The database file descriptor is closed.
5. Pager and table structures are freed.

This means the database is not purely an in-memory data structure; it has a persistent page-based storage layer.

---

# Example Session

```text
db > insert 1 alice alice@example.com
db > insert 2 bob bob@example.com
db > insert 3 charlie charlie@example.com
db > select

id:1 ,Username:alice, Email:alice@example.com
id:2 ,Username:bob, Email:bob@example.com
id:3 ,Username:charlie, Email:charlie@example.com

db > .visualize
Tree:
- leaf (size 3)
  - 1
  - 2
  - 3

db > .constants
CONSTANTS:
ROW SIZE: ...
COMMON NODE HEADER SIZE: ...
LEAF NODE HEADER SIZE: ...
LEAF NODE CELL SIZE: ...
LEAF NODE MAX CELLS: ...

db > .exit
Database Closed Successfully.
```

---

# Project Structure

A minimal version of the project can be organized as:

```text
database/
│
├── database.c
├── database
├── database.db
├── LICENSE
└── README.md
```

Where:

* `database.c` — database engine implementation
* `database` — compiled executable
* `database.db` — persistent database file created/opened by the engine
* `LICENSE` — project licensing and copyright information
* `README.md` — project documentation

---

# Technical Concepts Demonstrated

This project focuses on understanding the internals of database systems through direct implementation.

Key concepts include:

* C memory management
* File descriptors
* Low-level file I/O
* Page-based storage
* Buffer/cache management
* Data serialization
* Data deserialization
* Binary search
* Cursors
* Tree traversal
* B-Trees
* Leaf nodes
* Internal nodes
* Parent pointers
* Node splitting
* Root creation
* Persistent storage
* Command parsing
* Input validation

---

# Current Limitations

This project is intentionally small and educational rather than a full SQL database.

The current implementation supports only a limited set of operations:

* `INSERT`
* `SELECT`

It does not currently provide a complete SQL language, relational schema management, transactions, joins, indexes beyond the built-in B-Tree organization, query optimization, or concurrency control.

The source also contains commented-out earlier implementations and development-stage code, reflecting the project's progression from simpler page/row storage toward the current B-Tree implementation.

---

# Future Development

Potential future improvements include:

* `DELETE`
* `UPDATE`
* More SQL expressions
* Multiple tables
* Table schemas
* More robust query parsing
* Improved B-Tree balancing
* Database metadata
* Free-page management
* Transactions
* Crash recovery
* Write-ahead logging
* Concurrency support
* Improved buffer pool management
* More comprehensive error recovery
* Cross-platform support
* Automated testing
* Build system / Makefile

---

# Learning Objective

The primary objective of this project is to understand what happens **inside a database engine**.

Instead of treating a database as a black box, the project explores the individual mechanisms responsible for:

```text
User Query
    ↓
Parsing
    ↓
Execution
    ↓
B-Tree Search
    ↓
Page Management
    ↓
Memory
    ↓
Disk
```

The implementation therefore serves as both a functional miniature database and a practical study of database internals.

---

# Author

**Eklavya**

Database engine designed and implemented from scratch in C.

---

# License

This project is proprietary software.

Copyright © 2026 **Eklavya**. All rights reserved.

See the [`LICENSE`](LICENSE) file for complete terms and restrictions.

---

## Disclaimer

This project is intended primarily for educational and experimental purposes. It is not intended to replace production-grade database systems.
