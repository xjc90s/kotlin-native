/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_CPP_SUPPORT_H
#define RUNTIME_CPP_SUPPORT_H

#include <memory>

namespace kotlin {

// TODO: Replace with `std::make_unique` when we have C++14 everywhere
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args... args) {
    return std::unique_ptr<T>(new T(args...));
}

} // namespace kotlin

#endif // RUNTIME_CPP_SUPPORT_H
