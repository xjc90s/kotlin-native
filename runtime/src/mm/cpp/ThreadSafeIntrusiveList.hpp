/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H
#define RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H

#include <memory>
#include <mutex>
#include <type_traits>

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
        explicit Iterator(Node* node) : node_(node) {}

        Value& operator*() { return *node_->asValue(); }

        Iterator& operator++() {
            node_ = node_->next.get();
            return *this;
        }

        bool operator==(const Iterator& rhs) const {
            return node_ == rhs.node_;
        }

        bool operator!=(const Iterator& rhs) const {
            return node_ != rhs.node_;
        }

    private:
        Node* node_;
    };

    class Iterable : private NoCopy {
    public:
        explicit Iterable(ThreadSafeIntrusiveList* list) : list_(list), guard_(list->mutex_) {}

        Iterator begin() {
            return Iterator(list_->root_.get());
        }

        Iterator end() {
            return Iterator(nullptr);
        }

    private:
        ThreadSafeIntrusiveList* list_;
        std::unique_lock<SimpleMutex> guard_;
    };

    template <typename... Args>
    Value* emplace(Args... args) noexcept {
        auto node = std::make_unique<Node>(args...);
        auto* result = node.get();
        std::lock_guard<SimpleMutex> guard(mutex_);
        if (root_) {
            root_->previous = node.get();
        }
        node->next = std::move(root_);
        root_ = std::move(node);
        return result;
    }

    void erase(Value* value) noexcept {
        auto* node = Node::fromValue(value);
        std::lock_guard<SimpleMutex> guard(mutex_);
        if (root_.get() == node) {
            root_ = std::move(node->next);
            if (root_) {
                root_->previous = nullptr;
            }
            return;
        }
        auto* previous = node->previous;
        RuntmieAssert(previous != nullptr, "Only the root node doesn't have the previous node");
        auto ownedNode = std::move(previous->next);
        previous->next = std::move(node->next);
        if (auto& next = previous->next) {
            next->previous = previous;
        }
    }

    Iterable iter() noexcept {
        return Iterable(this);
    }

private:
    struct Node : private NoCopyOrMove {
        template <typename... Args>
        Node(Args... args) : value(args...) {}

        Value value;
        std::unique_ptr<Node> next;
        Node* previous = nullptr; // weak

        ALWAYS_INLINE static Node* fromValue(Value* value) { return reinterpret_cast<Node*>(value); }
        ALWAYS_INLINE Value* asValue() { return reinterpret_cast<Value*>(this); }
    };

    // It's valid to `reinterpret_cast` between struct type and it's first data member.
    // See https://en.cppreference.com/w/cpp/language/data_members#Standard_layout
    static_assert(std::is_standard_layout<Node>::value, "Node must be standard layout");

    std::unique_ptr<Node> root_;
    // TODO: Consider different locking mechanisms.
    SimpleMutex mutex_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_THREAD_SAFE_INTRUSIVE_LIST_H
