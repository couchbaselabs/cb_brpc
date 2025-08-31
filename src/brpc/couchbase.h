#pragma once

#include <couchbase/cluster.hxx>
#include <optional>
#include <string>
#include <vector>

namespace brpc {

class CouchbaseResponse {
 public:
  bool success;
  std::string data;
  couchbase::error err;

  CouchbaseResponse(bool success, const std::string& data, couchbase::error err)
      : success(success), data(data), err(err) {}
};
class CouchbaseQueryResponse {
 public:
  bool success;
  std::vector<std::string> result;
  couchbase::error err;

  CouchbaseQueryResponse(bool success, const std::vector<std::string>& result,
                         couchbase::error err)
      : success(success), result(result), err(err) {}
};
class CouchbaseWrapper {
 public:
  CouchbaseWrapper() = default;
  ~CouchbaseWrapper() { CloseCouchbase(); }
  // Initialize Couchbase connection (call once at startup)
  bool InitCouchbase(const std::string& connection_string,
                     const std::string& username, const std::string& password);

  // Get document by key
  CouchbaseResponse CouchbaseGet(const std::string& key,
                                 const std::string& bucket_name,
                                 const std::string& scope = "_default",
                                 const std::string& collection = "_default");

  // Upsert (insert/update) document
  CouchbaseResponse CouchbaseUpsert(const std::string& key,
                                    const std::string& value,
                                    const std::string& bucket_name,
                                    const std::string& scope = "_default",
                                    const std::string& collection = "_default");

  // Add document (insert only, fails if document already exists)
  CouchbaseResponse CouchbaseAdd(const std::string& key,
                                 const std::string& value,
                                 const std::string& bucket_name,
                                 const std::string& scope = "_default",
                                 const std::string& collection = "_default");

  // Remove document
  CouchbaseResponse CouchbaseRemove(const std::string& key,
                                    const std::string& bucket_name,
                                    const std::string& scope = "_default",
                                    const std::string& collection = "_default");

  // Close Couchbase connection (call at shutdown)
  void CloseCouchbase();

  // query helper functions
  CouchbaseQueryResponse Query(std::string statement);
  CouchbaseQueryResponse Query(std::string statement,
                               couchbase::query_options& q_opts);
  CouchbaseQueryResponse Query(std::string statement,
                               const std::string& bucket_name,
                               const std::string& scope = "_default");
  CouchbaseQueryResponse Query(std::string statement,
                               const std::string& bucket_name,
                               const std::string& scope,
                               couchbase::query_options& q_opts);

 private:
  std::optional<couchbase::cluster> g_cluster;
  bool g_initialized = false;
};
}  // namespace brpc