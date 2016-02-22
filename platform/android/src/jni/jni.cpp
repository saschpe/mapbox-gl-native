#include <jni/jni.hpp>

#include <cassert>

auto jni::FindClass(JNIEnv& env, const char* name) -> jclass&
   {
    return *CheckJavaException(env, env.FindClass(name));
   }


void jni::GlobalRefDeleter::operator()(jobject* p) const
   {
    if (p)
       {
        assert(env);
        env->DeleteGlobalRef(p);
       }
   }

auto jni::NewGlobalRef(JNIEnv& env, jobject& obj) -> UniqueGlobalRef<jobject>
   {
    jobject* result = env.NewGlobalRef(&obj);
    if (!result)
        throw std::bad_alloc();
    return UniqueGlobalRef<jobject>(result, GlobalRefDeleter(env));
   }


auto jni::GetMethodID(JNIEnv& env, jclass& clazz, const char* name, const char* sig) -> jmethodID&
   {
    return *CheckJavaException(env, env.GetMethodID(&clazz, name, sig));
   }

auto jni::GetStaticMethodID(JNIEnv& env, jclass& clazz, const char* name, const char* sig) -> jmethodID&
   {
    return *CheckJavaException(env, env.GetStaticMethodID(&clazz, name, sig));
   }


auto jni::ExceptionCheck(JNIEnv& env) -> jboolean
   {
    return env.ExceptionCheck();
   }


void jni::JNIEnvDeleter::operator()(JNIEnv* p) const
   {
    if (p)
       {
        assert(vm);
        vm->DetachCurrentThread();
       }
   }

auto jni::AttachCurrentThread(JavaVM& vm) -> UniqueEnv
   {
    JNIEnv* result;
    CheckErrorCode(vm.AttachCurrentThread(&result, nullptr));
    return UniqueEnv(result, JNIEnvDeleter(vm));
   }

void jni::DetachCurrentThread(JavaVM& vm, UniqueEnv&& env)
   {
    env.release();
    CheckErrorCode(vm.DetachCurrentThread());
   }
