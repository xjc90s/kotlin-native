/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "ThreadData.hpp"

namespace kotlin {
namespace mm {

// static
ThreadData& ThreadData::currentThreadInstance() {
    thread_local ThreadData data;
    return data;
}

} // namespace mm
} // namespace kotlin
