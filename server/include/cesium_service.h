#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <mutex>
#include <queue>
#include <atomic>
#include "protos/cesium_service.grpc.pb.h"
#include "thread_pool.h"

namespace cesium_server {

class CesiumServiceImpl final : public CesiumService::Service {
public:
    explicit CesiumServiceImpl(ThreadPool* thread_pool);
    ~CesiumServiceImpl() = default;

    // 更新坐标
    grpc::Status UpdateCoordinates(
        grpc::ServerContext* context,
        const CoordinatesUpdateRequest* request,
        StatusResponse* response) override;

    // 获取最新坐标
    grpc::Status GetLatestCoordinates(
        grpc::ServerContext* context,
        const CoordinatesStreamRequest* request,
        Coordinates* response) override;

    // 订阅坐标流
    grpc::Status SubscribeCoordinates(
        grpc::ServerContext* context,
        const CoordinatesStreamRequest* request,
        grpc::ServerWriter<Coordinates>* writer) override;

    // 双向坐标流
    grpc::Status StreamCoordinates(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<Coordinates, CoordinatesUpdateRequest>* stream) override;

private:
    // 线程池
    ThreadPool* thread_pool_;

    // 最新坐标
    Coordinates latest_coordinates_;
    std::mutex coordinates_mutex_;

    // 坐标更新队列
    std::queue<Coordinates> coordinates_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // 最大队列大小
    static constexpr size_t MAX_QUEUE_SIZE = 1000;

    // 清理过期数据
    void cleanupOldData();

    // 更新坐标并通知订阅者
    void updateAndNotify(const Coordinates& coordinates);
};

} // namespace cesium_server