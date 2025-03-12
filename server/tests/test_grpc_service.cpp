#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <chrono>
#include <memory>
#include "../include/cesium_service.h"
#include "../protos/cesium_service.grpc.pb.h"

namespace cesium_server {
namespace testing {

class CesiumServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建服务器
        thread_pool_ = std::make_unique<ThreadPool>(1);
        service_ = std::make_unique<CesiumServiceImpl>(thread_pool_.get());
        
        // 设置服务器
        grpc::ServerBuilder builder;
        builder.AddListeningPort("localhost:50052", grpc::InsecureServerCredentials());
        builder.RegisterService(service_.get());
        server_ = builder.BuildAndStart();
        
        // 创建客户端通道
        channel_ = grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials());
        stub_ = CesiumService::NewStub(channel_);
    }
    
    void TearDown() override {
        server_->Shutdown();
        server_->Wait();
    }
    
    // 辅助函数：创建坐标更新请求
    CoordinatesUpdateRequest createUpdateRequest(double lat, double lon, double alt) {
        CoordinatesUpdateRequest request;
        auto* coords = request.mutable_coordinates();
        coords->set_latitude(lat);
        coords->set_longitude(lon);
        coords->set_altitude(alt);
        coords->set_timestamp(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        request.set_client_id("test_client");
        return request;
    }
    
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<CesiumServiceImpl> service_;
    std::unique_ptr<grpc::Server> server_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<CesiumService::Stub> stub_;
};

// 测试坐标更新功能
TEST_F(CesiumServiceTest, UpdateCoordinates) {
    grpc::ClientContext context;
    StatusResponse response;
    
    // 测试正常更新
    auto request = createUpdateRequest(39.9073, 116.3912, 100.0);
    auto status = stub_->UpdateCoordinates(&context, request, &response);
    
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(response.success());
    EXPECT_FALSE(response.message().empty());
}

// 测试获取最新坐标功能
TEST_F(CesiumServiceTest, GetLatestCoordinates) {
    // 先更新一个坐标
    {
        grpc::ClientContext context;
        StatusResponse response;
        auto request = createUpdateRequest(39.9073, 116.3912, 100.0);
        stub_->UpdateCoordinates(&context, request, &response);
    }
    
    // 测试获取最新坐标
    grpc::ClientContext context;
    CoordinatesStreamRequest request;
    request.set_client_id("test_client");
    Coordinates response;
    
    auto status = stub_->GetLatestCoordinates(&context, request, &response);
    
    EXPECT_TRUE(status.ok());
    EXPECT_DOUBLE_EQ(response.latitude(), 39.9073);
    EXPECT_DOUBLE_EQ(response.longitude(), 116.3912);
    EXPECT_DOUBLE_EQ(response.altitude(), 100.0);
}

// 测试坐标订阅流
TEST_F(CesiumServiceTest, SubscribeCoordinates) {
    // 启动订阅线程
    std::atomic<int> received_count{0};
    std::thread subscriber([this, &received_count]() {
        grpc::ClientContext context;
        CoordinatesStreamRequest request;
        request.set_client_id("test_client");
        
        std::unique_ptr<grpc::ClientReader<Coordinates>> reader(
            stub_->SubscribeCoordinates(&context, request));
        
        Coordinates coords;
        while (reader->Read(&coords) && received_count < 3) {
            received_count++;
        }
    });
    
    // 发送多个坐标更新
    for (int i = 0; i < 3; ++i) {
        grpc::ClientContext context;
        StatusResponse response;
        auto request = createUpdateRequest(39.9073 + i * 0.1, 116.3912 + i * 0.1, 100.0 + i * 10);
        stub_->UpdateCoordinates(&context, request, &response);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 等待接收完成
    subscriber.join();
    EXPECT_EQ(received_count, 3);
}

// 测试双向流通信
TEST_F(CesiumServiceTest, StreamCoordinates) {
    grpc::ClientContext context;
    std::atomic<int> received_count{0};
    
    auto stream = stub_->StreamCoordinates(&context);
    
    // 启动接收线程
    std::thread reader([&stream, &received_count]() {
        Coordinates coords;
        while (stream->Read(&coords) && received_count < 3) {
            received_count++;
        }
    });
    
    // 发送坐标更新
    for (int i = 0; i < 3; ++i) {
        auto request = createUpdateRequest(39.9073 + i * 0.1, 116.3912 + i * 0.1, 100.0 + i * 10);
        EXPECT_TRUE(stream->Write(request));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    stream->WritesDone();
    reader.join();
    
    auto status = stream->Finish();
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(received_count, 3);
}

// 测试异常情况
TEST_F(CesiumServiceTest, ErrorHandling) {
    // 测试无效坐标
    grpc::ClientContext context1;
    StatusResponse response1;
    auto invalid_request = createUpdateRequest(91.0, 181.0, 0.0); // 无效的经纬度
    auto status1 = stub_->UpdateCoordinates(&context1, invalid_request, &response1);
    EXPECT_TRUE(status1.ok()); // API应该正常响应
    EXPECT_FALSE(response1.success()); // 但更新应该失败
    
    // 测试服务器关闭时的行为
    server_->Shutdown();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    grpc::ClientContext context2;
    StatusResponse response2;
    auto request = createUpdateRequest(39.9073, 116.3912, 100.0);
    auto status2 = stub_->UpdateCoordinates(&context2, request, &response2);
    EXPECT_FALSE(status2.ok()); // 应该返回错误状态
}

} // namespace testing
} // namespace cesium_server

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}