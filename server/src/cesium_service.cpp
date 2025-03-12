#include "../include/cesium_service.h"
#include <chrono>
#include <iostream>

namespace cesium_server {

CesiumServiceImpl::CesiumServiceImpl(ThreadPool* thread_pool)
    : thread_pool_(thread_pool) {
}

grpc::Status CesiumServiceImpl::UpdateCoordinates(
    grpc::ServerContext* context,
    const CoordinatesUpdateRequest* request,
    StatusResponse* response) {
    try {
        updateAndNotify(request->coordinates());
        
        response->set_success(true);
        response->set_message("坐标更新成功");
        return grpc::Status::OK;
    }
    catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("坐标更新失败: ") + e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, "内部错误");
    }
}

grpc::Status CesiumServiceImpl::GetLatestCoordinates(
    grpc::ServerContext* context,
    const CoordinatesStreamRequest* request,
    Coordinates* response) {
    try {
        std::lock_guard<std::mutex> lock(coordinates_mutex_);
        *response = latest_coordinates_;
        return grpc::Status::OK;
    }
    catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "获取坐标失败");
    }
}

grpc::Status CesiumServiceImpl::SubscribeCoordinates(
    grpc::ServerContext* context,
    const CoordinatesStreamRequest* request,
    grpc::ServerWriter<Coordinates>* writer) {
    try {
        while (!context->IsCancelled()) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // 等待新的坐标数据
            queue_cv_.wait_for(lock, std::chrono::seconds(1),
                [this] { return !coordinates_queue_.empty(); });
            
            // 如果队列不为空，发送所有可用的坐标
            while (!coordinates_queue_.empty()) {
                Coordinates coords = coordinates_queue_.front();
                coordinates_queue_.pop();
                
                if (!writer->Write(coords)) {
                    return grpc::Status(grpc::StatusCode::CANCELLED, "客户端取消订阅");
                }
            }
        }
        
        return grpc::Status::OK;
    }
    catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "订阅坐标失败");
    }
}

grpc::Status CesiumServiceImpl::StreamCoordinates(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<Coordinates, CoordinatesUpdateRequest>* stream) {
    try {
        CoordinatesUpdateRequest request;
        
        // 读取客户端发送的坐标更新
        while (stream->Read(&request)) {
            // 更新坐标
            updateAndNotify(request.coordinates());
            
            // 发送最新坐标给客户端
            std::lock_guard<std::mutex> lock(coordinates_mutex_);
            if (!stream->Write(latest_coordinates_)) {
                return grpc::Status(grpc::StatusCode::CANCELLED, "客户端断开连接");
            }
        }
        
        return grpc::Status::OK;
    }
    catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "双向流处理失败");
    }
}

void CesiumServiceImpl::updateAndNotify(const Coordinates& coordinates) {
    // 更新最新坐标
    {
        std::lock_guard<std::mutex> lock(coordinates_mutex_);
        latest_coordinates_ = coordinates;
    }
    
    // 添加到队列并通知订阅者
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // 如果队列已满，清理旧数据
        if (coordinates_queue_.size() >= MAX_QUEUE_SIZE) {
            cleanupOldData();
        }
        
        coordinates_queue_.push(coordinates);
    }
    
    queue_cv_.notify_all();
}

void CesiumServiceImpl::cleanupOldData() {
    // 保留队列中最新的一半数据
    size_t half_size = MAX_QUEUE_SIZE / 2;
    while (coordinates_queue_.size() > half_size) {
        coordinates_queue_.pop();
    }
}

} // namespace cesium_server