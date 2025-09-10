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

#include <bthread/bthread.h>
#include <gflags/gflags.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

#include "brpc/couchbase.h"

DEFINE_string(couchbase_host, "couchbases://localhost",
              "Couchbase server host");
DEFINE_string(username, "Administrator", "Couchbase username");
DEFINE_string(password, "password", "Couchbase password");

struct thread_parameters {
  brpc::CouchbaseWrapper *couchbase_client;
  std::string bucket_name;
};

// each thread will be performing these operations in different buckets using a
// single connection to a cluster
void *threaded_example(void *arg) {
  thread_parameters *params = static_cast<thread_parameters *>(arg);
  brpc::CouchbaseWrapper *couchbase_client = params->couchbase_client;
  std::string bucket_name = params->bucket_name;

  // Example operations
  std::string key = "example_key";
  std::string value = R"({"name": "Example", "value": 42})";

  // Example 1: Store user data using Add (insert only)
  std::cout << "\nAdding user data (insert only)..." << std::endl;
  std::string user_data =
      R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
  auto add_response = couchbase_client->CouchbaseAdd("user::john_doe", user_data, bucket_name);
  if (add_response.success) {
    std::cout << "User data added successfully" << std::endl;
  } else {
    if (add_response.err.ec() == couchbase::errc::key_value::document_exists) {
      std::cerr << "Document already exists" << std::endl;
    }
    if (!add_response.data.empty()) {
      std::cerr << "Response data: " << add_response.data[0] << std::endl;
    }
  }

  // Example 2: Try to add the same document again (should fail)
  std::cout << "\nTrying to add the same user data again (should fail)..."
            << std::endl;
  add_response = couchbase_client->CouchbaseAdd("user::john_doe", user_data, bucket_name);
  if (add_response.success) {
    std::cout << "User data added successfully (unexpected)" << std::endl;
  } else {
    std::cout << "Add operation failed as expected" << std::endl;
    if (add_response.err.ec() == couchbase::errc::key_value::document_exists) {
      std::cerr << "Document already exists" << std::endl;
    }
    if (!add_response.data.empty()) {
      std::cerr << "Response data: " << add_response.data[0] << std::endl;
    }
  }

  // Example 3: Use Upsert to update existing document
  std::cout << "\nUpdating user data using Upsert..." << std::endl;
  std::string updated_user_data =
      R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
  auto upsert_response = couchbase_client->CouchbaseUpsert(
      "user::john_doe", updated_user_data, bucket_name);
  if (upsert_response.success) {
    std::cout << "User data updated successfully with Upsert" << std::endl;
  } else {
    std::cerr << "Failed to update user data" << std::endl;
    if (!upsert_response.data.empty()) {
      std::cerr << "Response data: " << upsert_response.data[0] << std::endl;
    }
  }

  // Example 4: Retrieve the updated data
  std::cout << "\nRetrieving updated user data..." << std::endl;
  auto get_response = couchbase_client->CouchbaseGet("user::john_doe", bucket_name);
  if (get_response.success) {
    std::cout << "Retrieved updated user data: ";
    if (!get_response.data.empty()) {
      std::cout << get_response.data[0];
    }
    std::cout << std::endl;
  } else {
    if (get_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for get operation" << std::endl;
    }
    if (!get_response.data.empty()) {
      std::cerr << "Response data: " << get_response.data[0] << std::endl;
    }
  }

  // Example 5: Store multiple documents using Add and Upsert
  std::cout << "\nStoring multiple documents..." << std::endl;
  for (int i = 1; i <= 3; i++) {
    std::string key = "item::" + std::to_string(i);
    std::string value = R"({"item_id": )" + std::to_string(i) + "}";
    auto add_response_multiple = couchbase_client->CouchbaseAdd(key, value, bucket_name);
    if (add_response_multiple.success) {
      std::cout << "Added " << key << " using Add operation" << std::endl;
    } else {
      if (!add_response_multiple.data.empty()) {
        std::cerr << "Response data: " << add_response_multiple.data[0] << std::endl;
      }
      auto upsert_response_multiple = couchbase_client->CouchbaseUpsert(key, value, bucket_name);
      if (upsert_response_multiple.success) {
        std::cout << "Updated " << key << " using Upsert operation" << std::endl;
      } else {
        std::cerr << "Failed to store " << key << std::endl;
        if (!upsert_response_multiple.data.empty()) {
          std::cerr << "Upsert response data: " << upsert_response_multiple.data[0] << std::endl;
        }
      }
    }
  }

  // Example 6: Remove a document
  std::cout << "\nRemoving document..." << std::endl;
  auto remove_response = couchbase_client->CouchbaseRemove("item::1", bucket_name);
  if (remove_response.success) {
    std::cout << "Document removed successfully" << std::endl;
  } else {
    if (remove_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for removal" << std::endl;
    }
    if (!remove_response.data.empty()) {
      std::cerr << "Response data: " << remove_response.data[0] << std::endl;
    }
  }

  return nullptr;
}

int main(int argc, char *argv[]) {
  // Parse command line flags
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

  // Create CouchbaseWrapper instance
  brpc::CouchbaseWrapper couchbase_client;
  std::string connection_string = FLAGS_couchbase_host;
  if (!couchbase_client.InitCouchbase(connection_string, FLAGS_username,
                                      FLAGS_password)) {
    std::cerr << "Failed to initialize Couchbase connection" << std::endl;
    return -1;
  }

  std::string bucket_name = "testing";
  thread_parameters params[5];
  for (int i = 0; i < 5; i++) {
    params[i] = {&couchbase_client, bucket_name + std::to_string(i)};
  }
  std::vector<bthread_t> bids;
  for (int i = 0; i < 5; i++) {
    bthread_t bid;
    if (bthread_start_background(&bid, nullptr, threaded_example, &params[i]) !=
        0) {
      std::cerr << "Failed to start thread " << i << std::endl;
      return -1;
    }
    std::cout << "Started thread " << i
              << " for bucket: " << params[i].bucket_name << std::endl;
    // Store the thread ID for later joining
    bids.push_back(bid);
  }

  // Wait for all threads to finish
  for (auto &bid : bids) {
    bthread_join(bid, nullptr);
  }
  return 0;
}