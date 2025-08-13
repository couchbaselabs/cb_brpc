#include <gflags/gflags.h>

#include <chrono>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

#include "brpc/couchbase.h"

DEFINE_string(couchbase_host, "couchbases://localhost",
              "Couchbase server host");
DEFINE_string(username, "selfdb", "Couchbase username");
DEFINE_string(password, "Selfdb@1", "Couchbase password");
DEFINE_string(bucket, "testing", "Couchbase bucket name");

int main(int argc, char* argv[]) {
  // Parse command line flags
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

  // Create CouchbaseObject instance
  brpc::CouchbaseObject couchbase_client;

  // Variables to store operation timings
  std::vector<std::pair<std::string, long long>> operation_times;

  std::cout << "Starting Couchbase example" << std::endl;

  // Initialize Couchbase connection
  std::cout << "Initializing Couchbase connection..." << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  std::string connection_string = FLAGS_couchbase_host;
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
  start = std::chrono::high_resolution_clock::now();
  std::string user_data =
      R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
  if (couchbase_client.CouchbaseAdd("user::john_doe", user_data,
                                    FLAGS_bucket)) {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "User data added successfully in " << micro_duration.count()
              << " μs" << std::endl;
    operation_times.push_back(
        {"Add user data (first attempt)", micro_duration.count()});
  } else {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cerr << "Failed to add user data (document may already exist) - took "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back(
        {"Add user data (first attempt - failed)", micro_duration.count()});
  }

  // Example 2: Try to add the same document again (should fail)
  std::cout << "\nTrying to add the same user data again (should fail)..."
            << std::endl;
  start = std::chrono::high_resolution_clock::now();
  if (couchbase_client.CouchbaseAdd("user::john_doe", user_data,
                                    FLAGS_bucket)) {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "User data added successfully (unexpected) - took "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back(
        {"Add user data (second attempt - unexpected success)",
         micro_duration.count()});
  } else {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Add operation failed as expected - took "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back(
        {"Add user data (second attempt - expected failure)",
         micro_duration.count()});
  }

  // Example 3: Use Upsert to update existing document
  std::cout << "\nUpdating user data using Upsert..." << std::endl;
  start = std::chrono::high_resolution_clock::now();
  std::string updated_user_data =
      R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
  if (couchbase_client.CouchbaseUpsert("user::john_doe", updated_user_data,
                                       FLAGS_bucket)) {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "User data updated successfully with Upsert in "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back({"Upsert user data", micro_duration.count()});
  } else {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cerr << "Failed to update user data - took " << micro_duration.count()
              << " μs" << std::endl;
    operation_times.push_back(
        {"Upsert user data (failed)", micro_duration.count()});
  }

  // Example 4: Retrieve the updated data
  std::cout << "\nRetrieving updated user data..." << std::endl;
  start = std::chrono::high_resolution_clock::now();
  auto [success, retrieved_data] =
      couchbase_client.CouchbaseGet("user::john_doe", FLAGS_bucket);
  end = std::chrono::high_resolution_clock::now();
  auto micro_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (success && !retrieved_data.empty()) {
    std::cout << "Retrieved updated user data in " << micro_duration.count()
              << " μs: " << retrieved_data << std::endl;
    operation_times.push_back({"Get user data", micro_duration.count()});
  } else {
    std::cerr << "Failed to retrieve user data - took "
              << micro_duration.count() << " μs" << std::endl;
    operation_times.push_back(
        {"Get user data (failed)", micro_duration.count()});
  }

  // Example 5: Store multiple documents using Add and Upsert
  std::cout << "\nStoring multiple documents..." << std::endl;
  for (int i = 1; i <= 3; i++) {
    std::string key = "item::" + std::to_string(i);
    std::string value =
        R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";

    // First try Add (insert only)
    start = std::chrono::high_resolution_clock::now();
    if (couchbase_client.CouchbaseAdd(key, value, FLAGS_bucket)) {
      end = std::chrono::high_resolution_clock::now();
      auto micro_duration =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      std::cout << "Added " << key << " using Add operation in "
                << micro_duration.count() << " μs" << std::endl;
      operation_times.push_back({"Add " + key, micro_duration.count()});
    } else {
      end = std::chrono::high_resolution_clock::now();
      auto add_duration =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      operation_times.push_back(
          {"Add " + key + " (failed)", add_duration.count()});

      // If Add fails, try Upsert
      start = std::chrono::high_resolution_clock::now();
      if (couchbase_client.CouchbaseUpsert(key, value, FLAGS_bucket)) {
        end = std::chrono::high_resolution_clock::now();
        auto upsert_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Updated " << key << " using Upsert operation in "
                  << upsert_duration.count() << " μs (Add failed in "
                  << add_duration.count() << " μs)" << std::endl;
        operation_times.push_back(
            {"Upsert " + key + " (fallback)", upsert_duration.count()});
      } else {
        end = std::chrono::high_resolution_clock::now();
        auto upsert_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cerr << "Failed to store " << key
                  << " - Add: " << add_duration.count()
                  << " μs, Upsert: " << upsert_duration.count() << " μs"
                  << std::endl;
        operation_times.push_back(
            {"Upsert " + key + " (failed fallback)", upsert_duration.count()});
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
  start = std::chrono::high_resolution_clock::now();
  std::string select_all_query =
      "SELECT META().id, * FROM `" + FLAGS_bucket +
      "` WHERE META().id LIKE 'user::%' OR META().id LIKE 'item::%'";

  auto [query_success1, query_results1] = couchbase_client.Query(
      select_all_query);  // this function uses the cluster level query
                          // execution

  end = std::chrono::high_resolution_clock::now();
  auto query_duration1 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  if (query_success1) {
    std::cout << "Query executed successfully in " << query_duration1.count()
              << " μs" << std::endl;
    std::cout << "Found " << query_results1.size()
              << " documents:" << std::endl;
    for (size_t i = 0; i < query_results1.size() && i < 5;
         ++i) {  // Show first 5 results
      std::cout << "  Result " << (i + 1) << ": " << query_results1[i]
                << std::endl;
    }
    if (query_results1.size() > 5) {
      std::cout << "  ... and " << (query_results1.size() - 5)
                << " more results" << std::endl;
    }
    operation_times.push_back(
        {"N1QL Query - Select All", query_duration1.count()});
  } else {
    std::cerr << "Query failed - took " << query_duration1.count() << " μs"
              << std::endl;
    operation_times.push_back(
        {"N1QL Query - Select All (failed)", query_duration1.count()});
  }

  // Query 2: Test query with specific bucket and scope
  std::cout << "\n2. Testing query with explicit bucket and scope..."
            << std::endl;
  start = std::chrono::high_resolution_clock::now();
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

  auto [query_success6, query_results6] = couchbase_client.Query(
      scoped_query, FLAGS_bucket,
      "_default");  // here default is specifying the scope explicitly, hence
                    // this function uses the scope level query execution

  end = std::chrono::high_resolution_clock::now();
  auto query_duration6 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  if (query_success6) {
    std::cout << "Scoped query executed successfully in "
              << query_duration6.count() << " μs" << std::endl;
    std::cout << "Found " << query_results6.size()
              << " documents with email addresses:" << std::endl;
    for (const auto& result : query_results6) {
      std::cout << "  " << result << std::endl;
    }
    operation_times.push_back({"N1QL Query - Scoped", query_duration6.count()});
  } else {
    std::cerr << "Scoped query failed - took " << query_duration6.count()
              << " μs" << std::endl;
    operation_times.push_back(
        {"N1QL Query - Scoped (failed)", query_duration6.count()});
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
  auto [query_success7, query_results7] = couchbase_client.Query(
      scoped_parameterized_query, FLAGS_bucket, "_default", opts);
  end = std::chrono::high_resolution_clock::now();
  auto query_duration7 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (query_success7) {
    std::cout << "Parameterized query executed successfully." << std::endl;
    std::cout << "Found " << query_results7.size()
              << " documents with email addresses:" << std::endl;
    for (const auto& result : query_results7) {
      std::cout << "  " << result << std::endl;
    }
    operation_times.push_back(
        {"N1QL Query - Parameterized", query_duration7.count()});
  } else {
    std::cerr << "Parameterized query failed." << std::endl;
    operation_times.push_back(
        {"N1QL Query - Parameterized (failed)", query_duration7.count()});
  }

  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << "QUERY TESTING COMPLETED" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Example 8: Remove a document
  std::cout << "\nRemoving document..." << std::endl;
  start = std::chrono::high_resolution_clock::now();
  if (couchbase_client.CouchbaseRemove("item::1", FLAGS_bucket)) {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Document removed successfully in " << micro_duration.count()
              << " μs" << std::endl;
    operation_times.push_back({"Remove item::1", micro_duration.count()});
  } else {
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cerr << "Failed to remove document - took " << micro_duration.count()
              << " μs" << std::endl;
    operation_times.push_back(
        {"Remove item::1 (failed)", micro_duration.count()});
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
