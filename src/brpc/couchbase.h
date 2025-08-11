#pragma once

#include <string>
#include <vector>
#include <optional>
#include <couchbase/cluster.hxx>

namespace brpc {

    class CouchbaseObject {
        public:
        CouchbaseObject() = default;
        ~CouchbaseObject() {
            CloseCouchbase();
        }
        // Initialize Couchbase connection (call once at startup)
        bool InitCouchbase(const std::string& connection_string, 
                        const std::string& username, 
                        const std::string& password);

        // Get document by key
        std::pair<bool, std::string> CouchbaseGet(const std::string& key, const std::string& bucket_name);

        // Upsert (insert/update) document
        bool CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name);

        // Add document (insert only, fails if document already exists)
        bool CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name);

        // Remove document
        bool CouchbaseRemove(const std::string& key, const std::string& bucket_name);

        // Execute N1QL query
        std::string CouchbaseQuery(const std::string& query);

        // Close Couchbase connection (call at shutdown)
        void CloseCouchbase();     

        private:
            std::optional<couchbase::collection> GetCollectionForBucket(const std::string& bucket_name);

        private:
            std::optional<couchbase::cluster> g_cluster;
            std::vector<couchbase::collection> g_collection;
            bool g_initialized = false;
    };
} // namespace brpc