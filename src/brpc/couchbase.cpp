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

namespace brpc {

// Global Couchbase objects
static std::optional<couchbase::cluster> g_cluster;
static std::vector<couchbase::collection> g_collection;
static bool g_initialized = false;

bool InitCouchbase(const std::string& connection_string,
                   const std::string& username,
                   const std::string& password) {
    try {
        // Create cluster connection using Couchbase C++ SDK
        auto options = couchbase::cluster_options(username, password);
        auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        
        if (connect_err) {
            LOG(ERROR) << "Failed to connect to cluster: " << connect_err.message();
            return false;
        }
        
        g_cluster = std::move(cluster);
        
        //get all the buckets inside the cluster
        auto [buckets_err, all_buckets] = g_cluster->buckets().get_all_buckets().get();
        if (buckets_err) {
            LOG(ERROR) << "Failed to get buckets: " << buckets_err.message();
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
        LOG(ERROR) << "Failed to initialize Couchbase: " << ex.what();
        return false;
    }
}

std::optional<couchbase::collection> GetCollectionForBucket(const std::string& bucket_name) {
    auto it = std::find_if(
        g_collection.begin(),
        g_collection.end(),
        [&bucket_name](const couchbase::collection& collection) {
            return collection.bucket_name() == bucket_name;
        });

    if (it == g_collection.end()) {
        LOG(ERROR) << "Collection not found for bucket: " << bucket_name;
        return std::nullopt;
    }

    return *it; // copy of the collection handle
}

std::string CouchbaseGet(const std::string& key, const std::string& bucket_name) {
    if (!g_initialized || g_collection.empty()) {
        LOG(ERROR) << "Couchbase not initialized";
        return "";
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        LOG(ERROR) << "Collection not found for bucket: " << bucket_name;
        return "";
    }

    try {
        // Use Couchbase C++ SDK to get document
        auto [err, result] = collection->get(key).get();

        if (err) {
            LOG(ERROR) << "Get failed for key '" << key << "': " << err.message();
            return "";
        }
        
        // Convert content to string using raw binary data
        auto content_json = result.content_as<tao::json::value>();
        std::string content = tao::json::to_string(content_json);
        return content;
        
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception during get operation: " << ex.what();
        return "";
    }
}

bool CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name) {
    if (!g_initialized || g_collection.empty()) {
        LOG(ERROR) << "Couchbase not initialized";
        return false;
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        LOG(ERROR) << "Collection not found for bucket: " << bucket_name;
        return "";
    }

    try {
        
        auto content = tao::json::from_string(value);
        
        // Use Couchbase C++ SDK to upsert document
        auto [err, result] = collection->upsert(key, content).get();

        if (err) {
            LOG(ERROR) << "Upsert failed for key '" << key << "': " << err.message();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception during upsert operation: " << ex.what();
        return false;
    }
}

bool CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name) {
    if (!g_initialized || g_collection.empty()) {
        LOG(ERROR) << "Couchbase not initialized";
        return false;
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        LOG(ERROR) << "Collection not found for bucket: " << bucket_name;
        return "";
    }

    try {
        auto content = tao::json::from_string(value);
        
        // Use Couchbase C++ SDK to insert document (add operation)
        auto [err, result] = collection->insert(key, content).get();

        if (err) {
            LOG(ERROR) << "Add failed for key '" << key << "': " << err.message();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception during add operation: " << ex.what();
        return false;
    }
}

bool CouchbaseRemove(const std::string& key, const std::string& bucket_name) {
    if (!g_initialized || g_collection.empty()) {
        LOG(ERROR) << "Couchbase not initialized";
        return false;
    }

    auto collection = GetCollectionForBucket(bucket_name);
    if (!collection) {
        LOG(ERROR) << "Collection not found for bucket: " << bucket_name;
        return "";
    }

    try {
        // Use Couchbase C++ SDK to remove document
        auto [err, result] = collection->remove(key).get();

        if (err) {
            LOG(ERROR) << "Remove failed for key '" << key << "': " << err.message();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception during remove operation: " << ex.what();
        return false;
    }
}

// std::string CouchbaseQuery(const std::string& query_string) {
//     if (!g_initialized || !g_cluster) {
//         LOG(ERROR) << "Couchbase not initialized";
//         return "";
//     }
    
//     try {
//         // Use Couchbase C++ SDK to execute N1QL query
//         auto [err, result] = g_cluster->query(query_string, {}).get();
        
//         if (err) {
//             LOG(ERROR) << "Query failed: " << err.message();
//             return "";
//         }
        
//         // Process query results into JSON array
//         std::string json_result = "[";
//         bool first = true;
        
//         auto binary_rows = result.rows_as_binary();
//         for (const auto& row : binary_rows) {
//             if (!first) {
//                 json_result += ",";
//             }
//             // Convert binary row to string
//             std::string row_str(reinterpret_cast<const char*>(row.data.data()), row.data.size());
//             json_result += row_str;
//             first = false;
//         }
//         json_result += "]";
        
//         LOG(INFO) << "Query executed successfully, returned " << binary_rows.size() << " rows";
//         return json_result;
        
//     } catch (const std::exception& ex) {
//         LOG(ERROR) << "Exception during query operation: " << ex.what();
//         return "";
//     }
// }

void CloseCouchbase() {
    if (g_initialized) {
        g_cluster.reset();
        g_initialized = false;
    }
}

} // namespace brpc
