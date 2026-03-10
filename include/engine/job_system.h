#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <stdexcept>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace engine {

class JobSystem {
  public:
    explicit JobSystem(std::size_t workerCount = std::thread::hardware_concurrency());
    ~JobSystem();

    void shutdown();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    template <typename Fn>
    auto enqueue(Fn&& fn) -> std::future<decltype(fn())> {
        using ResultType = decltype(fn());

        auto task = std::make_shared<std::packaged_task<ResultType()>>(std::forward<Fn>(fn));
        std::future<ResultType> future = task->get_future();

        {
            std::scoped_lock lock(mutex_);
            if (stop_.load()) {
                throw std::runtime_error("JobSystem is shutting down; enqueue rejected");
            }
            jobs_.push([task]() { (*task)(); });
        }

        cv_.notify_one();
        return future;
    }

    void waitIdle();

  private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idleCv_;
    std::atomic<bool> stop_ {false};
    std::atomic<std::size_t> activeWorkers_ {0};
};

} // namespace engine
