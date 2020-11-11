/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Memory.h"

#include <type_traits>

#include "ThreadData.hpp"
#include "ThreadRegistry.hpp"

extern "C" {

struct MemoryState {
    kotlin::mm::ThreadData data;

    ALWAYS_INLINE static MemoryState* fromThreadData(kotlin::mm::ThreadData* data) {
        return reinterpret_cast<MemoryState*>(data);
    }

    ALWAYS_INLINE kotlin::mm::ThreadData* toThreadData() {
        return reinterpret_cast<kotlin::mm::ThreadData*>(this);
    }
};

// It's valid to `reinterpret_cast` between struct type and it's first data member.
// See https://en.cppreference.com/w/cpp/language/data_members#Standard_layout
static_assert(std::is_standard_layout<MemoryState>::value, "MemoryState must be standard layout");

MemoryState* InitMemory() {
    auto* data = kotlin::mm::ThreadRegistry::instance().RegisterCurrentThread();
    return MemoryState::fromThreadData(data);
}

void DeinitMemory(MemoryState* state) {
    kotlin::mm::ThreadRegistry::instance().Unregister(state->toThreadData());
}

} // extern "C"
