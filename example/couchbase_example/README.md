# Couchbase Example for brpc

This example demonstrates how to use the Couchbase C++ SDK integration with the brpc framework.

## Prerequisites

- Couchbase C++ SDK installed (via homebrew: `brew install couchbase-cxx-client`)
- brpc library built with Couchbase support
- gflags library
- A running Couchbase server instance

## Building

Use the provided Makefile to build the example:

```bash
make
```

### Available Makefile targets:

- `make` or `make all` - Build the couchbase_client executable
- `make clean` - Remove all generated files
- `make help` - Show available targets
- `make test` - Run the client with --help to show available flags

## Running

The client supports several command-line flags:

```bash
./couchbase_client [options]
```

### Available flags:

- `--couchbase_host` - Couchbase server host (default: "localhost")
- `--username` - Couchbase username (default: "Administrator")  
- `--password` - Couchbase password (default: "password")
- `--bucket` - Couchbase bucket name (default: "testing")

### Example usage:

```bash
# Run with default settings
./couchbase_client

# Run with custom server
./couchbase_client --couchbase_host=192.168.1.100 --username=myuser --password=mypass --bucket=mybucket
```

## What the example does

The client demonstrates several Couchbase operations:

1. **Connection**: Initializes connection to Couchbase cluster
2. **Add operation**: Inserts a new document (fails if document already exists)
3. **Upsert operation**: Inserts or updates a document
4. **Get operation**: Retrieves a document by key
5. **Batch operations**: Performs multiple add/upsert operations
6. **Remove operation**: Deletes a document
7. **Cleanup**: Properly closes the Couchbase connection

## Features

- Uses brpc's built-in logging system (butil/logging.h)
- Command-line flag support via gflags
- Error handling for all Couchbase operations
- Demonstrates both Add (insert-only) and Upsert (insert/update) operations
- Example of working with JSON documents

## Notes

- The `CouchbaseQuery` function is currently commented out in the brpc Couchbase implementation
- Make sure your Couchbase server is running and accessible before running the client
- The default bucket "testing" should exist, or specify a different bucket using the --bucket flag
- The example uses JSON documents with user and item data
