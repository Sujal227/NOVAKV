#include "thread_pool.hpp"

#include <utility>

// ============================================================
// Constructor
// ============================================================

ThreadPool::ThreadPool(
    std::size_t thread_count
)
    : stopping_(false) {

    if (thread_count == 0) {
        thread_count = 1;
    }

    for (
        std::size_t i = 0;
        i < thread_count;
        ++i
    ) {

        workers_.emplace_back(
            [this]() {
                worker();
            }
        );
    }
}


// ============================================================
// Destructor
// ============================================================

ThreadPool::~ThreadPool() {

    shutdown();
}


// ============================================================
// Worker
// ============================================================

void ThreadPool::worker() {

    while (true) {

        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(
                mutex_
            );

            condition_.wait(
                lock,
                [this]() {
                    return stopping_ ||
                           !tasks_.empty();
                }
            );

            /*
             * If shutdown was requested and there are
             * no remaining tasks, this worker can exit.
             */
            if (
                stopping_ &&
                tasks_.empty()
            ) {
                return;
            }

            task =
                std::move(
                    tasks_.front()
                );

            tasks_.pop();
        }

        /*
         * Execute outside the mutex.
         *
         * This is important because other workers should
         * be able to access the queue while this task runs.
         */
        if (task) {
            task();
        }
    }
}


// ============================================================
// Shutdown
// ============================================================

void ThreadPool::shutdown() {

    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );

        /*
         * Make shutdown idempotent.
         */
        if (stopping_) {
            return;
        }

        stopping_ = true;
    }

    /*
     * Wake every worker so they can either finish
     * remaining tasks or exit.
     */
    condition_.notify_all();

    /*
     * Wait for all workers to finish.
     */
    for (
        auto& worker :
        workers_
    ) {

        if (
            worker.joinable()
        ) {

            worker.join();
        }
    }

    workers_.clear();
}