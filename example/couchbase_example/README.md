# Couchbase Example Documentation

This repository contains example implementations demonstrating how to use the Couchbase C++ SDK with bRPC framework. The examples showcase comprehensive Couchbase operations including CRUD operations and N1QL queries.

## Table of Contents

- [Overview](#overview)
- [Files Structure](#files-structure)
- [Couchbase Core Implementation (couchbase.cpp)](#couchbase-core-implementation-couchbasecpp)
- [Single-Threaded Client (client.cpp)](#single-threaded-client-clientcpp)
- [Build and Run](#build-and-run)

## Overview

This example demonstrates integration of Couchbase NoSQL database with the bRPC (Better RPC) framework. It provides:

- **Connection Management**: Efficient cluster connection with scope and collection support
- **CRUD Operations**: Complete Create, Read, Update, Delete functionality
- **N1QL Query Support**: Cluster-level and scope-level query operations with options

## Files Structure

```
example/couchbase_example/
├── client.cpp                 # Single-threaded example demonstrating all operations
└── README.md                  # This documentation file

src/brpc/
├── couchbase.h                # CouchbaseWrapper class declaration
└── couchbase.cpp              # CouchbaseWrapper implementation
```

---

## Couchbase Core Implementation (couchbase.cpp)

The `couchbase.cpp` file implements the `CouchbaseWrapper` class, which provides a C++ wrapper around the Couchbase C++ SDK.

### Class: CouchbaseWrapper

The main class that encapsulates all Couchbase operations with thread-safe connection management.

#### Private Members

```cpp
std::optional<couchbase::cluster> g_cluster;     // Cluster connection handle
bool g_initialized = false;                      // Initialization state flag
```

#### Core Functions

### 1. `InitCouchbase(connection_string, username, password)`

**Purpose**: Initializes connection to Couchbase cluster.

**Parameters**:
- `connection_string`: Couchbase cluster URL (e.g., "couchbase://localhost")
- `username`: Authentication username
- `password`: Authentication password

**Returns**: `bool` - `true` if initialization successful, `false` otherwise

**Implementation Details**:
```cpp
bool CouchbaseWrapper::InitCouchbase(const std::string& connection_string,
                                   const std::string& username,
                                   const std::string& password)
```

**Process Flow**:
1. Creates cluster connection options with credentials
2. Establishes asynchronous connection to cluster
3. Sets `g_initialized` flag to `true`

**Error Handling**:
- Connection failures are logged with error messages
- Exception handling for SDK-level errors

### 2. `CouchbaseGet(key, bucket_name, scope, collection)`

**Purpose**: Retrieves a document from Couchbase by its key.

**Parameters**:
- `key`: Document identifier
- `bucket_name`: Target bucket name
- `scope`: Scope name (default: "_default")
- `collection`: Collection name (default: "_default")

**Returns**: `std::pair<bool, std::string>` - Success status and document content

**Implementation Details**:
```cpp
std::pair<bool, std::string> CouchbaseWrapper::CouchbaseGet(const std::string& key, 
                                                          const std::string& bucket_name,
                                                          const std::string& scope,
                                                          const std::string& collection)
```

**Process Flow**:
1. Validates initialization state
2. Executes asynchronous get operation using bucket.scope.collection path
3. Converts result to TAO JSON format
4. Serializes JSON to string
5. Returns success status and content

**Error Scenarios**:
- Uninitialized Couchbase connection
- Invalid bucket/scope/collection name
- Document not found
- JSON conversion errors

### 3. `CouchbaseUpsert(key, value, bucket_name, scope, collection)`

**Purpose**: Inserts or updates a document (insert if new, update if exists).

**Parameters**:
- `key`: Document identifier
- `value`: JSON string content to store
- `bucket_name`: Target bucket name
- `scope`: Scope name (default: "_default")
- `collection`: Collection name (default: "_default")

**Returns**: `bool` - `true` if operation successful, `false` otherwise

**Implementation Details**:
```cpp
bool CouchbaseWrapper::CouchbaseUpsert(const std::string& key, 
                                     const std::string& value,
                                     const std::string& bucket_name,
                                     const std::string& scope,
                                     const std::string& collection)
```

**Process Flow**:
1. Validates initialization and parses JSON
2. Executes asynchronous upsert operation
3. Handles operation result and errors

**Use Cases**:
- Initial document creation
- Document updates without CAS (Compare-And-Swap) checking
- Bulk data loading operations

### 4. `CouchbaseAdd(key, value, bucket_name, scope, collection)`

**Purpose**: Inserts a new document (fails if document already exists).

**Parameters**:
- `key`: Document identifier
- `value`: JSON string content to store
- `bucket_name`: Target bucket name
- `scope`: Scope name (default: "_default")
- `collection`: Collection name (default: "_default")

**Returns**: `bool` - `true` if insertion successful, `false` if document exists or error

**Implementation Details**:
```cpp
bool CouchbaseWrapper::CouchbaseAdd(const std::string& key, 
                                  const std::string& value,
                                  const std::string& bucket_name,
                                  const std::string& scope,
                                  const std::string& collection)
```


### 5. `CouchbaseRemove(key, bucket_name, scope, collection)`

**Purpose**: Deletes a document from Couchbase.

**Parameters**:
- `key`: Document identifier to delete
- `bucket_name`: Target bucket name
- `scope`: Scope name (default: "_default")
- `collection`: Collection name (default: "_default")

**Returns**: `bool` - `true` if deletion successful, `false` otherwise

**Implementation Details**:
```cpp
bool CouchbaseWrapper::CouchbaseRemove(const std::string& key,
                                     const std::string& bucket_name,
                                     const std::string& scope,
                                     const std::string& collection)
```

**Process Flow**:
1. Validates initialization
2. Executes asynchronous remove operation
3. Handles success/failure scenarios

### 6. Query Operations

The class provides four query methods for different use cases:

#### 6.1 `Query(statement)`
**Purpose**: Execute N1QL query at cluster level without options.
```cpp
std::pair<bool, std::vector<std::string>> Query(std::string statement)
```

#### 6.2 `Query(statement, query_options)`
**Purpose**: Execute N1QL query at cluster level with query options.
```cpp
std::pair<bool, std::vector<std::string>> Query(std::string statement, 
                                               couchbase::query_options& q_opts)
```

#### 6.3 `Query(statement, bucket_name, scope_name)`
**Purpose**: Execute N1QL query at scope level without options.
```cpp
std::pair<bool, std::vector<std::string>> Query(std::string statement, 
                                               const std::string& bucket_name, 
                                               const std::string& scope_name)
```

#### 6.4 `Query(statement, bucket_name, scope_name, query_options)`
**Purpose**: Execute N1QL query at scope level with query options.
```cpp
std::pair<bool, std::vector<std::string>> Query(std::string statement, 
                                               const std::string& bucket_name, 
                                               const std::string& scope_name, 
                                               couchbase::query_options& q_opts)
```

**Query Features**:
- Cluster-level and scope-level query execution
- Support for query options
- returns a pair of boolean and string, indicating query status and the query result

### 7. `CloseCouchbase()`

**Purpose**: Cleanly shuts down Couchbase connection and releases resources.

**Implementation Details**:
```cpp
void CouchbaseWrapper::CloseCouchbase()
```
**Process Flow**:
1. Checks initialization state
2. Resets cluster connection
3. Sets `g_initialized` to `false`


## Single-Threaded Client (client.cpp)

The `client.cpp` file demonstrates comprehensive Couchbase operations showcasing all available functionality.

### Main Function

**Purpose**: Orchestrates all Couchbase operations with comprehensive error reporting.

#### Command Line Parameters

```cpp
DEFINE_string(couchbase_host, "localhost", "Couchbase server host");
DEFINE_string(username, "Administrator", "Couchbase username");
DEFINE_string(password, "password", "Couchbase password");
DEFINE_string(bucket, "testing", "Couchbase bucket name");
```

#### Operation Sequence

### Initialization Phase

```cpp
std::string connection_string = "couchbase://" + FLAGS_couchbase_host;
if (!couchbase_client.InitCouchbase(connection_string, FLAGS_username, FLAGS_password))
```

**Purpose**: Establishes cluster connection
**Error Handling**: Exits application if initialization fails

### Example 1. Document Addition (First Attempt)

```cpp
std::string user_data = R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
if (couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket))
```

**Purpose**: Tests document insertion with Add operation
**Expected Result**: Success on first run, failure on subsequent runs

### Example 2. Document Addition (Duplicate Test)

```cpp
if (couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket))
```

**Purpose**: Validates Add operation's uniqueness constraint
**Expected Result**: Should fail with "document already exists" error
**Educational Value**: Demonstrates difference between Add and Upsert

### Example 3. Document Update with Upsert

```cpp
std::string updated_user_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
if (couchbase_client.CouchbaseUpsert("user::john_doe", updated_user_data, FLAGS_bucket))
```

**Purpose**: Updates existing document with new data
**Data Changes**: Age increment and email modification
**Strategy**: Upsert works regardless of document existence

### Example 4. Document Retrieval

```cpp
auto [success, retrieved_data] = couchbase_client.CouchbaseGet("user::john_doe", FLAGS_bucket);
if (success && !retrieved_data.empty())
```

**Purpose**: Verifies document content after update
**Validation**: Checks both operation success and content availability
**Output**: Displays retrieved JSON for verification

### Example 5. Bulk Operations Loop

Tests ADD function with fallback to upsert strategy:
```cpp
for (int i = 1; i <= 3; i++) {
    std::string key = "item::" + std::to_string(i);
    // Try Add first, fallback to Upsert if needed
}
```

**Strategy**: Attempts Add operation first, uses Upsert as fallback
**Purpose**: Demonstrates proper handling of document existence scenarios

### Example 6. N1QL Query Operations

#### Query 1: Select All Documents
```cpp
std::string select_all_query = "SELECT META().id, * FROM `" + FLAGS_bucket + "` WHERE META().id LIKE 'user::%' OR META().id LIKE 'item::%'";
auto [query_success1, query_results1] = couchbase_client.Query(select_all_query);
```

**Purpose**: Retrieves all documents matching specific key patterns
**Scope**: Cluster-level query execution
**Output**: Displays found documents with their metadata

#### Query 2: Scoped Query with Explicit Bucket and Scope
```cpp
std::string scoped_query = "SELECT META().id, email FROM _default WHERE email LIKE '%@%'";
auto [query_success6, query_results6] = couchbase_client.Query(scoped_query, FLAGS_bucket, "_default");
```

**Purpose**: Demonstrates scope-level query execution
**Target**: Default collection within default scope
**Filter**: Documents containing email addresses

#### Query 3: Parameterized Query with Options
```cpp
couchbase::mutation_state consistency_state;
std::string scoped_parameterized_query = R"(
    SELECT * FROM _default WHERE email = $1 LIMIT 10;
)";
couchbase::query_options opts{};
opts.client_context_id("my-query-ctx")
    .consistent_with(consistency_state)
    .metrics(true)
    .profile(couchbase::query_profile::phases)
    .adhoc(false);

const std::vector<std::string> param = {"john"};
for(const auto& p : param) {
    opts.add_positional_parameter(p);
}
auto [query_success7, query_results7] = couchbase_client.Query(scoped_parameterized_query, FLAGS_bucket, "_default", opts);
```

**Features Demonstrated**:
- Positional parameters
- Query options configuration
- Mutation state consistency

### Example 7. Document Removal

```cpp
if (couchbase_client.CouchbaseRemove("item::1", FLAGS_bucket))
```

**Purpose**: Demonstrates document deletion
**Target**: Removes first document from bulk operations
**Verification**: Success/failure logging

### Example 8. Connection Cleanup

```cpp
couchbase_client.CloseCouchbase();
```

**Purpose**: Graceful connection termination
**Best Practice**: Always close connections before application exit

---

## Build and Run

### Prerequisites

```bash
# Install dependencies
brew install couchbase-cxx-client
brew install fmt
```

### brpc compilation

Make sure you have the correct path set in the makefile for couchbase library and fmt library
```
# fmtlib support
CXXFLAGS += -I/opt/homebrew/opt/fmt/include
LIBPATHS += -L/opt/homebrew/opt/fmt/lib
DYNAMIC_LINKINGS += -lfmt
```

### Compilation

```bash
# Build with brpc framework
make
```

### Execution

#### Single-threaded Client

```bash
./couchbase_client --couchbase_host=localhost \
         --username=Administrator \
         --password=password \
         --bucket=testing
```

### Expected Output

The client will execute all operations sequentially and display:
- Connection establishment status
- CRUD operation results
- N1QL query execution and results
- Error messages (if any)
- Operation completion status

### Notes

- Ensure Couchbase Server is running and accessible
- Create the target bucket before running the example
- The example uses default scope and collection unless specified
---