#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-function-mocker.h>
#include <gmock/gmock-spec-builders.h>
#include "xxcnc/core/web/WebServer.h"
#include "xxcnc/core/web/WebAPI.h"

using namespace testing;
using namespace xxcnc::web;
using json = nlohmann::json;

class MockWebAPI : public WebAPI {
public:
    MOCK_METHOD(StatusResponse, getSystemStatus, (), (override));
    MOCK_METHOD(bool, executeCommand, (const nlohmann::json&), (override));
    MOCK_METHOD(FileListResponse, getFileList, (const std::string&), (override));
    MOCK_METHOD(ConfigResponse, getConfig, (), (override));
    MOCK_METHOD(bool, updateConfig, (const ConfigData&), (override));
    MOCK_METHOD(FileUploadResponse, uploadFile, (const std::string&, const std::string&), (override));
    MOCK_METHOD(FileParseResponse, parseFile, (const std::string&), (override));
};

class WebServerTest : public Test {
protected:
    void SetUp() override {
        mockAPI = std::make_shared<MockWebAPI>();
        server = std::make_unique<WebServer>(mockAPI);
    }

    std::shared_ptr<MockWebAPI> mockAPI;
    std::unique_ptr<WebServer> server;
};

TEST_F(WebServerTest, GetSystemStatus_ReturnsValidStatus) {
    StatusResponse expectedStatus;
    expectedStatus.status = "ready";
    expectedStatus.errorCode = 0;
    EXPECT_CALL(*mockAPI, getSystemStatus())
        .WillOnce(Return(expectedStatus));

    server->setStatusCallback([this]() -> json {
        auto status = mockAPI->getSystemStatus();
        return json{{"status", status.status}, {"errorCode", status.errorCode}};
    });

    auto callback = server->getStatusCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()();
    EXPECT_EQ(response["status"], "ready");
    EXPECT_EQ(response["errorCode"], 0);
}

TEST_F(WebServerTest, ExecuteCommand_ValidCommand_ReturnsTrue) {
    EXPECT_CALL(*mockAPI, executeCommand(json{{"command", "G0 X100"}}))
        .WillOnce(Return(true));

    server->setCommandCallback([this](const json& cmd) -> json {
        return json{{"success", mockAPI->executeCommand(cmd)}};
    });

    auto callback = server->getCommandCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()(json{{"command", "G0 X100"}});
    EXPECT_TRUE(response["success"]);
}

TEST_F(WebServerTest, GetFileList_ValidPath_ReturnsFiles) {
    FileListResponse expectedFiles{
        {"test.nc", "program1.nc"},
        {},
        {}
    };
    EXPECT_CALL(*mockAPI, getFileList("/"))
        .WillOnce(Return(expectedFiles));

    server->setFileParseCallback([this](const std::string& path) -> json {
        auto files = mockAPI->getFileList(path);
        return json{{"files", files.files}, {"errors", files.errors}};
    });

    auto callback = server->getFileParseCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()("/");
    EXPECT_THAT(response["files"], Contains("test.nc"));
    EXPECT_THAT(response["files"], Contains("program1.nc"));
}

TEST_F(WebServerTest, GetConfig_ReturnsValidConfig) {
    ConfigResponse expectedConfig;
    expectedConfig.config = {
        std::make_pair(std::string("maxSpeed"), std::string("1000")),
        std::make_pair(std::string("acceleration"), std::string("500"))
    };
    EXPECT_CALL(*mockAPI, getConfig())
        .WillOnce(Return(expectedConfig));

    server->setConfigCallback([this](const json& request) -> json {
        if (request.empty()) {
            auto config = mockAPI->getConfig();
            return json{{"config", config.config}};
        }
        return json{{"success", mockAPI->updateConfig({request["config"]})}};
    });

    auto callback = server->getConfigCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()(json{});
    EXPECT_EQ(response["config"]["maxSpeed"], "1000");
    EXPECT_EQ(response["config"]["acceleration"], "500");
}

TEST_F(WebServerTest, UpdateConfig_ValidConfig_ReturnsTrue) {
    ConfigData newConfig;
    newConfig.config = {
        std::make_pair(std::string("maxSpeed"), std::string("2000"))
    };
    EXPECT_CALL(*mockAPI, updateConfig(newConfig))
        .WillOnce(Return(true));

    server->setConfigCallback([this](const json& request) -> json {
        return json{{"success", mockAPI->updateConfig({request["config"]})}};
    });

    auto callback = server->getConfigCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()(json{{"config", {{"maxSpeed", "2000"}}}});
    EXPECT_TRUE(response["success"]);
}

TEST_F(WebServerTest, ExecuteCommand_InvalidCommand_ReturnsFalse) {
    EXPECT_CALL(*mockAPI, executeCommand(json{{"command", "INVALID"}}))
        .WillOnce(Return(false));

    server->setCommandCallback([this](const json& cmd) -> json {
        return json{{"success", mockAPI->executeCommand(cmd)}};
    });

    auto callback = server->getCommandCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()(json{{"command", "INVALID"}});
    EXPECT_FALSE(response["success"]);
}

TEST_F(WebServerTest, GetFileList_InvalidPath_ReturnsEmpty) {
    FileListResponse emptyResponse{
        {},
        {},
        {"Path not found"}
    };
    EXPECT_CALL(*mockAPI, getFileList("/invalid"))
        .WillOnce(Return(emptyResponse));

    server->setFileParseCallback([this](const std::string& path) -> json {
        auto files = mockAPI->getFileList(path);
        json response;
        response["files"] = std::vector<std::string>();
        response["folders"] = std::vector<std::string>();
        response["errors"] = std::vector<std::string>();
        response["errors"] = files.errors;
        return response;
    });

    auto callback = server->getFileParseCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()("/invalid");
    EXPECT_TRUE(response["files"].empty());
    EXPECT_TRUE(response["folders"].empty());
    EXPECT_FALSE(response["errors"].empty());
    EXPECT_EQ(response["errors"][0].get<std::string>(), "Path not found");
}

TEST_F(WebServerTest, UpdateConfig_InvalidConfig_ReturnsFalse) {
    ConfigData invalidConfig;
    invalidConfig.config = {
        std::make_pair(std::string("invalidKey"), std::string("value"))
    };
    EXPECT_CALL(*mockAPI, updateConfig(invalidConfig))
        .WillOnce(Return(false));

    server->setConfigCallback([this](const json& request) -> json {
        return json{{"success", mockAPI->updateConfig({request["config"]})}};
    });

    auto callback = server->getConfigCallback();
    ASSERT_TRUE(callback.has_value());
    auto response = callback.value()(json{{"config", {{"invalidKey", "value"}}}});
    EXPECT_FALSE(response["success"]);
}
