/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "ThreadRegistry.hpp"

namespace kotlin {
namespace mm {

// static
ThreadData& ThreadData::currentThreadInstance() {
    thread_local ThreadData data;
    return data;
}

// static
ThreadRegistry& ThreadRegistry::instance() {
    // No need to run a destructor for the registry at exit.
    static ThreadRegistry registry [[clang::no_destroy]];
    return registry;
}

} // namespace mm
} // namespace kotlin
