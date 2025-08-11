#include "brpc/couchbase.h"
#include "butil/logging.h"
#include <couchbase/cluster.hxx>
#include <fmt/format.h>              // Required for fmt::println and formatter<>
#include <couchbase/fmt/error.hxx>   // Adapter for formatting couchbase::error
#include <couchbase/fmt/cas.hxx>     // Adapter for couchbase::cas
#include <tao/json.hpp>  // Enables tao::json usage
#include <tao/pegtl.hpp> // Include necessary headers for tao::json
#include <memory>
#include <optional>
#include <couchbase/codec/tao_json_serializer.hxx>
#include <iostream>
using namespace std;

#define RED_TEXT "\033[31m"
#define RESET_TEXT "\033[0m"

namespace brpc {

bool  CouchbaseObject::InitCouchbase(const std::string& connection_string,
                   const std::string& username,
                   const std::string& password) {
    try {
        // Create cluster connection using Couchbase C++ SDK
        auto options = couchbase::cluster_options(username, password);
        auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        
        if (connect_err) {
            cout << "Failed to connect to cluster: " << connect_err.message();
            return false;
        }
        
        g_cluster = std::move(cluster);
        
        //get all the buckets inside the cluster
        auto [buckets_err, all_buckets] = g_cluster->buckets().get_all_buckets().get();
        if (buckets_err) {
            cout << "Failed to get buckets: " << buckets_err.message();
            return false;
        }

        g_collection.clear();
        for(const auto& settings : all_buckets){
            auto bucket = g_cluster->bucket(settings.name);
            auto collection = bucket.default_collection();
            g_collection.push_back(std::move(collection));
        }
        g_initialized = true;
        return true;
    } catch (const std::exception& ex) {
        cerr << RED_TEXT << "Failed to initialize Couchbase: " << ex.what() << RESET_TEXT;
        return false;
    }
}

std::optional<couchbase::collection> CouchbaseObject::GetCollectionForBucket(const std::string& bucket_name) {
    auto it = std::find_if(
        g_collection.begin(),
        g_collection.end(),
        [&bucket_name](const couchbase::collection& collection) {
            return collection.bucket_name() == bucket_name;
        });

    if (it == g_collection.end()) {
        std::cerr << RED_TEXT << "Collection not found for bucket: " << bucket_name << RESET_TEXT;
        return std::nullopt;
    }

    return *it; // copy of the collection handle
}

std::pair<bool, std::string> CouchbaseObject::CouchbaseGet(const std::string& key, const std::string& bucket_name) {
    if (!g_initialized || g_collection.empty()) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        // Return an error pair indicating failure
        return {false, "Couchbase not initialized"};
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        cerr << RED_TEXT << "Collection not found for bucket: " << bucket_name << ", Make sure the bucket exists." << RESET_TEXT;
        return {false, "Collection not found for bucket " + bucket_name + ", make sure the bucket exists"};
    }

    try {
        // Use Couchbase C++ SDK to get document
        auto [err, result] = collection->get(key).get();

        if (err) {
            std::string error_info = fmt::format("Get failed for key '{}': {} (error code: {})", 
                                                key, err.message(), err.ec().value());
            cerr << RED_TEXT << error_info << RESET_TEXT;
            return {false, "Get failed for key '" + key + "': " + err.message()};
        }
        
        // Convert content to string using raw binary data
        auto content_json = result.content_as<tao::json::value>();
        std::string content = tao::json::to_string(content_json);
        return {true, content};
    } 
    catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during get operation: " << ex.what() << RESET_TEXT;
        return {false, "Exception during get operation: " + std::string(ex.what())};
    }
}

bool CouchbaseObject::CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name) {

    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return false;
    }
    if(g_collection.empty()) {
        cerr << RED_TEXT << "No collections available, make sure Couchbase is initialized and buckets are loaded." << RESET_TEXT;
        return false;
    }
    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        cerr << RED_TEXT << "Collection not found for bucket: " << bucket_name << ", Make sure the bucket exists."<< RESET_TEXT;
        return false;
    }

    try {
        
        auto content = tao::json::from_string(value);
        
        // Use Couchbase C++ SDK to upsert document
        auto [err, result] = collection->upsert(key, content).get();

        if (err) {
            std::string error_info = fmt::format("Upsert failed for key '{}': {} (error code: {})", 
                                                key, err.message(), err.ec().value());
            cerr << RED_TEXT << error_info << RESET_TEXT;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during upsert operation: " << ex.what() << RESET_TEXT;
        return false;
    }
}

bool CouchbaseObject::CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return false;
    }
    if(g_collection.empty()) {
        cerr << RED_TEXT << "No collections available, make sure Couchbase is initialized and buckets are loaded." << RESET_TEXT;
        return false;
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        cerr << RED_TEXT << "Collection not found for bucket: " << bucket_name << ", Make sure the bucket exists." << RESET_TEXT << std::endl;
        return false;
    }

    try {
        auto content = tao::json::from_string(value);
        
        // Use Couchbase C++ SDK to insert document (add operation)
        auto [err, result] = collection->insert(key, content).get();

        if (err) {
            // Try multiple ways to get error information
            std::string error_details;
            
            // Method 1: Try to use fmt formatting directly
            try {
                error_details = fmt::format("{}", err);
            } catch (...) {
                // Method 2: Fallback to basic error info
                error_details = fmt::format("Error code: {}, Message: '{}'", 
                                          err.ec().value(), 
                                          err.message().empty() ? "No message provided" : err.message());
            }
            
            cerr << RED_TEXT << "Add failed for key '" << key << "': " << error_details << RESET_TEXT << std::endl;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during add operation: " << ex.what() << RESET_TEXT;
        return false;
    }
}

bool CouchbaseObject::CouchbaseRemove(const std::string& key, const std::string& bucket_name) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return false;
    }
    if(g_collection.empty()) {
        cerr << RED_TEXT << "No collections available, make sure Couchbase is initialized and buckets are loaded." << RESET_TEXT;
        return false;
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        cerr << RED_TEXT << "Collection not found for bucket: " << bucket_name << ", Make sure the bucket exists." << RESET_TEXT;
        return false;
    }

    try {
        // Use Couchbase C++ SDK to remove document
        auto [err, result] = collection->remove(key).get();

        if (err) {
            std::string error_info = fmt::format("Remove failed for key '{}': {} (error code: {})", 
                                                key, err.message(), err.ec().value());
            cerr << RED_TEXT << error_info << RESET_TEXT;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during remove operation: " << ex.what() << RESET_TEXT;
        return false;
    }
}

void CouchbaseObject::CloseCouchbase() {
    if (g_initialized) {
        g_cluster.reset();
        g_initialized = false;
    }
}

} // namespace brpc