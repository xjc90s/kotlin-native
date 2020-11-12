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
        ThreadData* threadData = list_.emplace(pthread_self());
        ThreadData*& currentData = currentThreadData_;
        RuntimeAssert(currentData == nullptr, "This thread already had some data assigned to it.");
        currentData = threadData;
        return threadData;
    }

    // Can only be called with `threadData` returned from `RegisterCurrentThread`.
    // `threadData` cannot be used after this call.
    void Unregister(ThreadData* threadData) {
        list_.erase(threadData);
        // Do not touch `currentThreadData_` as TLS may already have been deallocated.
    }

    // Locks `ThreadRegistry` for safe iteration.
    ThreadSafeIntrusiveList<ThreadData>::Iterable Iter() { return list_.iter(); }

    // Try not to use it very often, as (1) thread local access can be slow on some platforms,
    // (2) TLS gets deallocated before our thread destruction hooks run.
    // Using this after `Unregister` for the thread has been called is undefined behaviour.
    ThreadData* CurrentThreadData() const { return currentThreadData_; }

private:
    ThreadRegistry() = default;
    ~ThreadRegistry() = default;

    static ThreadRegistry instance_;

    static thread_local ThreadData* currentThreadData_;

    ThreadSafeIntrusiveList<ThreadData> list_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_REGISTRY_H
