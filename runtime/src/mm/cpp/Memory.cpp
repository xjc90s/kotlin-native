/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Memory.h"

#include <type_traits>

#include "ThreadData.hpp"
#include "ThreadRegistry.hpp"
#include "Utils.h"

extern "C" {

WRAP_TYPE(struct, MemoryState, kotlin::mm::ThreadData);

MemoryState* InitMemory() {
    auto* data = kotlin::mm::ThreadRegistry::instance().RegisterCurrentThread();
    return MemoryState::from(data);
}

void DeinitMemory(MemoryState* state) {
    kotlin::mm::ThreadRegistry::instance().Unregister(state->to());
}

} // extern "C"
