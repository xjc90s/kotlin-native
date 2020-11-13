/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_CPP_SUPPORT_H
#define RUNTIME_CPP_SUPPORT_H

#include <type_traits>
#include <memory>

// A collection of backported utilities from future C++ versions.

namespace kotlin {

// TODO: Replace with `std::make_unique` when we have C++14 everywhere
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args... args) {
    return std::unique_ptr<T>(new T(args...));
}

// TODO: Replace with `std::make_unsigned_t` when we have C++14 everywhere
template <typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;

// TODO: Replace with `std::is_trivially_destructible_v` when we have C++17 everywhere
template <typename T>
constexpr bool is_trivially_destructible_v = std::is_trivially_destructible<T>::value;

} // namespace kotlin

#endif // RUNTIME_CPP_SUPPORT_H
