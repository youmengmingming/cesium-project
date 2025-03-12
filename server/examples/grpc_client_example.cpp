#include <grpcpp/grpcpp.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "protos/cesium_service.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace cesium_server;

class CesiumClient {
public:
    CesiumClient(std::shared_ptr<Channel> channel)
        : stub_(CesiumService::NewStub(channel)) {}

    // 更新坐标示例
    bool UpdateCoordinates(double latitude, double longitude, double altitude) {
        CoordinatesUpdateRequest request;
        auto* coords = request.mutable_coordinates();
        coords->set_latitude(latitude);
        coords->set_longitude(longitude);
        coords->set_altitude(altitude);
        coords->set_timestamp(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        request.set_client_id("example_client");

        StatusResponse response;
        ClientContext context;

        Status status = stub_->UpdateCoordinates(&context, request, &response);
        if (status.ok()) {
            std::cout << "坐标更新成功: " << response.message() << std::endl;
            return true;
        } else {
            std::cout << "坐标更新失败: " << status.error_message() << std::endl;
            return false;
        }
    }

    // 获取最新坐标示例
    bool GetLatestCoordinates() {
        CoordinatesStreamRequest request;
        request.set_client_id("example_client");

        Coordinates response;
        ClientContext context;

        Status status = stub_->GetLatestCoordinates(&context, request, &response);
        if (status.ok()) {
            std::cout << "获取最新坐标:\n"
                      << "纬度: " << response.latitude() << "\n"
                      << "经度: " << response.longitude() << "\n"
                      << "高度: " << response.altitude() << "\n"
                      << "时间戳: " << response.timestamp() << std::endl;
            return true;
        } else {
            std::cout << "获取坐标失败: " << status.error_message() << std::endl;
            return false;
        }
    }

    // 订阅坐标流示例
    void SubscribeCoordinates() {
        CoordinatesStreamRequest request;
        request.set_client_id("example_client");

        ClientContext context;
        std::unique_ptr<grpc::ClientReader<Coordinates>> reader(
            stub_->SubscribeCoordinates(&context, request));

        Coordinates coords;
        while (reader->Read(&coords)) {
            std::cout << "收到坐标更新:\n"
                      << "纬度: " << coords.latitude() << "\n"
                      << "经度: " << coords.longitude() << "\n"
                      << "高度: " << coords.altitude() << "\n"
                      << "时间戳: " << coords.timestamp() << std::endl;
        }

        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << "坐标订阅结束: " << status.error_message() << std::endl;
        }
    }

    // 双向流示例
    void StreamCoordinates() {
        ClientContext context;
        std::shared_ptr<grpc::ClientReaderWriter<CoordinatesUpdateRequest, Coordinates>> stream(
            stub_->StreamCoordinates(&context));

        // 启动接收线程
        std::thread reader([stream]() {
            Coordinates coords;
            while (stream->Read(&coords)) {
                std::cout << "收到坐标更新:\n"
                          << "纬度: " << coords.latitude() << "\n"
                          << "经度: " << coords.longitude() << "\n"
                          << "高度: " << coords.altitude() << "\n"
                          << "时间戳: " << coords.timestamp() << std::endl;
            }
        });

        // 发送坐标更新
        for (int i = 0; i < 5; ++i) {
            CoordinatesUpdateRequest request;
            auto* coords = request.mutable_coordinates();
            coords->set_latitude(39.9 + i * 0.1);
            coords->set_longitude(116.3 + i * 0.1);
            coords->set_altitude(100 + i * 10);
            coords->set_timestamp(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
            request.set_client_id("example_client");

            if (!stream->Write(request)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        stream->WritesDone();
        reader.join();

        Status status = stream->Finish();
        if (!status.ok()) {
            std::cout << "双向流结束: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<CesiumService::Stub> stub_;
};

int main(int argc, char** argv) {
    // 创建Channel连接到服务器
    std::string target_str = "localhost:50051";
    CesiumClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    std::cout << "=== 基本坐标更新示例 ===" << std::endl;
    client.UpdateCoordinates(39.9073, 116.3912, 100);

    std::cout << "\n=== 获取最新坐标示例 ===" << std::endl;
    client.GetLatestCoordinates();

    std::cout << "\n=== 订阅坐标流示例 ===" << std::endl;
    std::cout << "按Ctrl+C终止程序" << std::endl;
    client.SubscribeCoordinates();

    std::cout << "\n=== 双向流示例 ===" << std::endl;
    client.StreamCoordinates();

    return 0;
}