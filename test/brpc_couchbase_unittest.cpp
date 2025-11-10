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

#include <brpc/couchbase.h>
#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <gtest/gtest.h>

#include <iostream>
#include <string>

namespace brpc {
DECLARE_int32(idle_timeout_second);
}

// Test configuration
#define COUCHBASE_SERVER "localhost:11210"
#define COUCHBASE_USERNAME "Administrator"
#define COUCHBASE_PASSWORD "password"
#define COUCHBASE_BUCKET "testing"
#define COUCHBASE_COLLECTION "col1"

int main(int argc, char* argv[]) {
  brpc::FLAGS_idle_timeout_second = 0;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace {

// Helper function to check if Couchbase server is available
bool IsCouchbaseServerAvailable() {
  brpc::CouchbaseOperations test_ops;
  brpc::CouchbaseOperations::Result result =
      test_ops.authenticate(COUCHBASE_USERNAME, COUCHBASE_PASSWORD,
                            COUCHBASE_SERVER, COUCHBASE_BUCKET);
  return result.success;
}

class CouchbaseTest : public testing::Test {
 protected:
  brpc::CouchbaseOperations* couchbase_ops_;
  bool server_available_;

  CouchbaseTest() : couchbase_ops_(nullptr), server_available_(false) {}

  void SetUp() override {
    server_available_ = IsCouchbaseServerAvailable();
    if (!server_available_) {
      GTEST_SKIP() << "Couchbase server not available at " << COUCHBASE_SERVER
                   << ", skipping tests";
      return;
    }

    couchbase_ops_ = new brpc::CouchbaseOperations();
    brpc::CouchbaseOperations::Result auth_result =
        couchbase_ops_->authenticate(COUCHBASE_USERNAME, COUCHBASE_PASSWORD,
                                     COUCHBASE_SERVER, COUCHBASE_BUCKET);
    ASSERT_TRUE(auth_result.success)
        << "Authentication failed: " << auth_result.error_message;
  }

  void TearDown() override {
    if (couchbase_ops_) {
      delete couchbase_ops_;
      couchbase_ops_ = nullptr;
    }
  }

  // Helper method to clean up test keys
  void CleanupKey(const std::string& key,
                  const std::string& collection = "_default") {
    if (couchbase_ops_) {
      couchbase_ops_->delete_(key, collection);
    }
  }
};

// ============================================================================
// Authentication Tests
// ============================================================================

TEST_F(CouchbaseTest, AuthenticationSuccess) {
  if (!server_available_) return;

  brpc::CouchbaseOperations ops;
  brpc::CouchbaseOperations::Result result = ops.authenticate(
      COUCHBASE_USERNAME, COUCHBASE_PASSWORD, COUCHBASE_SERVER, COUCHBASE_BUCKET);

  EXPECT_TRUE(result.success) << "Auth failed: " << result.error_message;
}

TEST_F(CouchbaseTest, AuthenticationFailureWrongPassword) {
  brpc::CouchbaseOperations ops;
  brpc::CouchbaseOperations::Result result = ops.authenticate(
      COUCHBASE_USERNAME, "wrong_password", COUCHBASE_SERVER, COUCHBASE_BUCKET);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CouchbaseTest, AuthenticationFailureWrongUsername) {
  brpc::CouchbaseOperations ops;
  brpc::CouchbaseOperations::Result result = ops.authenticate(
      "wrong_user", COUCHBASE_PASSWORD, COUCHBASE_SERVER, COUCHBASE_BUCKET);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// Bucket Selection Tests
// ============================================================================

TEST_F(CouchbaseTest, BucketSelectionSuccess) {
  if (!server_available_) return;

  brpc::CouchbaseOperations::Result result =
      couchbase_ops_->selectBucket(COUCHBASE_BUCKET);

  EXPECT_TRUE(result.success) << "Bucket selection failed: " << result.error_message;
}

TEST_F(CouchbaseTest, BucketSelectionFailureNonExistent) {
  if (!server_available_) return;

  brpc::CouchbaseOperations::Result result =
      couchbase_ops_->selectBucket("nonexistent_bucket_12345");

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// ADD Operation Tests
// ============================================================================

TEST_F(CouchbaseTest, AddOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::add_success";
  std::string value = R"({"name": "John Doe", "age": 30})";

  // Clean up first
  CleanupKey(key);

  brpc::CouchbaseOperations::Result result = couchbase_ops_->add(key, value);

  EXPECT_TRUE(result.success) << "Add failed: " << result.error_message;

  // Clean up
  CleanupKey(key);
}

TEST_F(CouchbaseTest, AddOperationFailureKeyExists) {
  if (!server_available_) return;

  std::string key = "test::add_duplicate";
  std::string value = R"({"name": "John Doe", "age": 30})";

  // Clean up first
  CleanupKey(key);

  // First add should succeed
  brpc::CouchbaseOperations::Result result1 = couchbase_ops_->add(key, value);
  EXPECT_TRUE(result1.success);

  // Second add should fail
  brpc::CouchbaseOperations::Result result2 = couchbase_ops_->add(key, value);
  EXPECT_FALSE(result2.success);
  EXPECT_FALSE(result2.error_message.empty());

  // Clean up
  CleanupKey(key);
}

TEST_F(CouchbaseTest, AddOperationWithEmptyKey) {
  if (!server_available_) return;

  std::string key = "";
  std::string value = R"({"name": "John Doe"})";

  brpc::CouchbaseOperations::Result result = couchbase_ops_->add(key, value);

  EXPECT_FALSE(result.success);
}

TEST_F(CouchbaseTest, AddOperationWithEmptyValue) {
  if (!server_available_) return;

  std::string key = "test::add_empty_value";
  std::string value = "";

  // Clean up first
  CleanupKey(key);

  brpc::CouchbaseOperations::Result result = couchbase_ops_->add(key, value);

  // Empty value should still work
  EXPECT_TRUE(result.success) << "Add with empty value failed: " << result.error_message;

  // Clean up
  CleanupKey(key);
}

// ============================================================================
// GET Operation Tests
// ============================================================================

TEST_F(CouchbaseTest, GetOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::get_success";
  std::string value = R"({"name": "Jane Doe", "age": 25})";

  // Clean up and add document
  CleanupKey(key);
  brpc::CouchbaseOperations::Result add_result = couchbase_ops_->add(key, value);
  ASSERT_TRUE(add_result.success);

  // Get the document
  brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);

  EXPECT_TRUE(get_result.success) << "Get failed: " << get_result.error_message;
  EXPECT_EQ(value, get_result.value);

  // Clean up
  CleanupKey(key);
}

TEST_F(CouchbaseTest, GetOperationFailureKeyNotFound) {
  if (!server_available_) return;

  std::string key = "test::get_nonexistent_key_12345";

  brpc::CouchbaseOperations::Result result = couchbase_ops_->get(key);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.value.empty());
}

TEST_F(CouchbaseTest, GetOperationWithEmptyKey) {
  if (!server_available_) return;

  std::string key = "";

  brpc::CouchbaseOperations::Result result = couchbase_ops_->get(key);

  EXPECT_FALSE(result.success);
}

// ============================================================================
// UPSERT Operation Tests
// ============================================================================

TEST_F(CouchbaseTest, UpsertOperationCreateNew) {
  if (!server_available_) return;

  std::string key = "test::upsert_new";
  std::string value = R"({"name": "Alice", "age": 28})";

  // Clean up first
  CleanupKey(key);

  brpc::CouchbaseOperations::Result result = couchbase_ops_->upsert(key, value);

  EXPECT_TRUE(result.success) << "Upsert create failed: " << result.error_message;

  // Verify the document was created
  brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);
  EXPECT_TRUE(get_result.success);
  EXPECT_EQ(value, get_result.value);

  // Clean up
  CleanupKey(key);
}

TEST_F(CouchbaseTest, UpsertOperationUpdateExisting) {
  if (!server_available_) return;

  std::string key = "test::upsert_update";
  std::string initial_value = R"({"name": "Bob", "age": 30})";
  std::string updated_value = R"({"name": "Bob Updated", "age": 31})";

  // Clean up and create initial document
  CleanupKey(key);
  brpc::CouchbaseOperations::Result add_result =
      couchbase_ops_->add(key, initial_value);
  ASSERT_TRUE(add_result.success);

  // Upsert to update
  brpc::CouchbaseOperations::Result upsert_result =
      couchbase_ops_->upsert(key, updated_value);

  EXPECT_TRUE(upsert_result.success)
      << "Upsert update failed: " << upsert_result.error_message;

  // Verify the document was updated
  brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);
  EXPECT_TRUE(get_result.success);
  EXPECT_EQ(updated_value, get_result.value);

  // Clean up
  CleanupKey(key);
}

// ============================================================================
// DELETE Operation Tests
// ============================================================================

TEST_F(CouchbaseTest, DeleteOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::delete_success";
  std::string value = R"({"name": "Charlie", "age": 35})";

  // Clean up and add document
  CleanupKey(key);
  brpc::CouchbaseOperations::Result add_result = couchbase_ops_->add(key, value);
  ASSERT_TRUE(add_result.success);

  // Delete the document
  brpc::CouchbaseOperations::Result delete_result = couchbase_ops_->delete_(key);

  EXPECT_TRUE(delete_result.success)
      << "Delete failed: " << delete_result.error_message;

  // Verify the document was deleted
  brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);
  EXPECT_FALSE(get_result.success);
}

TEST_F(CouchbaseTest, DeleteOperationFailureKeyNotFound) {
  if (!server_available_) return;

  std::string key = "test::delete_nonexistent_12345";

  brpc::CouchbaseOperations::Result result = couchbase_ops_->delete_(key);

  EXPECT_FALSE(result.success);
}

// ============================================================================
// Collection-Scoped Operation Tests
// ============================================================================

TEST_F(CouchbaseTest, CollectionAddOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::collection_add";
  std::string value = R"({"type": "collection", "operation": "add"})";

  // Clean up first
  CleanupKey(key, COUCHBASE_COLLECTION);

  brpc::CouchbaseOperations::Result result =
      couchbase_ops_->add(key, value, COUCHBASE_COLLECTION);

  // Note: This may fail if collection doesn't exist, which is expected
  if (result.success) {
    EXPECT_TRUE(result.success);

    // Verify
    brpc::CouchbaseOperations::Result get_result =
        couchbase_ops_->get(key, COUCHBASE_COLLECTION);
    EXPECT_TRUE(get_result.success);
    EXPECT_EQ(value, get_result.value);

    // Clean up
    CleanupKey(key, COUCHBASE_COLLECTION);
  } else {
    // Collection might not exist, which is acceptable
    GTEST_SKIP() << "Collection " << COUCHBASE_COLLECTION
                 << " not available: " << result.error_message;
  }
}

TEST_F(CouchbaseTest, CollectionGetOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::collection_get";
  std::string value = R"({"type": "collection", "operation": "get"})";

  // Clean up and add document
  CleanupKey(key, COUCHBASE_COLLECTION);
  brpc::CouchbaseOperations::Result add_result =
      couchbase_ops_->add(key, value, COUCHBASE_COLLECTION);

  if (!add_result.success) {
    GTEST_SKIP() << "Collection " << COUCHBASE_COLLECTION
                 << " not available: " << add_result.error_message;
    return;
  }

  // Get the document
  brpc::CouchbaseOperations::Result get_result =
      couchbase_ops_->get(key, COUCHBASE_COLLECTION);

  EXPECT_TRUE(get_result.success);
  EXPECT_EQ(value, get_result.value);

  // Clean up
  CleanupKey(key, COUCHBASE_COLLECTION);
}

TEST_F(CouchbaseTest, CollectionUpsertOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::collection_upsert";
  std::string value = R"({"type": "collection", "operation": "upsert"})";

  // Clean up first
  CleanupKey(key, COUCHBASE_COLLECTION);

  brpc::CouchbaseOperations::Result result =
      couchbase_ops_->upsert(key, value, COUCHBASE_COLLECTION);

  if (!result.success) {
    GTEST_SKIP() << "Collection " << COUCHBASE_COLLECTION
                 << " not available: " << result.error_message;
    return;
  }

  EXPECT_TRUE(result.success);

  // Verify
  brpc::CouchbaseOperations::Result get_result =
      couchbase_ops_->get(key, COUCHBASE_COLLECTION);
  EXPECT_TRUE(get_result.success);
  EXPECT_EQ(value, get_result.value);

  // Clean up
  CleanupKey(key, COUCHBASE_COLLECTION);
}

TEST_F(CouchbaseTest, CollectionDeleteOperationSuccess) {
  if (!server_available_) return;

  std::string key = "test::collection_delete";
  std::string value = R"({"type": "collection", "operation": "delete"})";

  // Clean up and add document
  CleanupKey(key, COUCHBASE_COLLECTION);
  brpc::CouchbaseOperations::Result add_result =
      couchbase_ops_->add(key, value, COUCHBASE_COLLECTION);

  if (!add_result.success) {
    GTEST_SKIP() << "Collection " << COUCHBASE_COLLECTION
                 << " not available: " << add_result.error_message;
    return;
  }

  // Delete the document
  brpc::CouchbaseOperations::Result delete_result =
      couchbase_ops_->delete_(key, COUCHBASE_COLLECTION);

  EXPECT_TRUE(delete_result.success);

  // Verify deletion
  brpc::CouchbaseOperations::Result get_result =
      couchbase_ops_->get(key, COUCHBASE_COLLECTION);
  EXPECT_FALSE(get_result.success);
}

// ============================================================================
// Pipeline Operation Tests
// ============================================================================

TEST_F(CouchbaseTest, PipelineBasicOperations) {
  if (!server_available_) return;

  std::string key1 = "test::pipeline_1";
  std::string key2 = "test::pipeline_2";
  std::string key3 = "test::pipeline_3";
  std::string value1 = R"({"id": 1, "operation": "pipeline_add"})";
  std::string value2 = R"({"id": 2, "operation": "pipeline_upsert"})";
  std::string value3 = R"({"id": 3, "operation": "pipeline_add"})";

  // Clean up
  CleanupKey(key1);
  CleanupKey(key2);
  CleanupKey(key3);

  // Begin pipeline
  ASSERT_TRUE(couchbase_ops_->beginPipeline());

  // Add operations to pipeline
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD,
                                               key1, value1));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(
      brpc::CouchbaseOperations::UPSERT, key2, value2));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD,
                                               key3, value3));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::GET,
                                               key1, ""));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::GET,
                                               key2, ""));

  // Execute pipeline
  std::vector<brpc::CouchbaseOperations::Result> results =
      couchbase_ops_->executePipeline();

  // Verify results
  ASSERT_EQ(5, results.size());
  EXPECT_TRUE(results[0].success) << "ADD key1 failed: " << results[0].error_message;
  EXPECT_TRUE(results[1].success) << "UPSERT key2 failed: " << results[1].error_message;
  EXPECT_TRUE(results[2].success) << "ADD key3 failed: " << results[2].error_message;
  EXPECT_TRUE(results[3].success) << "GET key1 failed: " << results[3].error_message;
  EXPECT_EQ(value1, results[3].value);
  EXPECT_TRUE(results[4].success) << "GET key2 failed: " << results[4].error_message;
  EXPECT_EQ(value2, results[4].value);

  // Clean up
  CleanupKey(key1);
  CleanupKey(key2);
  CleanupKey(key3);
}

TEST_F(CouchbaseTest, PipelineWithDeletes) {
  if (!server_available_) return;

  std::string key1 = "test::pipeline_delete_1";
  std::string key2 = "test::pipeline_delete_2";
  std::string value = R"({"operation": "pipeline_delete"})";

  // Clean up and create initial documents
  CleanupKey(key1);
  CleanupKey(key2);
  couchbase_ops_->add(key1, value);
  couchbase_ops_->add(key2, value);

  // Begin pipeline
  ASSERT_TRUE(couchbase_ops_->beginPipeline());

  // Add delete operations to pipeline
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(
      brpc::CouchbaseOperations::DELETE, key1, ""));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(
      brpc::CouchbaseOperations::DELETE, key2, ""));

  // Execute pipeline
  std::vector<brpc::CouchbaseOperations::Result> results =
      couchbase_ops_->executePipeline();

  // Verify results
  ASSERT_EQ(2, results.size());
  EXPECT_TRUE(results[0].success) << "DELETE key1 failed: " << results[0].error_message;
  EXPECT_TRUE(results[1].success) << "DELETE key2 failed: " << results[1].error_message;

  // Verify deletions
  brpc::CouchbaseOperations::Result get_result1 = couchbase_ops_->get(key1);
  brpc::CouchbaseOperations::Result get_result2 = couchbase_ops_->get(key2);
  EXPECT_FALSE(get_result1.success);
  EXPECT_FALSE(get_result2.success);
}

TEST_F(CouchbaseTest, PipelineWithCollections) {
  if (!server_available_) return;

  std::string key1 = "test::pipeline_coll_1";
  std::string key2 = "test::pipeline_coll_2";
  std::string value1 = R"({"id": 1, "collection": "pipeline"})";
  std::string value2 = R"({"id": 2, "collection": "pipeline"})";

  // Clean up
  CleanupKey(key1, COUCHBASE_COLLECTION);
  CleanupKey(key2, COUCHBASE_COLLECTION);

  // Begin pipeline
  ASSERT_TRUE(couchbase_ops_->beginPipeline());

  // Add operations to pipeline with collection
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD,
                                               key1, value1, COUCHBASE_COLLECTION));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(
      brpc::CouchbaseOperations::UPSERT, key2, value2, COUCHBASE_COLLECTION));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::GET,
                                               key1, "", COUCHBASE_COLLECTION));

  // Execute pipeline
  std::vector<brpc::CouchbaseOperations::Result> results =
      couchbase_ops_->executePipeline();

  // Verify results (may skip if collection doesn't exist)
  if (results.size() > 0 && !results[0].success) {
    GTEST_SKIP() << "Collection " << COUCHBASE_COLLECTION
                 << " not available: " << results[0].error_message;
    return;
  }

  ASSERT_EQ(3, results.size());
  EXPECT_TRUE(results[0].success);
  EXPECT_TRUE(results[1].success);
  EXPECT_TRUE(results[2].success);
  EXPECT_EQ(value1, results[2].value);

  // Clean up
  CleanupKey(key1, COUCHBASE_COLLECTION);
  CleanupKey(key2, COUCHBASE_COLLECTION);
}

TEST_F(CouchbaseTest, PipelineClearFunction) {
  if (!server_available_) return;

  // Begin pipeline
  ASSERT_TRUE(couchbase_ops_->beginPipeline());

  // Add some operations
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD,
                                               "key1", "value1"));
  ASSERT_TRUE(couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD,
                                               "key2", "value2"));

  // Verify pipeline has operations
  EXPECT_GT(couchbase_ops_->getPipelineSize(), 0);

  // Clear pipeline
  ASSERT_TRUE(couchbase_ops_->clearPipeline());

  // Verify pipeline is empty
  EXPECT_EQ(0, couchbase_ops_->getPipelineSize());
  EXPECT_FALSE(couchbase_ops_->isPipelineActive());
}

TEST_F(CouchbaseTest, PipelineStatusFunctions) {
  if (!server_available_) return;

  // Initially pipeline should not be active
  EXPECT_FALSE(couchbase_ops_->isPipelineActive());
  EXPECT_EQ(0, couchbase_ops_->getPipelineSize());

  // Begin pipeline
  ASSERT_TRUE(couchbase_ops_->beginPipeline());
  EXPECT_TRUE(couchbase_ops_->isPipelineActive());

  // Add operations and check size
  couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD, "key1",
                                  "value1");
  EXPECT_EQ(1, couchbase_ops_->getPipelineSize());

  couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::ADD, "key2",
                                  "value2");
  EXPECT_EQ(2, couchbase_ops_->getPipelineSize());

  couchbase_ops_->pipelineRequest(brpc::CouchbaseOperations::GET, "key1", "");
  EXPECT_EQ(3, couchbase_ops_->getPipelineSize());

  // Execute pipeline
  std::vector<brpc::CouchbaseOperations::Result> results =
      couchbase_ops_->executePipeline();

  // After execution, pipeline should not be active
  EXPECT_FALSE(couchbase_ops_->isPipelineActive());
  EXPECT_EQ(0, couchbase_ops_->getPipelineSize());
}

// ============================================================================
// CRUD Workflow Tests
// ============================================================================

TEST_F(CouchbaseTest, CompleteCRUDWorkflow) {
  if (!server_available_) return;

  std::string key = "test::crud_workflow";
  std::string initial_value = R"({"name": "Initial", "version": 1})";
  std::string updated_value = R"({"name": "Updated", "version": 2})";

  // Clean up
  CleanupKey(key);

  // 1. ADD (Create)
  brpc::CouchbaseOperations::Result add_result =
      couchbase_ops_->add(key, initial_value);
  ASSERT_TRUE(add_result.success) << "CREATE failed: " << add_result.error_message;

  // 2. GET (Read)
  brpc::CouchbaseOperations::Result get_result1 = couchbase_ops_->get(key);
  ASSERT_TRUE(get_result1.success) << "READ failed: " << get_result1.error_message;
  EXPECT_EQ(initial_value, get_result1.value);

  // 3. UPSERT (Update)
  brpc::CouchbaseOperations::Result upsert_result =
      couchbase_ops_->upsert(key, updated_value);
  ASSERT_TRUE(upsert_result.success) << "UPDATE failed: " << upsert_result.error_message;

  // 4. GET (Verify Update)
  brpc::CouchbaseOperations::Result get_result2 = couchbase_ops_->get(key);
  ASSERT_TRUE(get_result2.success);
  EXPECT_EQ(updated_value, get_result2.value);

  // 5. DELETE (Delete)
  brpc::CouchbaseOperations::Result delete_result = couchbase_ops_->delete_(key);
  ASSERT_TRUE(delete_result.success) << "DELETE failed: " << delete_result.error_message;

  // 6. GET (Verify Deletion)
  brpc::CouchbaseOperations::Result get_result3 = couchbase_ops_->get(key);
  EXPECT_FALSE(get_result3.success);
}

TEST_F(CouchbaseTest, MultipleDocumentsOperations) {
  if (!server_available_) return;

  std::vector<std::string> keys = {"test::multi_1", "test::multi_2",
                                   "test::multi_3"};
  std::string value = R"({"type": "multi", "operation": "test"})";

  // Clean up
  for (const auto& key : keys) {
    CleanupKey(key);
  }

  // Add multiple documents
  for (const auto& key : keys) {
    brpc::CouchbaseOperations::Result result = couchbase_ops_->add(key, value);
    EXPECT_TRUE(result.success) << "Add failed for key: " << key;
  }

  // Verify all documents
  for (const auto& key : keys) {
    brpc::CouchbaseOperations::Result result = couchbase_ops_->get(key);
    EXPECT_TRUE(result.success) << "Get failed for key: " << key;
    EXPECT_EQ(value, result.value);
  }

  // Delete all documents
  for (const auto& key : keys) {
    brpc::CouchbaseOperations::Result result = couchbase_ops_->delete_(key);
    EXPECT_TRUE(result.success) << "Delete failed for key: " << key;
  }

  // Verify deletions
  for (const auto& key : keys) {
    brpc::CouchbaseOperations::Result result = couchbase_ops_->get(key);
    EXPECT_FALSE(result.success);
  }
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

TEST_F(CouchbaseTest, LargeValueOperation) {
  if (!server_available_) return;

  std::string key = "test::large_value";
  // Create a large JSON value (approximately 1MB)
  std::string large_value = R"({"data": ")";
  large_value += std::string(1000000, 'x');
  large_value += R"("})";

  // Clean up
  CleanupKey(key);

  // Add large value
  brpc::CouchbaseOperations::Result add_result =
      couchbase_ops_->add(key, large_value);
  EXPECT_TRUE(add_result.success) << "Large value add failed: " << add_result.error_message;

  if (add_result.success) {
    // Get large value
    brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);
    EXPECT_TRUE(get_result.success);
    EXPECT_EQ(large_value, get_result.value);

    // Clean up
    CleanupKey(key);
  }
}

TEST_F(CouchbaseTest, SpecialCharactersInKey) {
  if (!server_available_) return;

  std::string key = "test::special::chars::123::!!!";
  std::string value = R"({"type": "special_chars"})";

  // Clean up
  CleanupKey(key);

  brpc::CouchbaseOperations::Result add_result = couchbase_ops_->add(key, value);
  EXPECT_TRUE(add_result.success);

  if (add_result.success) {
    brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);
    EXPECT_TRUE(get_result.success);
    EXPECT_EQ(value, get_result.value);

    CleanupKey(key);
  }
}

TEST_F(CouchbaseTest, SpecialCharactersInValue) {
  if (!server_available_) return;

  std::string key = "test::special_value";
  std::string value =
      R"({"special": "chars: !@#$%^&*()[]{}|\\/<>?~`", "unicode": "你好世界"})";

  // Clean up
  CleanupKey(key);

  brpc::CouchbaseOperations::Result add_result = couchbase_ops_->add(key, value);
  EXPECT_TRUE(add_result.success);

  if (add_result.success) {
    brpc::CouchbaseOperations::Result get_result = couchbase_ops_->get(key);
    EXPECT_TRUE(get_result.success);
    EXPECT_EQ(value, get_result.value);

    CleanupKey(key);
  }
}

}  // namespace
