
[Couchbase](https://www.couchbase.com/) is a high-performance NoSQL document database. In order to access Couchbase more conveniently and make full use of bthread's capability of concurrency, brpc directly supports Couchbase operations through a C++ wrapper. Check [example/couchbase_example](https://github.com/apache/brpc/tree/master/example/couchbase_example/) for complete examples.

**NOTE**: This implementation uses the official Couchbase C++ SDK v3.x which supports both community and enterprise features of Couchbase Server.

The current implementation provides a clean C++ interface for Couchbase operations including CRUD operations and N1QL queries.

## Build
Make sure to install couchbase-cxx-client

### Prerequisites
Make sure you have these installed:
### On Linux(Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y g++ cmake make git libssl-dev zlib1g-dev libsnappy-dev libevent-dev libcurl4-openssl-dev
```
#### Install couchbase c++ sdk:-
```bash
git clone https://github.com/couchbase/couchbase-cxx-client.git
cd couchbase-cxx-client
echo -e '\ninstall(DIRECTORY couchbase/ DESTINATION include/couchbase)\ninstall(TARGETS couchbase_cxx_client DESTINATION lib)' | sudo tee -a CMakeLists.txt
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make
sudo make install
```
#### Install fmt library:-
```bash
git clone https://github.com/fmtlib/fmt.git
cd fmt
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
make
sudo make install
```
#### get JSON libraries
```bash
git clone https://github.com/taocpp/json.git
cd json/include
sudo cp -dr tao /usr/include
```

### On MacOS

```bash
brew tap couchbaselabs/homebrew-couchbase
brew install couchbase-cxx-client
brew install pegtl
```

## Files Structure

```
example/couchbase_example/
├── client.cpp                 # Single-threaded example demonstrating all operations
└── README.md                  # This documentation file

src/brpc/
├── couchbase.h                # CouchbaseWrapper class declaration
└── couchbase.cpp              # CouchbaseWrapper implementation
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
CouchbaseResponse CouchbaseAdd(const std::string& key, 
                  const std::string& value, 
                  const std::string& bucket_name, 
                  const std::string& scope = "_default", 
                  const std::string& collection = "_default");

// Insert or update document
CouchbaseResponse CouchbaseUpsert(const std::string& key, 
                     const std::string& value, 
                     const std::string& bucket_name, 
                     const std::string& scope = "_default", 
                     const std::string& collection = "_default");

// Retrieve document
CouchbaseResponse CouchbaseGet(const std::string& key, 
                                           const std::string& bucket_name, 
                                           const std::string& scope = "_default", 
                                           const std::string& collection = "_default");

// Delete document
CouchbaseResponse CouchbaseRemove(const std::string& key, 
                     const std::string& bucket_name, 
                     const std::string& scope = "_default", 
                     const std::string& collection = "_default");
```

### Query Operations
```cpp
// Basic cluster query
CouchbaseQueryResponse Query(std::string statement);

// Query with options
CouchbaseQueryResponse Query(std::string statement, 
                                                 couchbase::query_options& q_opts);

// Scope-level query
CouchbaseQueryResponse Query(std::string statement, 
                                                 const std::string& bucket_name, 
                                                 const std::string& scope = "_default");

// Scope-level query with options
CouchbaseQueryResponse Query(std::string statement, 
                                                 const std::string& bucket_name, 
                                                 const std::string& scope, 
                                                 couchbase::query_options& q_opts);
```

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
  auto add_response = couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket);
  if (add_response.success) {
    std::cout << "User data added successfully";
  } else {
    //based on the returned error do actual error handling
    if(add_response.err.ec() == couchbase::errc::key_value::document_exists){
        //handle the error
    }
  }
```

### Upsert Document

Use `Upsert` to insert a new document or update an existing one:

```cpp
  std::string updated_user_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
  auto upsert_response = couchbase_client.CouchbaseUpsert("user::john_doe", updated_user_data, FLAGS_bucket);
  if (upsert_response.success) {
    std::cout << "User data updated successfully with Upsert";
  } else {
    //based on the returned error do the error handling
    if(upsert_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for update" << std::endl;
    }
  }
```

### Retrieve Document (Get)

Use `Get` to retrieve a document by its key:

```cpp
  auto get_response = couchbase_client.CouchbaseGet("user::john_doe", FLAGS_bucket);
  if (get_response.success && !get_response.data.empty()) {
    std::cout << "Retrieved updated user data";
    std::cout << get_response.data<<std::endl;
  } else {
    //based on the returned error do the error handling
    if(get_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for get operation" << std::endl;
    }
  }
```

### Delete Document (Remove)

Use `Remove` to delete a document:

```cpp
  auto remove_response = couchbase_client.CouchbaseRemove("item::1", FLAGS_bucket);
  micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (remove_response.success) {
    std::cout << "Document removed successfully";
  } else {
    //based on the err received handle the code.
    if(remove_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for removal" << std::endl;
    }
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
couchbase_client.CouchbaseAdd("doc::key", json_data, bucket, scope, collection);

// Get document from specific scope and collection
couchbase_client.CouchbaseGet("doc::key", bucket, scope, collection);
```

**Note**: If scope and collection are not specified, they set to `"_default"`.

## N1QL Queries

The client supports N1QL queries for complex data operations:

### Cluster-level Query

```cpp
std::string query = "SELECT name, age FROM `my_bucket` WHERE age > 25";
auto queryResponse = couchbase_client.Query(query);

if (queryResponse.success) {
    for (const auto& queryResponse.result : results) {
        std::cout << "Query result: " << result;
    }
} else {
    //based on the returned error handle it
    if(query_response.err.ec() == couchbase::errc::query::index_failure) {
      std::cerr << "Index not found for query" << std::endl;
    }
}
```

### Query with Options

```cpp
couchbase::query_options options;
options.consistent_with(couchbase::mutation_state{});
options.timeout(std::chrono::seconds(30));

auto queryResponse = couchbase_client.Query(query, options);
```

### Scope-level Query

```cpp
std::string bucket = "my_bucket";
std::string scope = "my_scope";
std::string query = "SELECT * FROM my_collection WHERE type = 'user'";

auto queryResponse = couchbase_client.Query(query, bucket, scope);
```


### Connection Management
```cpp
void CloseCouchbase();
```
Closes the Couchbase connection. Called automatically in destructor.

## Error Handling

- **CRUD operations**: Return `CouchbaseResponse` object, consisting of 3 data members which are of the type `bool success`, `string data`, `couchbase::error err`
- **Query operations**: Return `CouchbaseQueryResponse` object, consisting of 3 data members which are of the type `bool success`, `vector<string> result`, `couchbase::error err`


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
