#include <gflags/gflags.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <utility>
#include <iomanip>
#include "brpc/couchbase.h"

DEFINE_string(couchbase_host, "localhost", "Couchbase server host");
DEFINE_string(username, "Administrator", "Couchbase username");
DEFINE_string(password, "password", "Couchbase password");
DEFINE_string(bucket, "testing", "Couchbase bucket name");

int main(int argc, char* argv[]) {
    // Parse command line flags
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    
    // Variables to store operation timings
    std::vector<std::pair<std::string, long long>> operation_times;
    
    std::cout << "Starting Couchbase example" << std::endl;
    
    // Initialize Couchbase connection
    std::cout << "Initializing Couchbase connection..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    std::string connection_string = "couchbase://" + FLAGS_couchbase_host;
    if (!brpc::InitCouchbase(connection_string, FLAGS_username, FLAGS_password, FLAGS_bucket)) {
        std::cerr << "Failed to initialize Couchbase" << std::endl;
        return -1;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Couchbase initialization completed in " << duration.count() << " ms" << std::endl;
    operation_times.push_back({"Couchbase initialization", duration.count() * 1000}); // Convert to microseconds for consistency
    
    // Example 1: Store user data using Add (insert only)
    std::cout << "\nAdding user data (insert only)..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    std::string user_data = R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
    if (brpc::CouchbaseAdd("user::john_doe", user_data)) {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "User data added successfully in " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Add user data (first attempt)", micro_duration.count()});
    } else {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cerr << "Failed to add user data (document may already exist) - took " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Add user data (first attempt - failed)", micro_duration.count()});
    }
    
    // Example 2: Try to add the same document again (should fail)
    std::cout << "\nTrying to add the same user data again (should fail)..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    if (brpc::CouchbaseAdd("user::john_doe", user_data)) {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "User data added successfully (unexpected) - took " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Add user data (second attempt - unexpected success)", micro_duration.count()});
    } else {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Add operation failed as expected - document already exists - took " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Add user data (second attempt - expected failure)", micro_duration.count()});
    }
    
    // Example 3: Use Upsert to update existing document
    std::cout << "\nUpdating user data using Upsert..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    std::string updated_user_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
    if (brpc::CouchbaseUpsert("user::john_doe", updated_user_data)) {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "User data updated successfully with Upsert in " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Upsert user data", micro_duration.count()});
    } else {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cerr << "Failed to update user data - took " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Upsert user data (failed)", micro_duration.count()});
    }
    
    // Example 4: Retrieve the updated data
    std::cout << "\nRetrieving updated user data..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    std::string retrieved_data = brpc::CouchbaseGet("user::john_doe");
    end = std::chrono::high_resolution_clock::now();
    auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    if (!retrieved_data.empty()) {
        std::cout << "Retrieved updated user data in " << micro_duration.count() << " μs: " << retrieved_data << std::endl;
        operation_times.push_back({"Get user data", micro_duration.count()});
    } else {
        std::cerr << "Failed to retrieve user data - took " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Get user data (failed)", micro_duration.count()});
    }
    
    // Example 5: Store multiple documents using Add and Upsert
    std::cout << "\nStoring multiple documents..." << std::endl;
    for (int i = 1; i <= 3; i++) {
        std::string key = "item::" + std::to_string(i);
        std::string value = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
        
        // First try Add (insert only)
        start = std::chrono::high_resolution_clock::now();
        if (brpc::CouchbaseAdd(key, value)) {
            end = std::chrono::high_resolution_clock::now();
            auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "Added " << key << " using Add operation in " << micro_duration.count() << " μs" << std::endl;
            operation_times.push_back({"Add " + key, micro_duration.count()});
        } else {
            end = std::chrono::high_resolution_clock::now();
            auto add_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            operation_times.push_back({"Add " + key + " (failed)", add_duration.count()});
            
            // If Add fails, try Upsert
            start = std::chrono::high_resolution_clock::now();
            if (brpc::CouchbaseUpsert(key, value)) {
                end = std::chrono::high_resolution_clock::now();
                auto upsert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                std::cout << "Updated " << key << " using Upsert operation in " << upsert_duration.count() << " μs (Add failed in " << add_duration.count() << " μs)" << std::endl;
                operation_times.push_back({"Upsert " + key + " (fallback)", upsert_duration.count()});
            } else {
                end = std::chrono::high_resolution_clock::now();
                auto upsert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                std::cerr << "Failed to store " << key << " - Add: " << add_duration.count() << " μs, Upsert: " << upsert_duration.count() << " μs" << std::endl;
                operation_times.push_back({"Upsert " + key + " (failed fallback)", upsert_duration.count()});
            }
        }
    }
    // Example 7: Remove a document
    std::cout << "\nRemoving document..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    if (brpc::CouchbaseRemove("item::1")) {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Document removed successfully in " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Remove item::1", micro_duration.count()});
    } else {
        end = std::chrono::high_resolution_clock::now();
        auto micro_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cerr << "Failed to remove document - took " << micro_duration.count() << " μs" << std::endl;
        operation_times.push_back({"Remove item::1 (failed)", micro_duration.count()});
    }
    
    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    brpc::CloseCouchbase();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Couchbase connection closed in " << duration.count() << " ms" << std::endl;
    operation_times.push_back({"Couchbase cleanup", duration.count() * 1000}); // Convert to microseconds for consistency
    
    // Display operation timing summary
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "OPERATION TIMING SUMMARY" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    long long total_time = 0;
    for (const auto& op : operation_times) {
        std::cout << std::left << std::setw(40) << op.first << ": ";
        if (op.second >= 1000) {
            std::cout << std::right << std::setw(8) << (op.second / 1000.0) << " ms" << std::endl;
        } else {
            std::cout << std::right << std::setw(8) << op.second << " μs" << std::endl;
        }
        total_time += op.second;
    }
    
    std::cout << std::string(60, '-') << std::endl;
    std::cout << std::left << std::setw(40) << "TOTAL EXECUTION TIME" << ": ";
    if (total_time >= 1000) {
        std::cout << std::right << std::setw(8) << (total_time / 1000.0) << " ms" << std::endl;
    } else {
        std::cout << std::right << std::setw(8) << total_time << " μs" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\nExample completed" << std::endl;
    return 0;
}
