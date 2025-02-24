#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "xxcnc/core/web/WebServer.h"
#include "xxcnc/core/web/WebAPI.h"

using namespace testing;
using namespace xxcnc::web;

class MockWebAPI : public WebAPI {
public:
    MOCK_METHOD(StatusResponse, getSystemStatus, (), (override));
    MOCK_METHOD(bool, executeCommand, (const std::string& command), (override));
    MOCK_METHOD(FileListResponse, getFileList, (const std::string& path), (override));
    MOCK_METHOD(ConfigResponse, getConfig, (), (override));
    MOCK_METHOD(bool, updateConfig, (const ConfigData& config), (override));
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
    StatusResponse expectedStatus{"ready", 0, {}};
    EXPECT_CALL(*mockAPI, getSystemStatus())
        .WillOnce(Return(expectedStatus));

    auto response = server->handleStatusRequest();
    EXPECT_EQ(response.status, "ready");
    EXPECT_EQ(response.errorCode, 0);
}

TEST_F(WebServerTest, ExecuteCommand_ValidCommand_ReturnsTrue) {
    EXPECT_CALL(*mockAPI, executeCommand("G0 X100"))
        .WillOnce(Return(true));

    EXPECT_TRUE(server->handleCommandRequest("G0 X100"));
}

TEST_F(WebServerTest, GetFileList_ValidPath_ReturnsFiles) {
    FileListResponse expectedFiles{{"test.nc", "program1.nc"}, {}};
    EXPECT_CALL(*mockAPI, getFileList("/"))
        .WillOnce(Return(expectedFiles));

    auto response = server->handleFileListRequest("/");
    EXPECT_THAT(response.files, Contains("/test.nc"));
    EXPECT_THAT(response.files, Contains("/program1.nc"));
}

TEST_F(WebServerTest, GetConfig_ReturnsValidConfig) {
    ConfigResponse expectedConfig;
    expectedConfig.config = {
        std::make_pair(std::string("maxSpeed"), std::string("1000")),
        std::make_pair(std::string("acceleration"), std::string("500"))
    };
    EXPECT_CALL(*mockAPI, getConfig())
        .WillOnce(Return(expectedConfig));

    auto response = server->handleConfigRequest();
    EXPECT_EQ(response.config.at("maxSpeed"), "1000");
    EXPECT_EQ(response.config.at("acceleration"), "500");
}

TEST_F(WebServerTest, UpdateConfig_ValidConfig_ReturnsTrue) {
    ConfigData newConfig;
    newConfig.config = {
        std::make_pair(std::string("maxSpeed"), std::string("2000"))
    };
    EXPECT_CALL(*mockAPI, updateConfig(newConfig))
        .WillOnce(Return(true));

    EXPECT_TRUE(server->handleConfigUpdateRequest(newConfig));
}

TEST_F(WebServerTest, ExecuteCommand_InvalidCommand_ReturnsFalse) {
    EXPECT_CALL(*mockAPI, executeCommand("INVALID"))
        .WillOnce(Return(false));

    EXPECT_FALSE(server->handleCommandRequest("INVALID"));
}

TEST_F(WebServerTest, GetFileList_InvalidPath_ReturnsEmpty) {
    FileListResponse emptyResponse{{}, {"Invalid path"}};
    EXPECT_CALL(*mockAPI, getFileList("/invalid"))
        .WillOnce(Return(emptyResponse));

    auto response = server->handleFileListRequest("/invalid");
    EXPECT_TRUE(response.files.empty());
    EXPECT_FALSE(response.errors.empty());
}

TEST_F(WebServerTest, UpdateConfig_InvalidConfig_ReturnsFalse) {
    ConfigData invalidConfig;
    invalidConfig.config = {
        std::make_pair(std::string("invalidKey"), std::string("value"))
    };
    EXPECT_CALL(*mockAPI, updateConfig(invalidConfig))
        .WillOnce(Return(false));

    EXPECT_FALSE(server->handleConfigUpdateRequest(invalidConfig));
}
