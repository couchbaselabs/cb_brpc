
[Couchbase](https://www.couchbase.com/) is a high-performance NoSQL document database. In order to access Couchbase more conveniently and make full use of bthread's capability of concurrency, brpc directly supports Couchbase operations through a C++ wrapper. Check [example/couchbase_example](https://github.com/apache/brpc/tree/master/example/couchbase_example/) for complete examples.

**NOTE**: This implementation uses the official Couchbase C++ SDK v3.x which supports both community and enterprise features of Couchbase Server.

Advantages compared to using the raw [Couchbase C++ SDK](https://docs.couchbase.com/cxx-sdk/current/hello-world/start-using-sdk.html):

- **Thread safety**: No need to set up separate clients for each thread.
- **Simplified interface**: Streamlined API that wraps common operations.
- **JSON handling**: Built-in JSON serialization using tao::json.
- **Connection management**: Automatic connection lifecycle management.
- **Error handling**: Consistent error reporting with detailed error messages.

The current implementation provides a clean C++ interface for Couchbase operations including CRUD operations and N1QL queries.

# Using the Couchbase Client

## Initialization

Create and initialize a `CouchbaseWrapper` for accessing Couchbase:

```cpp
#include <brpc/couchbase.h>

brpc::CouchbaseWrapper couchbase_client;

// Initialize connection to Couchbase cluster
std::string connection_string = FLAGS_couchbase_host;
std::string username = FLAGS_username;
std::string password = FLAGS_password;

if (!couchbase_client.InitCouchbase(connection_string, username, password)) {
    LOG(FATAL) << "Failed to initialize Couchbase connection";
    return -1;
}
```

## Basic CRUD Operations

### Insert Document (Add)

Use `Add` to insert a new document. This operation will fail if the document already exists:

```cpp
std::string user_data = R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";

if (couchbase_client.CouchbaseAdd("user::john_doe", user_data, "my_bucket")) {
    LOG(INFO) << "Document added successfully";
} else {
    LOG(ERROR) << "Failed to add document (may already exist)";
}
```

### Upsert Document

Use `Upsert` to insert a new document or update an existing one:

```cpp
std::string updated_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";

if (couchbase_client.CouchbaseUpsert("user::john_doe", updated_data, "my_bucket")) {
    LOG(INFO) << "Document upserted successfully";
} else {
    LOG(ERROR) << "Failed to upsert document";
}
```

### Retrieve Document (Get)

Use `Get` to retrieve a document by its key:

```cpp
auto [success, retrieved_data] = couchbase_client.CouchbaseGet("user::john_doe", "my_bucket");

if (success && !retrieved_data.empty()) {
    LOG(INFO) << "Retrieved document: " << retrieved_data;
} else {
    LOG(ERROR) << "Failed to retrieve document or document not found";
}
```

### Delete Document (Remove)

Use `Remove` to delete a document:

```cpp
if (couchbase_client.CouchbaseRemove("user::john_doe", "my_bucket")) {
    LOG(INFO) << "Document removed successfully";
} else {
    LOG(ERROR) << "Failed to remove document";
}
```

## Working with Scopes and Collections

All operations support Couchbase's scope and collection model for better data organization:

```cpp
// Using custom scope and collection
std::string bucket = "my_bucket";
std::string scope = "my_scope";
std::string collection = "my_collection";

// Add document to specific scope and collection
if (couchbase_client.CouchbaseAdd("doc::key", json_data, bucket, scope, collection)) {
    LOG(INFO) << "Document added to " << bucket << "." << scope << "." << collection;
}

// Get document from specific scope and collection
auto [success, data] = couchbase_client.CouchbaseGet("doc::key", bucket, scope, collection);
```

**Note**: If scope and collection are not specified, they default to `"_default"`.

## N1QL Queries

The client supports N1QL queries for complex data operations:

### Cluster-level Query

```cpp
std::string query = "SELECT name, age FROM `my_bucket` WHERE age > 25";
auto [success, results] = couchbase_client.Query(query);

if (success) {
    for (const auto& result : results) {
        LOG(INFO) << "Query result: " << result;
    }
} else {
    LOG(ERROR) << "Query failed";
}
```

### Query with Options

```cpp
couchbase::query_options options;
options.consistent_with(couchbase::mutation_state{});
options.timeout(std::chrono::seconds(30));

auto [success, results] = couchbase_client.Query(query, options);
```

### Scope-level Query

```cpp
std::string bucket = "my_bucket";
std::string scope = "my_scope";
std::string query = "SELECT * FROM my_collection WHERE type = 'user'";

auto [success, results] = couchbase_client.Query(query, bucket, scope);
```

## API Reference

### Initialization
```cpp
bool InitCouchbase(const std::string& connection_string, 
                   const std::string& username, 
                   const std::string& password);
```
Initializes connection to Couchbase cluster. Must be called before any operations.

### CRUD Operations
```cpp
// Insert document (fails if exists)
bool CouchbaseAdd(const std::string& key, 
                  const std::string& value, 
                  const std::string& bucket_name, 
                  const std::string& scope = "_default", 
                  const std::string& collection = "_default");

// Insert or update document
bool CouchbaseUpsert(const std::string& key, 
                     const std::string& value, 
                     const std::string& bucket_name, 
                     const std::string& scope = "_default", 
                     const std::string& collection = "_default");

// Retrieve document
std::pair<bool, std::string> CouchbaseGet(const std::string& key, 
                                           const std::string& bucket_name, 
                                           const std::string& scope = "_default", 
                                           const std::string& collection = "_default");

// Delete document
bool CouchbaseRemove(const std::string& key, 
                     const std::string& bucket_name, 
                     const std::string& scope = "_default", 
                     const std::string& collection = "_default");
```

### Query Operations
```cpp
// Basic cluster query
std::pair<bool, std::vector<std::string>> Query(std::string statement);

// Query with options
std::pair<bool, std::vector<std::string>> Query(std::string statement, 
                                                 couchbase::query_options& q_opts);

// Scope-level query
std::pair<bool, std::vector<std::string>> Query(std::string statement, 
                                                 const std::string& bucket_name, 
                                                 const std::string& scope = "_default");

// Scope-level query with options
std::pair<bool, std::vector<std::string>> Query(std::string statement, 
                                                 const std::string& bucket_name, 
                                                 const std::string& scope, 
                                                 couchbase::query_options& q_opts);
```

### Connection Management
```cpp
void CloseCouchbase();
```
Closes the Couchbase connection. Called automatically in destructor.

## Error Handling

All operations return boolean values or pairs indicating success/failure:

- **CRUD operations**: Return `true` on success, `false` on failure
- **Get operation**: Returns `std::pair<bool, std::string>` where the boolean indicates success and string contains the document or error message
- **Query operations**: Return `std::pair<bool, std::vector<std::string>>` where boolean indicates success and vector contains results

Error messages are logged using brpc's logging system and include detailed error codes and descriptions.

## Performance Considerations

- **Connection reuse**: Initialize once and reuse the same `CouchbaseWrapper` instance
- **Thread safety**: The implementation is thread-safe and can be used across multiple threads
- **JSON serialization**: Uses efficient tao::json library for JSON operations
- **Connection pooling**: Underlying Couchbase SDK handles connection pooling automatically

## Example Usage

For complete working examples, see:
- `example/couchbase_example/client.cpp` - Single-threaded comprehensive example
- `example/couchbase_example/multi_threaded_client.cpp` - Multi-threaded usage patterns

## Dependencies

- Couchbase C++ SDK v3.x
- tao::json for JSON handling
- fmt library for formatting
- brpc framework for logging and utilities
