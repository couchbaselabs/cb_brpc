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
        std::pair<bool, std::string> CouchbaseGet(const std::string& key, const std::string& bucket_name, const std::string& scope = "_default", const std::string& collection = "_default");

        // Upsert (insert/update) document
        bool CouchbaseUpsert(const std::string& key, const std::string& value, const std::string& bucket_name, const std::string& scope = "_default", const std::string& collection = "_default");

        // Add document (insert only, fails if document already exists)
        bool CouchbaseAdd(const std::string& key, const std::string& value, const std::string& bucket_name, const std::string& scope = "_default", const std::string& collection = "_default");

        // Remove document
        bool CouchbaseRemove(const std::string& key, const std::string& bucket_name, const std::string& scope = "_default", const std::string& collection = "_default");

        // Close Couchbase connection (call at shutdown)
        void CloseCouchbase();     

        //query helper functions
        std::pair<bool, std::vector<std::string>> Query(std::string statement);
        std::pair<bool, std::vector<std::string>> Query(std::string statement, couchbase::query_options& q_opts);
        std::pair<bool, std::vector<std::string>> Query(std::string statement, const std::string& bucket_name, const std::string& scope = "_default");
        std::pair<bool, std::vector<std::string>> Query(std::string statement, const std::string& bucket_name, const std::string& scope, couchbase::query_options &q_opts);

        private:
            std::optional<couchbase::cluster> g_cluster;
            bool g_initialized = false;
    };
} // namespace brpc