#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool {
public:

    explicit ThreadPool(
        std::size_t thread_count
    );

    ~ThreadPool();


    ThreadPool(
        const ThreadPool&
    ) = delete;


    ThreadPool& operator=(
        const ThreadPool&
    ) = delete;


    template <typename F>
    bool enqueue(F&& task) {

        using TaskType = std::decay_t<F>;

        /*
         * std::function requires a copyable target.
         *
         * A task may contain move-only objects such as
         * std::promise, so we store the task inside a
         * shared_ptr and put a small copyable lambda into
         * the queue.
         */

        auto task_ptr =
            std::make_shared<TaskType>(
                std::forward<F>(task)
            );


        std::function<void()> wrapper =
            [task_ptr]() mutable {

                (*task_ptr)();
            };


        {
            std::lock_guard<std::mutex> lock(
                mutex_
            );

            if (stopping_) {
                return false;
            }

            tasks_.push(
                std::move(wrapper)
            );
        }


        condition_.notify_one();

        return true;
    }


    void shutdown();


private:

    std::vector<std::thread> workers_;


    std::queue<
        std::function<void()>
    > tasks_;


    std::mutex mutex_;

    std::condition_variable condition_;

    bool stopping_;


    void worker();
};