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

#include <gflags/gflags.h>

#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <iomanip>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

#include "brpc/couchbase.h"

DEFINE_string(couchbase_host, "couchbase://localhost", "Couchbase server host");
DEFINE_string(username, "Administrator", "Couchbase username");
DEFINE_string(password, "password", "Couchbase password");
DEFINE_string(bucket, "testing", "Couchbase bucket name");



int main(int argc, char* argv[]) {
  // Parse command line flags
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

  // Create CouchbaseWrapper instance
  brpc::CouchbaseWrapper couchbase_client;

  // Variables to store operation timings
  std::vector<std::pair<std::string, long long>> operation_times;

  std::cout << "Starting Couchbase example" << std::endl;

  // Initialize Couchbase connection
  std::cout << "Initializing Couchbase connection..." << std::endl;
  std::string connection_string = FLAGS_couchbase_host;
  auto start = std::chrono::high_resolution_clock::now();
  if (!couchbase_client.InitCouchbase(connection_string, FLAGS_username,
                                      FLAGS_password)) {
    std::cerr << "Failed to initialize Couchbase" << std::endl;
    return -1;
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Couchbase initialization completed in " << duration.count()
            << " ms" << std::endl;
  operation_times.push_back(
      {"Couchbase initialization",
       duration.count() * 1000});  // Convert to microseconds for consistency

  // Example 1: Store user data using Add (insert only)
  std::cout << "\nAdding user data (insert only)..." << std::endl;
  std::string user_data =
      R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
  start = std::chrono::high_resolution_clock::now();
  auto add_response = couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket);
  end = std::chrono::high_resolution_clock::now();
  auto micro_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (add_response.success) {
    std::cout << "User data added successfully in " << micro_duration.count()
              << " μs" << std::endl;
    operation_times.push_back({"Add user data (first attempt)", micro_duration.count()});
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
  start = std::chrono::high_resolution_clock::now();
  add_response = couchbase_client.CouchbaseAdd("user::john_doe", user_data, FLAGS_bucket);
  end = std::chrono::high_resolution_clock::now();
  micro_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (add_response.success) {
    std::cout << "User data added successfully (unexpected) - took "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back({"Add user data (second attempt - unexpected success)", micro_duration.count()});
  } else {
    std::cout << "Add operation failed as expected - took "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back({"Add user data (second attempt - expected failure)", micro_duration.count()});
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
  start = std::chrono::high_resolution_clock::now();
  auto upsert_response = couchbase_client.CouchbaseUpsert(
      "user::john_doe", updated_user_data, FLAGS_bucket);
  end = std::chrono::high_resolution_clock::now();
  micro_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (upsert_response.success) {
    std::cout << "User data updated successfully with Upsert in "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back({"Upsert user data", micro_duration.count()});
  } else {
    std::cerr << "Failed to update user data - took " << micro_duration.count()
              << " μs" << std::endl;
    operation_times.push_back({"Upsert user data (failed)", micro_duration.count()});
    if (upsert_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for update" << std::endl;
    }
    if (!upsert_response.data.empty()) {
      std::cerr << "Response data: " << upsert_response.data[0] << std::endl;
    }
  }

  // Example 4: Retrieve the updated data
  std::cout << "\nRetrieving updated user data..." << std::endl;
  start = std::chrono::high_resolution_clock::now();
  auto get_response = couchbase_client.CouchbaseGet("user::john_doe", FLAGS_bucket);
  end = std::chrono::high_resolution_clock::now();
  micro_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (get_response.success) {
    std::cout << "Retrieved updated user data in " << micro_duration.count()
              << " μs: ";
    if (!get_response.data.empty()) {
      std::cout << get_response.data[0];
    }
    std::cout << std::endl;
    operation_times.push_back({"Get user data", micro_duration.count()});
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
    std::string value =
        R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";

    // First try Add (insert only)
    start = std::chrono::high_resolution_clock::now();
  add_response = couchbase_client.CouchbaseAdd(key, value, FLAGS_bucket);
    end = std::chrono::high_resolution_clock::now();
    micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    if (add_response.success) {
      std::cout << "Added " << key << " using Add operation in "
                << micro_duration.count() << " μs" << std::endl;
      operation_times.push_back({"Add " + key, micro_duration.count()});
    } else {
      end = std::chrono::high_resolution_clock::now();
      auto add_duration =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      operation_times.push_back({"Add " + key + " (failed)", add_duration.count()});
      if (!add_response.data.empty()) {
        std::cerr << "Response data: " << add_response.data[0] << std::endl;
      }
      // If Add fails, try Upsert
      start = std::chrono::high_resolution_clock::now();
      auto upsert_response_multiple = couchbase_client.CouchbaseUpsert(key, value, FLAGS_bucket);
      end = std::chrono::high_resolution_clock::now();
      if (upsert_response_multiple.success) {
        auto upsert_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Updated " << key << " using Upsert operation in "
                  << upsert_duration.count() << " μs (Add failed in "
                  << add_duration.count() << " μs)" << std::endl;
        operation_times.push_back({"Upsert " + key + " (fallback)", upsert_duration.count()});
      } else {
        if (!upsert_response_multiple.data.empty()) {
          std::cerr << "Upsert response data: " << upsert_response_multiple.data[0] << std::endl;
        }
      }
    }
  }
  // Example 6: N1QL Query Operations
  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "TESTING N1QL QUERY OPERATIONS" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Query 1: Select all documents using cluster from the bucket
  std::cout << "\n1. Querying all documents from bucket '" << FLAGS_bucket
            << "'..." << std::endl;
  std::string select_all_query =
      "SELECT META().id, * FROM `" + FLAGS_bucket +
      "` WHERE META().id LIKE 'user::%' OR META().id LIKE 'item::%'";
  start = std::chrono::high_resolution_clock::now();
  auto query_response1 = couchbase_client.Query(
      select_all_query);  // this function uses the cluster level query
                          // execution

  end = std::chrono::high_resolution_clock::now();
  auto query_duration1 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  if (query_response1.success) {
    std::cout << "Query executed successfully in " << query_duration1.count()
              << " μs" << std::endl;
    std::cout << "Found " << query_response1.data.size()
              << " documents:" << std::endl;
    for (size_t i = 0; i < query_response1.data.size() && i < 5; ++i) {
      std::cout << "  Result " << (i + 1) << ": " << query_response1.data[i] << std::endl;
    }
    if (query_response1.data.size() > 5) {
      std::cout << "  ... and " << (query_response1.data.size() - 5)
                << " more results" << std::endl;
    }
    operation_times.push_back({"N1QL Query - Select All", query_duration1.count()});
  } else {
    std::cerr << "Query failed - took " << query_duration1.count() << " μs"
              << std::endl;
    operation_times.push_back({"N1QL Query - Select All (failed)", query_duration1.count()});
    if (query_response1.err.ec() == couchbase::errc::query::index_failure) {
      std::cerr << "Index not found for query" << std::endl;
    }
    if (!query_response1.data.empty()) {
      std::cerr << "Response data: " << query_response1.data[0] << std::endl;
    }
  }

  // Query 2: Test query with specific bucket and scope
  std::cout << "\n2. Testing query with explicit bucket and scope..."
            << std::endl;
  std::string scoped_query =
      "SELECT META().id, email FROM _default WHERE email LIKE '%@%'";  // Here
                                                                       // default
                                                                       // collection
                                                                       // is
                                                                       // used,
                                                                       // since
                                                                       // scope
                                                                       // is
                                                                       // already
                                                                       // specified
                                                                       // you
                                                                       // need
                                                                       // to run
                                                                       // the
                                                                       // query
                                                                       // on the
                                                                       // collection
  start = std::chrono::high_resolution_clock::now();
  auto query_response2 = couchbase_client.Query(
      scoped_query, FLAGS_bucket,
      "_default");  // here default is specifying the scope explicitly, hence
                    // this function uses the scope level query execution

  end = std::chrono::high_resolution_clock::now();
  auto query_duration6 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  if (query_response2.success) {
    std::cout << "Scoped query executed successfully in "
              << query_duration6.count() << " μs" << std::endl;
    std::cout << "Found " << query_response2.data.size()
              << " documents with email addresses:" << std::endl;
    for (const auto& result : query_response2.data) {
      std::cout << "  " << result << std::endl;
    }
    operation_times.push_back({"N1QL Query - Scoped", query_duration6.count()});
  } else {
    std::cerr << "Scoped query failed - took " << query_duration6.count()
              << " μs" << std::endl;
    operation_times.push_back({"N1QL Query - Scoped (failed)", query_duration6.count()});
    if (query_response2.err.ec() == couchbase::errc::query::index_failure) {
      std::cerr << "Index not found for query" << std::endl;
    }
    if (!query_response2.data.empty()) {
      std::cerr << "Response data: " << query_response2.data[0] << std::endl;
    }
  }

  // Example 7: Query with parameters
  std::cout << "\n Running query with query options..." << std::endl;

  // Build a mutation state for consistency
  couchbase::mutation_state consistency_state;
  // Compose the query with placeholders ex-$1
  std::string scoped_parameterized_query = R"(
        SELECT * FROM _default WHERE email = $1 LIMIT 10;
    )";
  // Configure options
  couchbase::query_options opts{};
  opts.client_context_id("my-query-ctx")
      .consistent_with(consistency_state)
      .metrics(true)
      .profile(couchbase::query_profile::phases)
      .adhoc(false);

  // add positional parameters, you can also use named parameters and other
  // query options that might be required.
  const std::vector<std::string> param = {"john"};
  for (const auto& p : param) {
    opts.add_positional_parameter(p);
  }
  start = std::chrono::high_resolution_clock::now();
  auto query_response3 = couchbase_client.Query(scoped_parameterized_query,
                                                FLAGS_bucket, "_default", opts);
  end = std::chrono::high_resolution_clock::now();
  auto query_duration7 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (query_response3.success) {
    std::cout << "Parameterized query executed successfully." << std::endl;
    std::cout << "Found " << query_response3.data.size()
              << " documents with email addresses:" << std::endl;
    for (const auto& result : query_response3.data) {
      std::cout << "  " << result << std::endl;
    }
    operation_times.push_back({"N1QL Query - Parameterized", query_duration7.count()});
  } else {
    std::cerr << "Parameterized query failed." << std::endl;
    operation_times.push_back({"N1QL Query - Parameterized (failed)", query_duration7.count()});
    if (query_response3.err.ec() == couchbase::errc::query::index_failure) {
      std::cerr << "Index not found for query" << std::endl;
    }
    if (!query_response3.data.empty()) {
      std::cerr << "Response data: " << query_response3.data[0] << std::endl;
    }
  }

  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "QUERY TESTING COMPLETED" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Example 8: Remove a document
  std::cout << "\nRemoving document..." << std::endl;
  start = std::chrono::high_resolution_clock::now();
  auto remove_response = couchbase_client.CouchbaseRemove("item::1", FLAGS_bucket);
  end = std::chrono::high_resolution_clock::now();
  micro_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (remove_response.success) {
    std::cout << "Document removed successfully";
    operation_times.push_back({"Remove item::1", micro_duration.count()});
  } else {
    if (remove_response.err.ec() == couchbase::errc::key_value::document_not_found) {
      std::cerr << "Document not found for removal" << std::endl;
    }
    if (!remove_response.data.empty()) {
      std::cerr << "Response data: " << remove_response.data[0] << std::endl;
    }
  }

  // Cleanup
  std::cout << "\nCleaning up..." << std::endl;
  start = std::chrono::high_resolution_clock::now();
  couchbase_client.CloseCouchbase();
  end = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Couchbase connection closed in " << duration.count() << " ms"
            << std::endl;
  operation_times.push_back(
      {"Couchbase cleanup",
       duration.count() * 1000});  // Convert to microseconds for consistency

  // Display operation timing summary
  std::cout << "\n" << std::string(60, '=') << std::endl;
  std::cout << "OPERATION TIMING SUMMARY" << std::endl;
  std::cout << std::string(60, '=') << std::endl;

  long long total_time = 0;
  for (const auto& op : operation_times) {
    std::cout << std::left << std::setw(40) << op.first << ": ";
    if (op.second >= 1000) {
      std::cout << std::right << std::setw(8) << (op.second / 1000.0) << " ms"
                << std::endl;
    } else {
      std::cout << std::right << std::setw(8) << op.second << " μs"
                << std::endl;
    }
    total_time += op.second;
  }

  std::cout << std::string(60, '-') << std::endl;
  std::cout << std::left << std::setw(40) << "TOTAL EXECUTION TIME" << ": ";
  if (total_time >= 1000) {
    std::cout << std::right << std::setw(8) << (total_time / 1000.0) << " ms"
              << std::endl;
  } else {
    std::cout << std::right << std::setw(8) << total_time << " μs" << std::endl;
  }
  std::cout << std::string(60, '=') << std::endl;

  std::cout << "\nExample completed" << std::endl;

  return 0;
}
