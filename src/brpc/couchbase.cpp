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
            fmt::println(RED_TEXT "Failed to connect to cluster: {}" RESET_TEXT , connect_err);
            return false;
        }
        g_cluster = std::move(cluster);
        g_initialized = true;
        return true;
    } catch (const std::exception& ex) {
        cerr << RED_TEXT << "Failed to initialize Couchbase: " << ex.what() << RESET_TEXT;
        return false;
    }
}

std::pair<bool, std::string> CouchbaseObject::CouchbaseGet(const std::string& key, const std::string& bucket_name, const std::string& scope, const std::string& collection) {
    if (!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        // Return an error pair indicating failure
        return {false, "Couchbase not initialized"};
    }
    try {
        // Use Couchbase C++ SDK to get document
        auto [err, result] = g_cluster->bucket(bucket_name).scope(scope).collection(collection).get(key).get();

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

bool CouchbaseObject::CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name, const std::string& scope, const std::string& collection) {

    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return false;
    }
    try {
        
        auto content = tao::json::from_string(value);
        
        // Use Couchbase C++ SDK to upsert document
        auto [err, result] = g_cluster->bucket(bucket_name).scope(scope).collection(collection).upsert(key, content).get();

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

bool CouchbaseObject::CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name, const std::string& scope, const std::string& collection) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return false;
    }
    try {
        auto content = tao::json::from_string(value);
        
        // Use Couchbase C++ SDK to insert document (add operation)
        auto [err, result] = g_cluster->bucket(bucket_name).scope(scope).collection(collection).insert(key, content).get();

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

bool CouchbaseObject::CouchbaseRemove(const std::string& key, const std::string& bucket_name, const std::string& scope, const std::string& collection) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return false;
    }
    try {
        // Use Couchbase C++ SDK to remove document
        auto [err, result] = g_cluster->bucket(bucket_name).scope(scope).collection(collection).remove(key).get();

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

vector<std::string> query_result_parser(couchbase::query_result &result){
    vector<string> rows;
    for (const auto& row : result.rows_as<couchbase::codec::tao_json_serializer>()) {
        rows.push_back(tao::json::to_string(row));
    }
    return rows;
}

//query operation at cluster level with query options
std::pair<bool,vector<std::string>> CouchbaseObject::Query(std::string statement, couchbase::query_options &q_opts) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return {false, {}};
    }
    try{
        // Use Couchbase C++ SDK to execute N1QL query
        auto [err, result] = g_cluster->query(statement, q_opts).get();

        if (err) {
            std::string error_info = fmt::format("Query failed: {} (error code: {})", 
                                                err.message(), err.ec().value());
            cerr << RED_TEXT << error_info << RESET_TEXT;
            return {false, {}};
        }
        else{
            return {true, query_result_parser(result)};
        }
    }
    catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during query operation: " << ex.what() << RESET_TEXT;
        return {false, {}};
    }
}

//Query operation at cluster level without query options
std::pair<bool,vector<std::string>> CouchbaseObject::Query(std::string statement) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return {false, {}};
    }
    try{
        // Use Couchbase C++ SDK to execute N1QL query
        auto [err, result] = g_cluster->query(statement,{}).get();

        if (err) {
            std::string error_info = fmt::format("Query failed: {} (error code: {})", 
                                                err.message(), err.ec().value());
            cerr << RED_TEXT << error_info << RESET_TEXT;
            return {false, {}};
        }
        else{
            return {true, query_result_parser(result)};
        }
    }
    catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during query operation: " << ex.what() << RESET_TEXT;
        return {false, {}};
    }
}

//Query operation at scope level without query options
std::pair<bool,vector<std::string>> CouchbaseObject::Query(std::string statement, const std::string& bucket_name, const std::string& scope_name) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return {false, {}};
    }
    //get the desired scope of the bucket
    auto scope = g_cluster->bucket(bucket_name).scope(scope_name); 
    //if the bucket or scope does not exist, it will not throw an error
    //rather the error will be thrown when the query is executed
    try{
        // Use Couchbase C++ SDK to execute N1QL query
        auto [err, result] = scope.query(statement,{}).get();

        if (err) {
            std::string error_details;
            try {
                error_details = fmt::format("{}", err);
            } catch (...) {
                // Method 2: Fallback to basic error info
                error_details = fmt::format("Error code: {}, Message: '{}'", 
                                          err.ec().value(), 
                                          err.message().empty() ? "No message provided" : err.message());
            }
            cerr << RED_TEXT << error_details << RESET_TEXT;
            return {false, {}};
        }
        else{
            return {true, query_result_parser(result)};
        }
    }
    catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during query operation: " << ex.what() << RESET_TEXT;
        return {false, {}};
    }
}

//Query operation at scope level with query options
std::pair<bool,vector<std::string>> CouchbaseObject::Query(std::string statement, const std::string& bucket_name, const std::string& scope_name, couchbase::query_options &q_opts) {
    if(!g_initialized) {
        cerr << RED_TEXT << "Couchbase not initialized" << RESET_TEXT;
        return {false, {}};
    }
    //get the desired scope of the bucket
    auto scope = g_cluster->bucket(bucket_name).scope(scope_name); 
    //if the bucket or scope does not exist, it will not throw an error
    //rather the error will be thrown when the query is executed
    try{
        auto [err, result] = scope.query(statement,q_opts).get();

        if (err) {
            std::string error_info = fmt::format("Query failed: {} (error code: {})", 
                                                err.message(), err.ec().value());
            cerr << RED_TEXT << error_info << RESET_TEXT;
            return {false, {}};
        }
        else{
            return {true, query_result_parser(result)};
        }
    }
    catch (const std::exception& ex) {
        cerr << RED_TEXT << "Exception during query operation: " << ex.what() << RESET_TEXT;
        return {false, {}};
    }
}

void CouchbaseObject::CloseCouchbase() {
    if (g_initialized) {
        g_cluster.reset();
        g_initialized = false;
    }
}

} // namespace brpc