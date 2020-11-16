/*
 * Copyright 2010-2017 JetBrains s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <type_traits>
#include "KAssert.h"

#ifndef RUNTIME_UTILS_H
#define RUNTIME_UTILS_H

class SimpleMutex {
 private:
  int32_t atomicInt = 0;

 public:
  void lock() {
    while (!__sync_bool_compare_and_swap(&atomicInt, 0, 1)) {
      // TODO: yield.
    }
  }

  void unlock() {
    if (!__sync_bool_compare_and_swap(&atomicInt, 1, 0)) {
      RuntimeAssert(false, "Unable to unlock");
    }
  }
};

// TODO: use std::lock_guard instead?
template <class Mutex>
class LockGuard {
 public:
  explicit LockGuard(Mutex& mutex_) : mutex(mutex_) {
    mutex.lock();
  }

  ~LockGuard() {
    mutex.unlock();
  }

 private:
  Mutex& mutex;

  LockGuard(const LockGuard&) = delete;
  LockGuard& operator=(const LockGuard&) = delete;
};

// A helper for implementing classes with disabled copy constructor and copy assignment.
// Usage:
// class A: private NoCopy {
//     ...
// };
// Prefer private inheritance to discourage casting instances of `A` to instances
// of `NoCopy`.
class NoCopy {
    // Hide constructors, assignments and destructor, to discourage operating on an instance of `NoCopy`.
protected:
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = default;

    NoCopy& operator=(const NoCopy&) = delete;
    NoCopy& operator=(NoCopy&&) = default;

    // Not virtual by design. Since this class hides this destructor, no one can destroy an
    // instance of `NoCopy` directly, so this destructor is never called in a virtual manner.
    ~NoCopy() = default;
};

// A helper for implementing classes with disabled copy and move constructors, and copy and move assignments.
// Usage:
// class A: private NoCopyOrMove {
//     ...
// };
// Prefer private inheritance to discourage casting instances of `A` to instances
// of `NoCopyOrMove`.
class NoCopyOrMove {
    // Hide constructors, assignments and destructor, to discourage operating on an instance of `NoCopyOrMove`.
protected:
    NoCopyOrMove() = default;
    NoCopyOrMove(const NoCopyOrMove&) = delete;
    NoCopyOrMove(NoCopyOrMove&&) = delete;

    NoCopyOrMove& operator=(const NoCopyOrMove&) = delete;
    NoCopyOrMove& operator=(NoCopyOrMove&&) = delete;

    // Not virtual by design. Since this class hides this destructor, no one can destroy an
    // instance of `NoCopyOrMove` directly, so this destructor is never called in a virtual manner.
    ~NoCopyOrMove() = default;
};

// Given
// struct SomeWrapper {
//     SomeType value;
//     ... // (possibly) some other fields
// };
// allows to cast from `SomeValue*` to `SomeWrapper*` as no-op. It only works
// if `SomeWrapper` is standard layout and `value` is the first non-static data member.
// See https://en.cppreference.com/w/cpp/language/data_members#Standard_layout
//
// Useful for exporting SomeType under a different name (e.g. exporting inner C++ class
// as public C struct).
//
// Also useful for implementing a form of private inheritance. It allows inheriting from
// primitive types, however there're no virtual functions (including virtual destructor).
// For example:
// // Module A
// struct SomeType { ... };
// // Module B API
// SomeType* constructSomeType(Args...);
// void useSomeType(SomeType*);
// // Module B
// struct InternalType {
//     SomeType value;
//     InternalData extra;
// };
// SomeType* constructSomeType(Args... args) {
//     auto* internalType = new InternalType{ SomeType(args...), InternalData(...) };
//     return &internalType->value;
// }
// void useSomeType(SomeType* value) {
//     InternalType* internalType = wrapper_cast(InternalType, value, value);
//     // Do something with InternalType
// }
#define wrapper_cast(Wrapper, inner, field) \
    /* With -O2 this lambda does not exist in the bitcode. */ \
    []() { \
        static_assert(std::is_standard_layout<Wrapper>::value, #Wrapper " must be standard layout"); \
        static_assert(offsetof(Wrapper, field) == 0, #field " must be at 0 offset"); \
    }(), \
            reinterpret_cast<Wrapper*>(inner)

#endif // RUNTIME_UTILS_H
