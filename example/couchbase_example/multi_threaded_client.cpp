#include <gflags/gflags.h>
#include <bthread/bthread.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <utility>
#include <iomanip>
#include "brpc/couchbase.h"

DEFINE_string(couchbase_host, "couchbases://localhost", "Couchbase server host");
DEFINE_string(username, "Administrator", "Couchbase username");
DEFINE_string(password, "password", "Couchbase password");

struct thread_parameters {
    brpc::CouchbaseObject *couchbase_client;
    std::string bucket_name;
};

//each thread will be performing these operations in different buckets using a single connection to a cluster 
void *threaded_example(void *arg) {
    thread_parameters *params = static_cast<thread_parameters *>(arg);
    brpc::CouchbaseObject *couchbase_client = params->couchbase_client;
    std::string bucket_name = params->bucket_name;

    // Example operations
    std::string key = "example_key";
    std::string value = R"({"name": "Example", "value": 42})";

    // Example 1: Store user data using Add (insert only)
    std::cout << "\nAdding user data (insert only)..." << std::endl;
    std::string user_data = R"({"name": "John Doe", "age": 30, "email": "john@example.com"})";
    if (couchbase_client->CouchbaseAdd("user::john_doe", user_data, bucket_name)) {
        std::cout << "User data added successfully" << std::endl;
    } else {
        std::cerr << "Failed to add user data " << std::endl;
    }
    
    // Example 2: Try to add the same document again (should fail)
    std::cout << "\nTrying to add the same user data again (should fail)..." << std::endl;
    if (couchbase_client->CouchbaseAdd("user::john_doe", user_data, bucket_name)) {
        std::cout << "User data added successfully (unexpected)" << std::endl;
    } else {
        std::cout << "Add operation failed as expected" << std::endl;
    }
    
    // Example 3: Use Upsert to update existing document
    std::cout << "\nUpdating user data using Upsert..." << std::endl;
    std::string updated_user_data = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
    if (couchbase_client->CouchbaseUpsert("user::john_doe", updated_user_data, bucket_name)) {
        std::cout << "User data updated successfully with Upsert" << std::endl;
    } else {
        std::cerr << "Failed to update user data" <<  std::endl;
    }
    
    // Example 4: Retrieve the updated data
    std::cout << "\nRetrieving updated user data..." << std::endl;
    auto [success, retrieved_data] = couchbase_client->CouchbaseGet("user::john_doe", bucket_name);
    if (success && !retrieved_data.empty()) {
        std::cout << "Retrieved updated user data" << std::endl;
    } else {
        std::cerr << "Failed to retrieve user data" << retrieved_data << std::endl;
    }
    
    // Example 5: Store multiple documents using Add and Upsert
    std::cout << "\nStoring multiple documents..." << std::endl;
    for (int i = 1; i <= 3; i++) {
        std::string key = "item::" + std::to_string(i);
        std::string value = R"({"name": "John Doe", "age": 31, "email": "john.doe@example.com", "updated": true})";
        if (couchbase_client->CouchbaseAdd(key, value, bucket_name)) {
            std::cout << "Added " << key << " using Add operation" << std::endl;
        } else {     
            // If Add fails, try Upsert
            if (couchbase_client->CouchbaseUpsert(key, value, bucket_name)) {
                std::cout << "Updated " << key << " using Upsert operation" << std::endl;
            } else {
                std::cerr << "Failed to store " << key << std::endl;
            }
        }
    }
    // Example 7: Remove a document
    std::cout << "\nRemoving document..." << std::endl;
    if (couchbase_client->CouchbaseRemove("item::1", bucket_name)) {
        std::cout << "Document removed successfully" << std::endl;
    } else {
        std::cerr << "Failed to remove document" << std::endl;
    }
    
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Parse command line flags
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    
    // Create CouchbaseObject instance
    brpc::CouchbaseObject couchbase_client;
    std::string connection_string = FLAGS_couchbase_host;
    if(!couchbase_client.InitCouchbase(connection_string, FLAGS_username, FLAGS_password)){
        std::cerr << "Failed to initialize Couchbase connection" << std::endl;
        return -1;
    }

    std::string bucket_name = "testing";
    thread_parameters params[5];
    for(int i = 0; i < 5; i++) {
        params[i] = {&couchbase_client, bucket_name + std::to_string(i)};
    }
    std::vector<bthread_t> bids;
    for (int i = 0; i < 5; i++) {
        bthread_t bid;
        if(bthread_start_background(&bid, nullptr, threaded_example, &params[i]) != 0) {
            std::cerr << "Failed to start thread " << i << std::endl;
            return -1;
        }
        std::cout << "Started thread " << i << " for bucket: " << params[i].bucket_name << std::endl;
        // Store the thread ID for later joining
        bids.push_back(bid);
    }
    
    // Wait for all threads to finish
    for (auto& bid : bids) {
        bthread_join(bid, nullptr);
    }
    return 0;

}