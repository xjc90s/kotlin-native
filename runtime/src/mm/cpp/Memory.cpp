/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Memory.h"

#include "ThreadRegistry.hpp"

extern "C" {

struct MemoryState {
    kotlin::mm::ThreadData data;
};

namespace {

// It's valid to `reinterpret_cast` between struct type and it's first data member.
// See https://en.cppreference.com/w/cpp/language/data_members#Standard_layout

ALWAYS_INLINE MemoryState* asMemoryState(kotlin::mm::ThreadData* data) {
    return reinterpret_cast<MemoryState*>(data);
}

ALWAYS_INLINE kotlin::mm::ThreadData* asThreadData(MemoryState* state) {
    return reinterpret_cast<kotlin::mm::ThreadData*>(state);
}

} // namespace

MemoryState* InitMemory() {
    kotlin::mm::ThreadData* data = kotlin::mm::ThreadRegistry::instance().Register();
    return asMemoryState(data);
}

void DeinitMemory(MemoryState* state) {
    kotlin::mm::ThreadRegistry::instance().Unregister(asThreadData(state));
}

} // extern "C"
