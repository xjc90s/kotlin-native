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
    static ThreadData& currentThreadInstance();

    pthread_t threadId() const { return threadId_; }

private:
    ThreadData() = default;
    ~ThreadData() = default;

    const pthread_t threadId_ = pthread_self();
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_DATA_H
