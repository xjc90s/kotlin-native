/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_DATA_H
#define RUNTIME_MM_THREAD_DATA_H

#include <pthread.h>

#include "Utils.h"

namespace kotlin {
namespace mm {

class ThreadData final : private NoCopyOrMove {
public:
    ThreadData() = default;
    ~ThreadData() = default;

    pthread_t threadId() const { return threadId_; }

private:
    const pthread_t threadId_ = pthread_self();
};

// Try not to use it very often, as (1) thread local access can be slow on some platforms,
// (2) TLS gets deallocated before our thread destruction hooks run.
extern thread_local ThreadData* currentThreadData;

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_DATA_H
