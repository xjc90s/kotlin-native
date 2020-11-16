/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H
#define RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H

#include <cstddef>
#include <memory>
#include <mutex>

#include "CppSupport.hpp"
#include "Utils.h"

namespace kotlin {
namespace mm {

template <typename Value>
class ThreadSafeIntrusiveList : private NoCopyOrMove {
private:
    struct Node;

public:
    class Iterator {
    public:
        explicit Iterator(Node* node) noexcept : node_(node) {}

        Value& operator*() noexcept { return node_->value; }

        Iterator& operator++() noexcept {
            node_ = node_->next.get();
            return *this;
        }

        bool operator==(const Iterator& rhs) const noexcept { return node_ == rhs.node_; }

        bool operator!=(const Iterator& rhs) const noexcept { return node_ != rhs.node_; }

    private:
        Node* node_;
    };

    class Iterable : private NoCopy {
    public:
        explicit Iterable(ThreadSafeIntrusiveList* list) noexcept : list_(list), guard_(list->mutex_) {}

        Iterator begin() noexcept { return Iterator(list_->root_.get()); }

        Iterator end() noexcept { return Iterator(nullptr); }

    private:
        ThreadSafeIntrusiveList* list_;
        std::unique_lock<SimpleMutex> guard_;
    };

    template <typename... Args>
    Value* emplace(Args... args) noexcept {
        auto node = kotlin::make_unique<Node>(args...);
        auto* result = &node.get()->value;
        std::lock_guard<SimpleMutex> guard(mutex_);
        if (root_) {
            root_->previous = node.get();
        }
        node->next = std::move(root_);
        root_ = std::move(node);
        return result;
    }

    // You can only `erase` `Value`s that were returned by `emplace`. Trying
    // to erase some other value is undefined behaviour. Using `value` after
    // `erase` is undefined behaviour.
    void erase(Value* value) noexcept {
        auto* node = Node::from(value);
        std::lock_guard<SimpleMutex> guard(mutex_);
        if (root_.get() == node) {
            root_ = std::move(node->next);
            if (root_) {
                root_->previous = nullptr;
            }
            return;
        }
        auto* previous = node->previous;
        RuntimeAssert(previous != nullptr, "Only the root node doesn't have the previous node");
        auto ownedNode = std::move(previous->next);
        previous->next = std::move(node->next);
        if (auto& next = previous->next) {
            next->previous = previous;
        }
    }

    // Returned value locks `this` to perform safe iteration. `this` unlocks when
    // `Iterable` gets out of scope. Example usage:
    // for (auto& value: list.iter()) {
    //    // Do something with `value`, there's a guarantee that it'll not be
    //    // destroyed mid-iteration.
    // }
    // // At this point `list` is unlocked.
    Iterable iter() noexcept { return Iterable(this); }

private:
    struct Node {
        template <typename... Args>
        Node(Args... args) noexcept : value(args...) {}

        Value value;
        // TODO: Consider adding a marker for checks in debug mode if Value was constructed inside the Node.
        std::unique_ptr<Node> next;
        Node* previous = nullptr; // weak

        ALWAYS_INLINE static Node* from(Value* value) { return wrapper_cast(Node, value, value); }
    };

    std::unique_ptr<Node> root_;
    // TODO: Consider different locking mechanisms.
    SimpleMutex mutex_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H
