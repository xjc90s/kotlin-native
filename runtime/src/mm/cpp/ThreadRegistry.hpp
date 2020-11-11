/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_REGISTRY_H
#define RUNTIME_MM_THREAD_REGISTRY_H

#include <pthread.h>

#include "Utils.h"

namespace kotlin {
namespace mm {

namespace internal {

template <typename Key, typename Value>
class ThreadRegistryImpl {
public:
    void Register(Key key, Value value);
    void Unregister(Key key);

private:
};

} // namespace internal

class ThreadData final : private NoCopyOrMove {
public:
    static ThreadData& currentThreadInstance();

    const pthread_t threadId = pthread_self();

private:
    ThreadData() = default;
    ~ThreadData() = default;
};

class ThreadRegistry final : private NoCopyOrMove {
public:
    static ThreadRegistry& instance();

    ThreadData* Register() {
        auto& data = ThreadData::currentThreadInstance();
        registryImpl_.Register(data.threadId, &data);
        return &data;
    }

    void Unregister(ThreadData* data) { registryImpl_.Unregister(data->threadId); }

private:
    using ThreadKey = pthread_t;

    ThreadRegistry() = default;
    ~ThreadRegistry() = default;

    static ThreadKey currentThreadKey() { return pthread_self(); }

    internal::ThreadRegistryImpl<ThreadKey, ThreadData*> registryImpl_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_REGISTRY_H
