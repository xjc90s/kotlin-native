/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_REGISTRY_H
#define RUNTIME_MM_THREAD_REGISTRY_H

#include <pthread.h>

#include "ThreadData.hpp"
#include "Utils.h"

namespace kotlin {
namespace mm {

namespace internal {

template <typename Value>
class ThreadSafeIntrusiveList {
public:
    class Iterable : private NoCopy {
    };

    template <typename... Args>
    Value* emplace(Args... args);

    void erase(Value* value);

    Iterable iter();
private:
};

} // namespace internal

class ThreadRegistry final : private NoCopyOrMove {
public:
    static ThreadRegistry& instance();

    ThreadData* Register() {
        auto* threadData = list_.emplace();
        auto& currentData = currentThreadData;
        RuntimeAssert(currentData == nullptr, "This thread already had some data assigned to it.");
        currentData = threadData;
        return threadData;
    }

    void Unregister(ThreadData* threadData) { list_.erase(threadData); }

    internal::ThreadSafeIntrusiveList<ThreadData>::Iterable Iter() { return list_.iter(); }

private:
    ThreadRegistry() = default;
    ~ThreadRegistry() = default;

    internal::ThreadSafeIntrusiveList<ThreadData> list_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_REGISTRY_H
