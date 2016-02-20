#pragma once

#include <exception>

namespace mbgl {
namespace jni {

class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& what)
        : std::runtime_error(what) {}
    explicit Exception(const char* what)
        : std::runtime_error(what) {}
};

} // namespace jni
} // namespace mbgl
