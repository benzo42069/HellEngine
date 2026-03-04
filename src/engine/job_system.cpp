#include <engine/job_system.h>

namespace engine {

JobSystem::JobSystem(std::size_t workerCount) {
    if (workerCount == 0) {
        workerCount = 1;
    }

    workers_.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i) {
        workers_.emplace_back([this]() { workerLoop(); });
    }
}

JobSystem::~JobSystem() {
    stop_ = true;
    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void JobSystem::waitIdle() {
    std::unique_lock lock(mutex_);
    idleCv_.wait(lock, [this]() {
        return jobs_.empty() && activeWorkers_.load() == 0;
    });
}

void JobSystem::workerLoop() {
    while (true) {
        std::function<void()> job;

        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this]() {
                return stop_.load() || !jobs_.empty();
            });

            if (stop_.load() && jobs_.empty()) {
                return;
            }

            job = std::move(jobs_.front());
            jobs_.pop();
            activeWorkers_.fetch_add(1);
        }

        job();

        {
            std::scoped_lock lock(mutex_);
            activeWorkers_.fetch_sub(1);
            if (jobs_.empty() && activeWorkers_.load() == 0) {
                idleCv_.notify_all();
            }
        }
    }
}

} // namespace engine
