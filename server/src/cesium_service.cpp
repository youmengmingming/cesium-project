// #include "../include/cesium_service.h"
// #include <chrono>
// #include <iostream>

// namespace cesium_server {

// CesiumServiceImpl::CesiumServiceImpl(ThreadPool* thread_pool)
//     : thread_pool_(thread_pool) {
// }

// // grpc::Status CesiumServiceImpl::UpdateCoordinates(
// //     grpc::ServerContext* context,
// //     const CoordinatesUpdateRequest* request,
// //     StatusResponse* response) {
// //     try {
// //         updateAndNotify(request->coordinates());
        
// //         response->set_success(true);
// //         response->set_message("Coordinates updated successfully");
// //         return grpc::Status::OK;
// //     }
// //     catch (const std::exception& e) {
// //         response->set_success(false);
// //         response->set_message(std::string("Failed to update coordinates: ") + e.what());
// //         return grpc::Status(grpc::StatusCode::INTERNAL, "Internal error");
// //     }
// // }

// // grpc::Status CesiumServiceImpl::GetLatestCoordinates(
// //     grpc::ServerContext* context,
// //     const CoordinatesStreamRequest* request,
// //     Coordinates* response) {
// //     try {
// //         std::lock_guard<std::mutex> lock(coordinates_mutex_);
// //         *response = latest_coordinates_;
// //         return grpc::Status::OK;
// //     }
// //     catch (const std::exception& e) {
// //         return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to get coordinates");
// //     }
// // }

// // grpc::Status CesiumServiceImpl::SubscribeCoordinates(
// //     grpc::ServerContext* context,
// //     const CoordinatesStreamRequest* request,
// //     grpc::ServerWriter<Coordinates>* writer) {
// //     try {
// //         while (!context->IsCancelled()) {
// //             std::unique_lock<std::mutex> lock(queue_mutex_);
            
// //             // Wait for new coordinate data
// //             queue_cv_.wait_for(lock, std::chrono::seconds(1),
// //                 [this] { return !coordinates_queue_.empty(); });
            
// //             // If queue is not empty, send all available coordinates
// //             while (!coordinates_queue_.empty()) {
// //                 Coordinates coords = coordinates_queue_.front();
// //                 coordinates_queue_.pop();
                
// //                 if (!writer->Write(coords)) {
// //                     return grpc::Status(grpc::StatusCode::CANCELLED, "Client subscription cancelled");
// //                 }
// //             }
// //         }
        
// //         return grpc::Status::OK;
// //     }
// //     catch (const std::exception& e) {
// //         return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to subscribe to coordinates");
// //     }
// // }

// // grpc::Status CesiumServiceImpl::StreamCoordinates(
// //     grpc::ServerContext* context,
// //     grpc::ServerReaderWriter<Coordinates, CoordinatesUpdateRequest>* stream) {
// //     try {
// //         CoordinatesUpdateRequest request;
        
// //         // Read coordinate updates from client
// //         while (stream->Read(&request)) {
// //             // Update coordinates
// //             updateAndNotify(request.coordinates());
            
// //             // Send latest coordinates to client
// //             std::lock_guard<std::mutex> lock(coordinates_mutex_);
// //             if (!stream->Write(latest_coordinates_)) {
// //                 return grpc::Status(grpc::StatusCode::CANCELLED, "Client disconnected");
// //             }
// //         }
        
// //         return grpc::Status::OK;
// //     }
// //     catch (const std::exception& e) {
// //         return grpc::Status(grpc::StatusCode::INTERNAL, "Bidirectional stream processing failed");
// //     }
// // }

// void CesiumServiceImpl::updateAndNotify(const Coordinates& coordinates) {
//     // Update latest coordinates
//     {
//         std::lock_guard<std::mutex> lock(coordinates_mutex_);
//         latest_coordinates_ = coordinates;
//     }
    
//     // Add to queue and notify subscribers
//     {
//         std::lock_guard<std::mutex> lock(queue_mutex_);
        
//         // If queue is full, clean up old data
//         if (coordinates_queue_.size() >= MAX_QUEUE_SIZE) {
//             cleanupOldData();
//         }
        
//         coordinates_queue_.push(coordinates);
//     }
    
//     queue_cv_.notify_all();
// }

// void CesiumServiceImpl::cleanupOldData() {
//     // Keep the newest half of the data in the queue
//     size_t half_size = MAX_QUEUE_SIZE / 2;
//     while (coordinates_queue_.size() > half_size) {
//         coordinates_queue_.pop();
//     }
// }

// } // namespace cesium_server