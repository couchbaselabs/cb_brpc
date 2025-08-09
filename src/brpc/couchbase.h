#pragma once

#include <string>

namespace brpc {

// Initialize Couchbase connection (call once at startup)
bool InitCouchbase(const std::string& connection_string, 
                   const std::string& username, 
                   const std::string& password);

// Get document by key
std::string CouchbaseGet(const std::string& key, const std::string& bucket_name);

// Upsert (insert/update) document
bool CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name);

// Add document (insert only, fails if document already exists)
bool CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name);

// Remove document
bool CouchbaseRemove(const std::string& key, const std::string& bucket_name);

// Execute N1QL query
std::string CouchbaseQuery(const std::string& query);

// Close Couchbase connection (call at shutdown)
void CloseCouchbase();

} // namespace brpc
