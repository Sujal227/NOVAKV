#include "kv_store.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>


TEST(
    KeyValueStoreTest,
    SetAndGet
) {

    KeyValueStore db;

    db.set(
        "name",
        "Sujal"
    );


    ASSERT_TRUE(
        db.get("name").has_value()
    );


    EXPECT_EQ(
        db.get("name").value(),
        "Sujal"
    );
}


TEST(
    KeyValueStoreTest,
    OverwriteExistingValue
) {

    KeyValueStore db;


    db.set(
        "name",
        "Sujal"
    );


    db.set(
        "name",
        "Nova"
    );


    EXPECT_EQ(
        db.get("name").value(),
        "Nova"
    );
}


TEST(
    KeyValueStoreTest,
    GetMissingKey
) {

    KeyValueStore db;


    EXPECT_FALSE(
        db.get("missing").has_value()
    );
}


TEST(
    KeyValueStoreTest,
    Exists
) {

    KeyValueStore db;


    EXPECT_FALSE(
        db.exists("name")
    );


    db.set(
        "name",
        "Sujal"
    );


    EXPECT_TRUE(
        db.exists("name")
    );
}


TEST(
    KeyValueStoreTest,
    Remove
) {

    KeyValueStore db;


    db.set(
        "name",
        "Sujal"
    );


    EXPECT_TRUE(
        db.remove("name")
    );


    EXPECT_FALSE(
        db.exists("name")
    );
}


TEST(
    KeyValueStoreTest,
    RemoveMissingKey
) {

    KeyValueStore db;


    EXPECT_FALSE(
        db.remove("missing")
    );
}


TEST(
    KeyValueStoreTest,
    Size
) {

    KeyValueStore db;


    EXPECT_EQ(
        db.size(),
        0
    );


    db.set("a", "1");
    db.set("b", "2");
    db.set("c", "3");


    EXPECT_EQ(
        db.size(),
        3
    );
}


TEST(
    KeyValueStoreTest,
    ConcurrentWrites
) {

    KeyValueStore db;


    constexpr int thread_count = 8;

    constexpr int writes_per_thread = 100;


    std::vector<std::thread> threads;


    for (
        int t = 0;
        t < thread_count;
        ++t
    ) {

        threads.emplace_back(
            [&db, t]() {

                for (
                    int i = 0;
                    i < writes_per_thread;
                    ++i
                ) {

                    std::string key =
                        "key_" +
                        std::to_string(t) +
                        "_" +
                        std::to_string(i);


                    db.set(
                        key,
                        "value"
                    );
                }
            }
        );
    }


    for (
        auto& thread :
        threads
    ) {

        thread.join();
    }


    EXPECT_EQ(
        db.size(),
        thread_count *
        writes_per_thread
    );
}


TEST(
    KeyValueStoreTest,
    ConcurrentReadsAndWrites
) {

    KeyValueStore db;


    db.set(
        "shared",
        "initial"
    );


    std::vector<std::thread> writers;

    std::vector<std::thread> readers;


    for (
        int i = 0;
        i < 4;
        ++i
    ) {

        writers.emplace_back(
            [&db, i]() {

                for (
                    int j = 0;
                    j < 100;
                    ++j
                ) {

                    db.set(
                        "key_" +
                        std::to_string(i) +
                        "_" +
                        std::to_string(j),
                        "value"
                    );
                }
            }
        );
    }


    for (
        int i = 0;
        i < 4;
        ++i
    ) {

        readers.emplace_back(
            [&db]() {

                for (
                    int j = 0;
                    j < 100;
                    ++j
                ) {

                    db.get(
                        "shared"
                    );

                    db.exists(
                        "shared"
                    );

                    db.size();
                }
            }
        );
    }


    for (
        auto& thread :
        writers
    ) {

        thread.join();
    }


    for (
        auto& thread :
        readers
    ) {

        thread.join();
    }


    EXPECT_TRUE(
        db.exists("shared")
    );
}


TEST(
    KeyValueStoreTest,
    TTL
) {

    KeyValueStore db;


    db.set(
        "temporary",
        "hello",
        2
    );


    EXPECT_TRUE(
        db.exists("temporary")
    );


    auto ttl =
        db.ttl(
            "temporary"
        );


    EXPECT_GE(
        ttl,
        0
    );


    EXPECT_LE(
        ttl,
        2
    );


    std::this_thread::sleep_for(
        std::chrono::seconds(3)
    );


    EXPECT_FALSE(
        db.exists("temporary")
    );


    EXPECT_EQ(
        db.ttl("temporary"),
        -2
    );
}


TEST(
    KeyValueStoreTest,
    PermanentTTL
) {

    KeyValueStore db;


    db.set(
        "permanent",
        "value"
    );


    EXPECT_EQ(
        db.ttl("permanent"),
        -1
    );
}


TEST(
    KeyValueStoreTest,
    MissingTTL
) {

    KeyValueStore db;


    EXPECT_EQ(
        db.ttl("missing"),
        -2
    );
}