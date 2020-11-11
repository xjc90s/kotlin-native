/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_REGISTRY_H
#define RUNTIME_MM_THREAD_REGISTRY_H

#include <pthread.h>

#include "ThreadData.hpp"
#include "ThreadSafeIntrusiveList.hpp"
#include "Utils.h"

namespace kotlin {
namespace mm {

class ThreadRegistry final : private NoCopyOrMove {
public:
    static ThreadRegistry& instance() { return instance_; }

    ThreadData* RegisterCurrentThread() {
        ThreadData* threadData = list_.emplace();
        ThreadData*& currentData = currentThreadData;
        RuntimeAssert(currentData == nullptr, "This thread already had some data assigned to it.");
        currentData = threadData;
        return threadData;
    }

    void Unregister(ThreadData* threadData) { list_.erase(threadData); }

    ThreadSafeIntrusiveList<ThreadData>::Iterable Iter() { return list_.iter(); }

private:
    ThreadRegistry() = default;
    ~ThreadRegistry() = default;

    static ThreadRegistry instance_;

    ThreadSafeIntrusiveList<ThreadData> list_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_REGISTRY_H
