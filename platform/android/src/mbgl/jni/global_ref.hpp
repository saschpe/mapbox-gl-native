#pragma once

#include <mbgl/util/noncopyable.hpp>
#include <mbgl/jni/exception.hpp>

#include <jni.h>
#include <cassert>

namespace mbgl {
namespace jni {

class NewGlobalRefFailed : public Exception {
public:
    explicit NewGlobalRefFailed()
        : Exception("JNI: NewGlobalRef failed") {}
};

template <class T>
class GlobalRef : public util::noncopyable {
public:
    GlobalRef() = default;

    GlobalRef(JNIEnv* env_, T obj) : env(env_) {
        ref = reinterpret_cast<T>(env->NewGlobalRef(obj));
        if (!ref) {
            throw NewGlobalRefFailed();
        }
    }

    ~GlobalRef() {
        if (ref) {
            env->DeleteGlobalRef(ref);
        }
    }

    operator jobject() const { return ref; }

    JNIEnv* env;
    T ref;
};

} // namespace jni
} // namespace mbgl
