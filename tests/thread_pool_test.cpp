#include "thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include <gtest/gtest.h>


TEST(
    ThreadPoolTest,
    ExecutesTasks
) {

    ThreadPool pool(2);


    std::atomic<int> value{0};


    std::promise<void> promise;

    auto future =
        promise.get_future();


    ASSERT_TRUE(
        pool.enqueue(
            [&]() {

                value = 42;

                promise.set_value();
            }
        )
    );


    future.wait();


    EXPECT_EQ(
        value.load(),
        42
    );
}


TEST(
    ThreadPoolTest,
    ExecutesManyTasks
) {

    ThreadPool pool(4);


    std::atomic<int> counter{0};


    constexpr int task_count = 100;


    for (
        int i = 0;
        i < task_count;
        ++i
    ) {

        ASSERT_TRUE(
            pool.enqueue(
                [&counter]() {

                    counter.fetch_add(
                        1
                    );
                }
            )
        );
    }


    pool.shutdown();


    EXPECT_EQ(
        counter.load(),
        task_count
    );
}


TEST(
    ThreadPoolTest,
    TasksRunConcurrently
) {

    ThreadPool pool(4);


    std::atomic<int> running{0};

    std::atomic<int> maximum_running{0};


    std::vector<
        std::future<void>
    > futures;


    for (
        int i = 0;
        i < 4;
        ++i
    ) {

        std::promise<void> promise;

        futures.push_back(
            promise.get_future()
        );


        pool.enqueue(
            [p = std::move(promise),
             &running,
             &maximum_running]() mutable {

                int current =
                    running.fetch_add(
                        1
                    ) + 1;


                int previous =
                    maximum_running.load();


                while (
                    current > previous &&
                    !maximum_running.compare_exchange_weak(
                        previous,
                        current
                    )
                ) {
                }


                std::this_thread::sleep_for(
                    std::chrono::milliseconds(
                        100
                    )
                );


                running.fetch_sub(
                    1
                );


                p.set_value();
            }
        );
    }


    for (
        auto& future :
        futures
    ) {

        future.wait();
    }


    EXPECT_GE(
        maximum_running.load(),
        2
    );
}


TEST(
    ThreadPoolTest,
    RejectsTasksAfterShutdown
) {

    ThreadPool pool(2);


    pool.shutdown();


    EXPECT_FALSE(
        pool.enqueue(
            []() {}
        )
    );
}