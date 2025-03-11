#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <atomic>
#include <stdexcept>

namespace cesium_server {

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop_(false), active_tasks_(0) {
        for(size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });

                        if(stop_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    ++active_tasks_;
                    task();
                    --active_tasks_;
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(std::forward<F>(f), std::move(args));
            }
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if(stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return res;
    }

    // 获取当前活跃任务数
    size_t getActiveTaskCount() const {
        return active_tasks_.load();
    }

    // 获取等待队列中的任务数
    size_t getQueuedTaskCount() const {
        //std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    // 调整线程池大小
    void resize(size_t threads) {
        if (threads == 0) return;

        std::unique_lock<std::mutex> lock(queue_mutex_);
        size_t current_size = workers_.size();

        if (threads > current_size) {
            // 增加线程
            workers_.reserve(threads);
            for (size_t i = current_size; i < threads; ++i) {
                workers_.emplace_back([this] {
                    while(true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex_);
                            condition_.wait(lock, [this] {
                                return stop_ || !tasks_.empty();
                            });

                            if(stop_ && tasks_.empty()) {
                                return;
                            }

                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }

                        ++active_tasks_;
                        task();
                        --active_tasks_;
                    }
                });
            }
        }
        // 注意：目前不支持减少线程数，因为这需要更复杂的线程管理机制
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }

        condition_.notify_all();
        for(std::thread &worker: workers_) {
            if(worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    std::vector<std::thread> workers_;              // 工作线程容器
    std::queue<std::function<void()>> tasks_;       // 任务队列
    std::mutex queue_mutex_;                        // 队列互斥锁
    std::condition_variable condition_;             // 条件变量
    std::atomic<bool> stop_;                        // 停止标志
    std::atomic<size_t> active_tasks_;             // 当前活跃任务数
};

} // namespace cesium_server