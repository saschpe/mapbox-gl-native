#pragma once

#include <mbgl/util/noncopyable.hpp>

#include <jni.h>
#include <cassert>

namespace mbgl {
namespace jni {

class ThreadAttachment : public util::noncopyable {
public:
    ThreadAttachment(JavaVM *vm_) : vm(vm_) {
        Check(vm->AttachCurrentThread(&env, nullptr));
    }

    ~ThreadAttachment() {
        Check(vm->DetachCurrentThread());
    }

    jclass FindClass(const char *name) {
        return ExceptionCheck(env->FindClass(name));
    }

    jmethodID GetStaticMethodID(jclass clazz, const char *name, const char *sig) {
        return ExceptionCheck(env->GetStaticMethodID(clazz, name, sig));
    }

    jmethodID GetMethodID(jclass clazz, const char *name, const char *sig) {
        return ExceptionCheck(env->GetMethodID(clazz, name, sig));
    }

    template <class... Args>
    jobject NewObject(jclass clazz, jmethodID methodID, Args... args) {
        return ExceptionCheck(env->NewObject(clazz, methodID, args...));
    }

    template <class... Args>
    void CallStaticVoidMethod(jclass clazz, jmethodID methodID, Args... args) {
        env->CallStaticVoidMethod(clazz, methodID, args...);
        ExceptionCheck();
    }

    template <class... Args>
    jobject CallStaticObjectMethod(jclass clazz, jmethodID methodID, Args... args) {
        return ExceptionCheck(env->CallStaticObjectMethod(clazz, methodID, args...));
    }

    template <class... Args>
    void CallVoidMethod(jobject obj, jmethodID methodID, Args... args) {
        env->CallVoidMethod(obj, methodID, args...);
        ExceptionCheck();
    }

private:
    void Check(jint result) {
        if (result != JNI_OK) {
            std::terminate();
        }
    }

    void ExceptionCheck() {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            std::terminate();
        }
    }

    template <typename T>
    T ExceptionCheck(T t) {
        ExceptionCheck();
        return t;
    }

    JavaVM* vm;
    JNIEnv* env;
};

} // namespace jni
} // namespace mbgl
