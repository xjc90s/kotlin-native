/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H
#define RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H

#include <type_traits>

#include "Utils.h"

namespace kotlin {
namespace mm {

template <typename Value>
class ThreadSafeIntrusiveList : private NoCopyOrMove {
public:
    class Iterable : private NoCopy {
    public:
        Value* begin();
        Value* end();
    };

    template <typename... Args>
    Value* emplace(Args... args);

    void erase(Value* value);

    Iterable iter();

private:
    struct Node : private NoCopyOrMove {
        template <typename... Args>
        Node(Args... args) : value(args...) {}

        Value value;
        Node* next = nullptr;
        Node* prev = nullptr;

        ALWAYS_INLINE static Node* fromValue(Value* value) { return reinterpret_cast<Node*>(value); }
        ALWAYS_INLINE Value* asValue() { return reinterpret_cast<Value*>(this); }
    };

    // It's valid to `reinterpret_cast` between struct type and it's first data member.
    // See https://en.cppreference.com/w/cpp/language/data_members#Standard_layout
    static_assert(std::is_standard_layout<Node>::value, "Node must be standard layout");

    Node* root_ = nullptr;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H
