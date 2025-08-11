# Couchbase Example Documentation

This repository contains example implementations demonstrating how to use the Couchbase C++ SDK with bRPC framework. The examples showcase both single-threaded and multi-threaded operations against Couchbase clusters.

## Table of Contents

- [Overview](#overview)
- [Files Structure](#files-structure)
- [Couchbase Core Implementation (couchbase.cpp)](#couchbase-core-implementation-couchbasecpp)
- [Single-Threaded Client (client.cpp)](#single-threaded-client-clientcpp)
- [Multi-Threaded Client (multi_threaded_client.cpp)](#multi-threaded-client-multi_threaded_clientcpp)
- [Build and Run](#build-and-run)

## Overview

This example demonstrates integration of Couchbase NoSQL database with the bRPC (Better RPC) framework. It provides:

- **Connection Management**: Efficient cluster connection and bucket handling
- **CRUD Operations**: Complete Create, Read, Update, Delete functionality
- **Performance Monitoring**: Detailed timing analysis for all operations
- **Multi-threading Support**: Concurrent operations using bthread
- **Error Handling**: Comprehensive error reporting and recovery

## Files Structure

```
example/couchbase_example/
├── client.cpp                 # Single-threaded example with timing analysis
├── multi_threaded_client.cpp  # Multi-threaded example using bthread
└── README.md                  # This documentation file

src/brpc/
├── couchbase.h                # CouchbaseObject class declaration
└── couchbase.cpp              # CouchbaseObject implementation
```

---

## Couchbase Core Implementation (couchbase.cpp)

The `couchbase.cpp` file implements the `CouchbaseObject` class, which provides a C++ wrapper around the Couchbase C++ SDK.

### Class: CouchbaseObject

The main class that encapsulates all Couchbase operations with thread-safe connection management.

#### Private Members

```cpp
std::optional<couchbase::cluster> g_cluster;     // Cluster connection handle
std::vector<couchbase::collection> g_collection; // Collections for all buckets
bool g_initialized = false;                      // Initialization state flag
```

#### Core Functions

### 1. `InitCouchbase(connection_string, username, password)`

**Purpose**: Initializes connection to Couchbase cluster and discovers all available buckets.

**Parameters**:
- `connection_string`: Couchbase cluster URL (e.g., "couchbase://localhost")
- `username`: Authentication username
- `password`: Authentication password

**Returns**: `bool` - `true` if initialization successful, `false` otherwise

**Implementation Details**:
```cpp
bool CouchbaseObject::InitCouchbase(const std::string& connection_string,
                                   const std::string& username,
                                   const std::string& password)
```

**Process Flow**:
1. Creates cluster connection options with credentials
2. Establishes asynchronous connection to cluster
3. Retrieves list of all available buckets
4. Creates collection handles for each bucket's default collection
5. Stores collections in `g_collection` vector
6. Sets `g_initialized` flag to `true`

**Error Handling**:
- Connection failures are logged with error messages
- Bucket discovery failures are handled gracefully
- Exception handling for SDK-level errors

### 2. `GetCollectionForBucket(bucket_name)`

**Purpose**: Internal helper function to retrieve collection handle for a specific bucket.

**Parameters**:
- `bucket_name`: Name of the target bucket

**Returns**: `std::optional<couchbase::collection>` - Collection handle or nullopt if not found

**Implementation Details**:
```cpp
std::optional<couchbase::collection> CouchbaseObject::GetCollectionForBucket(const std::string& bucket_name)
```

**Process Flow**:
1. Searches through `g_collection` vector
2. Compares `bucket_name()` method of each collection
3. Returns copy of matching collection handle
4. Returns `nullopt` if bucket not found

### 3. `CouchbaseGet(key, bucket_name)`

**Purpose**: Retrieves a document from Couchbase by its key.

**Parameters**:
- `key`: Document identifier
- `bucket_name`: Target bucket name

**Returns**: `std::pair<bool, std::string>` - Success status and document content

**Implementation Details**:
```cpp
std::pair<bool, std::string> CouchbaseObject::CouchbaseGet(const std::string& key, const std::string& bucket_name)
```

**Process Flow**:
1. Validates initialization state
2. Gets collection handle for specified bucket
3. Executes asynchronous get operation
4. Converts result to TAO JSON format
5. Serializes JSON to string
6. Returns success status and content

**Error Scenarios**:
- Uninitialized Couchbase connection
- Invalid bucket name
- Document not found
- JSON conversion errors

### 4. `CouchbaseUpsert(key, value, bucket_name)`

**Purpose**: Inserts or updates a document (insert if new, update if exists).

**Parameters**:
- `key`: Document identifier
- `value`: JSON string content to store
- `bucket_name`: Target bucket name

**Returns**: `bool` - `true` if operation successful, `false` otherwise

**Implementation Details**:
```cpp
bool CouchbaseObject::CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name)
```

**Process Flow**:
1. Validates initialization and gets collection
2. Parses input JSON string using TAO JSON
3. Executes asynchronous upsert operation
4. Handles operation result and errors

**Use Cases**:
- Initial document creation
- Document updates without CAS (Compare-And-Swap) checking
- Bulk data loading operations

### 5. `CouchbaseAdd(key, value, bucket_name)`

**Purpose**: Inserts a new document (fails if document already exists).

**Parameters**:
- `key`: Document identifier
- `value`: JSON string content to store
- `bucket_name`: Target bucket name

**Returns**: `bool` - `true` if insertion successful, `false` if document exists or error

**Implementation Details**:
```cpp
bool CouchbaseObject::CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name)
```

**Process Flow**:
1. Validates initialization and gets collection
2. Parses input JSON string
3. Executes asynchronous insert operation
4. Returns false if document already exists

**Use Cases**:
- Ensuring document uniqueness
- Race condition prevention
- Initial data seeding

### 6. `CouchbaseRemove(key, bucket_name)`

**Purpose**: Deletes a document from Couchbase.

**Parameters**:
- `key`: Document identifier to delete
- `bucket_name`: Target bucket name

**Returns**: `bool` - `true` if deletion successful, `false` otherwise

**Implementation Details**:
```cpp
bool CouchbaseObject::CouchbaseRemove(const std::string& key, const std::string& bucket_name)
```

**Process Flow**:
1. Validates initialization and gets collection
2. Executes asynchronous remove operation
3. Handles success/failure scenarios

### 7. `CloseCouchbase()`

**Purpose**: Cleanly shuts down Couchbase connection and releases resources.

**Implementation Details**:
```cpp
void CouchbaseObject::CloseCouchbase()
```
**Process Flow**:
1. Checks initialization state
2. Resets cluster connection
3. Clears collections vector
4. Sets `g_initialized` to `false`

The implementation uses multiple layers of error handling:

1. **SDK Level**: Catches `std::exception` from Couchbase SDK
2. **Operation Level**: Checks error codes from async operations
3. **Validation Level**: Verifies initialization and bucket existence
4. **Logging**: Uses colored console output (RED_TEXT) for error visibility

#### JSON Handling

Uses TAO JSON library for:
- **Parsing**: `tao::json::from_string(value)`
- **Serialization**: `tao::json::to_string(content_json)`
- **Type Safety**: `result.content_as<tao::json::value>()`

---

## Single-Threaded Client (client.cpp)

The `client.cpp` file demonstrates comprehensive Couchbase operations with detailed performance monitoring.

### Main Function

**Purpose**: Orchestrates all Couchbase operations with timing analysis and comprehensive error reporting.

#### Command Line Parameters

```cpp
DEFINE_string(couchbase_host, "localhost", "Couchbase server host");
DEFINE_string(username, "Administrator", "Couchbase username");
DEFINE_string(password, "password", "Couchbase password");
DEFINE_string(bucket, "testing", "Couchbase bucket name");
```

#### Operation Sequence

### 1. Initialization Phase

```cpp
std::string connection_string = "couchbase://" + FLAGS_couchbase_host;
if (!couchbase_client.InitCouchbase(connection_string, FLAGS_username, FLAGS_password))
```

**Purpose**: Establishes cluster connection and discovers buckets
**Timing**: Measured in milliseconds due to network overhead
**Error Handling**: Exits application if initialization fails

### 2. Document Addition (First Attempt)

```cpp
std::string user_data = R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
if (couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket))
```

**Purpose**: Tests document insertion with Add operation
**Expected Result**: Success on first run, failure on subsequent runs

### 3. Document Addition (Duplicate Test)

```cpp
if (couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket))
```

**Purpose**: Validates Add operation's uniqueness constraint
**Expected Result**: Should fail with "document already exists" error
**Educational Value**: Demonstrates difference between Add and Upsert

### 4. Document Update with Upsert

```cpp
std::string updated_user_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
if (couchbase_client.CouchbaseUpsert("user::john_doe", updated_user_data, FLAGS_bucket))
```

**Purpose**: Updates existing document with new data
**Data Changes**: Age increment and email modification
**Strategy**: Upsert works regardless of document existence

### 5. Document Retrieval

```cpp
auto [success, retrieved_data] = couchbase_client.CouchbaseGet("user::john_doe", FLAGS_bucket);
if (success && !retrieved_data.empty())
```

**Purpose**: Verifies document content after update
**Validation**: Checks both operation success and content availability
**Output**: Displays retrieved JSON for verification

### 6. Bulk Operations Loop

```cpp
for (int i = 1; i <= 3; i++) {
    std::string key = "item::" + std::to_string(i);
    // Try Add first, fallback to Upsert if needed
}
```

### 7. Document Removal

```cpp
if (couchbase_client.CouchbaseRemove("item::1", FLAGS_bucket))
```

**Purpose**: Demonstrates document deletion
**Target**: Removes first document from bulk operations
**Verification**: Success/failure logging with timing

### 8. Connection Cleanup

```cpp
couchbase_client.CloseCouchbase();
```

**Purpose**: Graceful connection termination
**Best Practice**: Always close connections before application exit

---

## Multi-Threaded Client (multi_threaded_client.cpp)

The `multi_threaded_client.cpp` demonstrates concurrent Couchbase operations using bthread (brpc's threading library).

### Thread Parameters Structure

```cpp
struct thread_parameters {
    brpc::CouchbaseObject *couchbase_client;
    std::string bucket_name;
};
```

**Purpose**: Passes shared connection and unique bucket name to each thread
**Design**: Single connection, multiple buckets approach for optimal resource usage

### Thread Function: `threaded_example(void* arg)`

**Purpose**: Executes complete CRUD operation sequence in each thread.

**Parameters**: 
- `arg`: Void pointer to `thread_parameters` structure

**Returns**: `nullptr` (required by bthread interface)

#### Thread Operations Sequence

Each thread performs identical operations to the single-threaded client:

### 1. Parameter Extraction

```cpp
thread_parameters *params = static_cast<thread_parameters *>(arg);
brpc::CouchbaseObject *couchbase_client = params->couchbase_client;
std::string bucket_name = params->bucket_name;
```

**Safety**: Static cast from void pointer with proper type safety
**Scope**: Local variables for thread-specific bucket operations

### 2. Document Addition Test

```cpp
std::string user_data = R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
if (couchbase_client->CouchbaseAdd("user::john_doe", user_data, bucket_name))
```

**Thread Safety**: Each thread operates on different bucket
**Isolation**: No data conflicts between threads

### 3. Duplicate Addition Test

```cpp
if (couchbase_client->CouchbaseAdd("user::john_doe", user_data, bucket_name))
```

**Expected Behavior**: Should fail in each thread after first success
**Concurrency**: Each thread maintains its own bucket state

### 4. Update Operation

```cpp
std::string updated_user_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
if (couchbase_client->CouchbaseUpsert("user::john_doe", updated_user_data, bucket_name))
```

**Thread Isolation**: Updates only affect the thread's designated bucket

### 5. Data Retrieval

```cpp
auto [success, retrieved_data] = couchbase_client->CouchbaseGet("user::john_doe", bucket_name);
```

**Verification**: Each thread validates its own data modifications

### 6. Bulk Operations

```cpp
for (int i = 1; i <= 3; i++) {
    std::string key = "item::" + std::to_string(i);
    // Add/Upsert strategy per thread
}
```

**Scaling**: Multiple documents per thread across multiple buckets
**Performance**: Tests concurrent bulk operations

### 7. Document Removal

```cpp
if (couchbase_client->CouchbaseRemove("item::1", bucket_name))
```

**Cleanup**: Each thread removes its own documents

### Main Function (Multi-threaded)

#### Initialization

```cpp
brpc::CouchbaseObject couchbase_client;
couchbase_client.InitCouchbase("couchbase://" + FLAGS_couchbase_host, FLAGS_username, FLAGS_password);
```

**Shared Resource**: Single connection object used by all threads
**Efficiency**: Reduces connection overhead and resource usage

#### Thread Creation Strategy

```cpp
std::string bucket_name = "testing_";
thread_parameters params[5];
for(int i = 0; i < 5; i++) {
    params[i] = {&couchbase_client, bucket_name + std::to_string(i)};
}
```

**Design Pattern**: 
- **5 concurrent threads**
- **Unique bucket per thread** (`testing_0`, `testing_1`, etc.)
- **Shared connection object**
- **Independent parameter structures**

#### Thread Launching

```cpp
std::vector<bthread_t> bids;
for (int i = 0; i < 5; i++) {
    bthread_t bid;
    if(bthread_start_background(&bid, nullptr, threaded_example, &params[i]) != 0) {
        std::cerr << "Failed to start thread " << i << std::endl;
        return -1;
    }
    bids.push_back(bid);
}
```


**Purpose**: Waits for all threads to complete before program termination
**Safety**: Ensures proper cleanup and prevents premature exit

---

## Build and Run

### Prerequisites

```bash
# Install dependencies
sudo apt-get install libcouchbase-dev
sudo apt-get install libgflags-dev
```

### Compilation

```bash
# Build with brpc framework
make
```

### brpc compilation

Make sure you have the correct path set in the makefile for couchbase library and fmt library
```
# fmtlib support
CXXFLAGS += -I/opt/homebrew/opt/fmt/include
LIBPATHS += -L/opt/homebrew/opt/fmt/lib
DYNAMIC_LINKINGS += -lfmt
```

### Execution

#### Single-threaded Client

```bash
./client --couchbase_host=localhost \
         --username=Administrator \
         --password=password \
         --bucket=testing
```

#### Multi-threaded Client

```bash
./multi_threaded_client --couchbase_host=localhost \
                       --username=Administrator \
                       --password=password
```

---