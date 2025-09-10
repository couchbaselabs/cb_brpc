// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <couchbase/cluster.hxx>
#include <optional>
#include <string>
#include <vector>

namespace brpc {

class CouchbaseResponse {
 public:
  bool success;
  std::vector<std::string> data;
  couchbase::error err;

  CouchbaseResponse(bool success, const std::vector<std::string>& data, couchbase::error err)
      : success(success), data(data), err(err) {}
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
  CouchbaseResponse Query(std::string statement);
  CouchbaseResponse Query(std::string statement,
                               couchbase::query_options& q_opts);
  CouchbaseResponse Query(std::string statement,
                               const std::string& bucket_name,
                               const std::string& scope = "_default");
  CouchbaseResponse Query(std::string statement,
                               const std::string& bucket_name,
                               const std::string& scope,
                               couchbase::query_options& q_opts);

 private:
  std::optional<couchbase::cluster> g_cluster;
  bool g_initialized = false;
};
}  // namespace brpc