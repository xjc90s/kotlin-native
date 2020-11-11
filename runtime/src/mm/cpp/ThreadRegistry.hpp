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
class ThreadRegistryImpl {
public:
    template <typename... Args>
    Value* emplace(Args... args);

    void erase(Value* value);

private:
};

} // namespace internal

class ThreadRegistry final : private NoCopyOrMove {
public:
    static ThreadRegistry& instance();

    ThreadData* Register() {
        auto* data = registryImpl_.emplace();
        auto& currentData = currentThreadData;
        RuntimeAssert(currentData == nullptr, "This thread already had some data assigned to it.");
        currentData = data;
        return data;
    }

    void Unregister(ThreadData* data) { registryImpl_.erase(data); }

private:
    ThreadRegistry() = default;
    ~ThreadRegistry() = default;

    internal::ThreadRegistryImpl<ThreadData> registryImpl_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_REGISTRY_H
