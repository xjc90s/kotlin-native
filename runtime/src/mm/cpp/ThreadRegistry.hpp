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

template <typename Key, typename Value>
class ThreadRegistryImpl {
public:
    void insert(Key key, Value value);
    void erase(Key key);

private:
};

} // namespace internal

class ThreadRegistry final : private NoCopyOrMove {
public:
    static ThreadRegistry& instance();

    ThreadData* Register() {
        auto& data = ThreadData::currentThreadInstance();
        registryImpl_.insert(data.threadId(), &data);
        return &data;
    }

    void Unregister(ThreadData* data) { registryImpl_.erase(data->threadId()); }

private:
    ThreadRegistry() = default;
    ~ThreadRegistry() = default;

    internal::ThreadRegistryImpl<pthread_t, ThreadData*> registryImpl_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_REGISTRY_H
