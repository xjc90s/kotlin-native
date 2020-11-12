/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "ThreadSafeIntrusiveList.hpp"

#include <deque>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace kotlin {
namespace mm {

using IntList = ThreadSafeIntrusiveList<int>;

TEST(ThreadSafeIntrusiveListTest, Emplace) {
    IntList list;
    constexpr int kFirst = 1;
    constexpr int kSecond = 2;
    constexpr int kThird = 3;
    int* first = list.emplace(kFirst);
    int* second = list.emplace(kSecond);
    int* third = list.emplace(kThird);
    EXPECT_THAT(*first, kFirst);
    EXPECT_THAT(*second, kSecond);
    EXPECT_THAT(*third, kThird);
}

TEST(ThreadSafeIntrusiveListTest, EmplaceAndIter) {
    IntList list;
    constexpr int kFirst = 1;
    constexpr int kSecond = 2;
    constexpr int kThird = 3;
    list.emplace(kFirst);
    list.emplace(kSecond);
    list.emplace(kThird);

    std::vector<int> actual;
    for (int element : list.iter()) {
        actual.push_back(element);
    }

    EXPECT_THAT(actual, testing::ElementsAre(kThird, kSecond, kFirst));
}

TEST(ThreadSafeIntrusiveListTest, EmplaceEraseAndIter) {
    IntList list;
    constexpr int kFirst = 1;
    constexpr int kSecond = 2;
    constexpr int kThird = 3;
    list.emplace(kFirst);
    int* second = list.emplace(kSecond);
    list.emplace(kThird);
    list.erase(second);

    std::vector<int> actual;
    for (int element : list.iter()) {
        actual.push_back(element);
    }

    EXPECT_THAT(actual, testing::ElementsAre(kThird, kFirst));
}

TEST(ThreadSafeIntrusiveListTest, IterEmpty) {
    IntList list;

    std::vector<int> actual;
    for (int element : list.iter()) {
        actual.push_back(element);
    }

    EXPECT_THAT(actual, testing::IsEmpty());
}

TEST(ThreadSafeIntrusiveListTest, EraseToEmptyEmplaceAndIter) {
    IntList list;
    constexpr int kFirst = 1;
    constexpr int kSecond = 2;
    constexpr int kThird = 3;
    constexpr int kFourth = 4;
    auto* first = list.emplace(kFirst);
    auto* second = list.emplace(kSecond);
    list.erase(first);
    list.erase(second);
    list.emplace(kThird);
    list.emplace(kFourth);

    std::vector<int> actual;
    for (int element : list.iter()) {
        actual.push_back(element);
    }

    EXPECT_THAT(actual, testing::ElementsAre(kFourth, kThird));
}

TEST(ThreadSafeIntrusiveListTest, ConcurrentEmplace) {
    IntList list;
    constexpr int kThreadCount = 100;
    std::atomic<bool> canStart;
    std::vector<std::thread> threads;
    std::vector<int> expected;
    for (int i = 0; i < kThreadCount; ++i) {
        expected.push_back(i);
        threads.emplace_back([i, &list, &canStart]() {
            while (!canStart) {
            }
            list.emplace(i);
        });
    }

    canStart = true;
    for (auto& t : threads) {
        t.join();
    }

    std::vector<int> actual;
    for (int element : list.iter()) {
        actual.push_back(element);
    }

    EXPECT_THAT(actual, testing::UnorderedElementsAreArray(expected));
}

TEST(ThreadSafeIntrusiveListTest, ConcurrentErase) {
    IntList list;
    constexpr int kThreadCount = 100;
    std::vector<int*> items;
    for (int i = 0; i < kThreadCount; ++i) {
        items.push_back(list.emplace(i));
    }

    std::atomic<bool> canStart;
    std::vector<std::thread> threads;
    for (int* item : items) {
        threads.emplace_back([item, &list, &canStart]() {
            while (!canStart) {
            }
            list.erase(item);
        });
    }

    canStart = true;
    for (auto& t : threads) {
        t.join();
    }

    std::vector<int> actual;
    for (int element : list.iter()) {
        actual.push_back(element);
    }

    EXPECT_THAT(actual, testing::IsEmpty());
}

TEST(ThreadSafeIntrusiveListTest, DISABLED_IterWhileConcurrentEmplace) {
    IntList list;
    constexpr int kStartCount = 50;
    constexpr int kThreadCount = 100;

    std::deque<int> expectedBefore;
    std::vector<int> expectedAfter;
    for (int i = 0; i < kStartCount; ++i) {
        expectedBefore.push_front(i);
        expectedAfter.push_back(i);
        list.emplace(i);
    }

    std::atomic<bool> canStart;
    std::atomic<int> startedCount;
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreadCount; ++i) {
        int j = i + kStartCount;
        expectedAfter.push_back(j);
        threads.emplace_back([j, &list, &canStart, &startedCount]() {
            while (!canStart) {
            }
            ++startedCount;
            list.emplace(j);
        });
    }

    std::vector<int> actualBefore;
    {
        auto iter = list.iter();
        canStart = true;
        while (startedCount < kThreadCount) {
        }

        for (int element : iter) {
            actualBefore.push_back(element);
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_THAT(actualBefore, testing::ElementsAreArray(expectedBefore));

    std::vector<int> actualAfter;
    for (int element : list.iter()) {
        actualAfter.push_back(element);
    }

    EXPECT_THAT(actualAfter, testing::UnorderedElementsAreArray(expectedAfter));
}

TEST(ThreadSafeIntrusiveListTest, DISABLED_IterWhileConcurrentErase) {
    IntList list;
    constexpr int kThreadCount = 100;

    std::deque<int> expectedBefore;
    std::vector<int*> items;
    for (int i = 0; i < kThreadCount; ++i) {
        expectedBefore.push_front(i);
        items.push_back(list.emplace(i));
    }

    std::atomic<bool> canStart;
    std::atomic<int> startedCount;
    std::vector<std::thread> threads;
    for (int* item : items) {
        threads.emplace_back([item, &list, &canStart, &startedCount]() {
            while (!canStart) {
            }
            ++startedCount;
            list.erase(item);
        });
    }

    std::vector<int> actualBefore;
    {
        auto iter = list.iter();
        canStart = true;
        while (startedCount < kThreadCount) {
        }

        for (int element : iter) {
            actualBefore.push_back(element);
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_THAT(actualBefore, testing::ElementsAreArray(expectedBefore));

    std::vector<int> actualAfter;
    for (int element : list.iter()) {
        actualAfter.push_back(element);
    }

    EXPECT_THAT(actualAfter, testing::IsEmpty());
}

namespace {

struct Pinned : private NoCopyOrMove {
    Pinned(int i) : i(i) {}

    int i;
};

} // namespace

TEST(ThreadSafeIntrusiveListTest, PinnedType) {
    ThreadSafeIntrusiveList<Pinned> list;
    constexpr int kFirst = 1;

    auto* item = list.emplace(kFirst);
    EXPECT_THAT(item->i, kFirst);

    list.erase(item);

    std::vector<Pinned*> actualAfter;
    for (auto& element : list.iter()) {
        actualAfter.push_back(&element);
    }

    EXPECT_THAT(actualAfter, testing::IsEmpty());
}

} // namespace mm
} // namespace kotlin
