#include <jni/jni.hpp>

auto jni::FindClass(JNIEnv& env, const char* name) -> const jclass&
   {
    return *Invoke(Result<jclass*>() + FailsWhenFalse(),
                   std::mem_fn(&::JNIEnv::FindClass),
                   In(env, name),
                   CheckJavaException(env));
   }


void jni::GlobalRefDeleter::operator()(jobject* p) const
   {
    if (p)
       {
        env.DeleteGlobalRef(p);
       }
   }

auto jni::NewGlobalRef(JNIEnv& env, const jobject& object) -> UniqueGlobalRef<jobject>
   {
    return UniqueGlobalRef<jobject>(Invoke(Result<jobject*>() + FailsWhenFalse(),
                                           std::mem_fn(&::JNIEnv::NewGlobalRef),
                                           In(env, object)),
                                    GlobalRefDeleter { env });
   }


auto jni::GetMethodID(JNIEnv& env, const jclass& clazz, const char* name, const char* sig) -> const jmethodID&
   {
    return *Invoke(Result<jmethodID*>(),
                   std::mem_fn(&::JNIEnv::GetMethodID),
                   In(env, clazz, name, sig));
   }

auto jni::GetStaticMethodID(JNIEnv& env, const jclass& clazz, const char* name, const char* sig) -> const jmethodID&
   {
    return *Invoke(Result<jmethodID*>(),
                   std::mem_fn(&::JNIEnv::GetStaticMethodID),
                   In(env, clazz, name, sig));
   }


auto jni::ExceptionCheck(JNIEnv& env) -> jboolean
   {
    return Invoke(Result<jboolean>(),
                  std::mem_fn(&::JNIEnv::ExceptionCheck),
                  In(env));
   }


void jni::JNIEnvDeleter::operator()(JNIEnv* p) const
   {
    if (p)
       {
        vm.DetachCurrentThread();
       }
   }

auto jni::AttachCurrentThread(JavaVM& vm) -> UniqueEnv
   {
    return UniqueEnv(Invoke(ErrorResult(),
                            std::mem_fn(&::JavaVM::AttachCurrentThread),
                            In(vm),
                            Out<JNIEnv*>(),
                            In(nullptr)),
                     JNIEnvDeleter { vm });
   }

void jni::DetachCurrentThread(JavaVM& vm, UniqueEnv&& env)
   {
    return Invoke(ErrorResult(),
                  std::mem_fn(&::JavaVM::DetachCurrentThread),
                  In(vm),
                  NotPassed(env));
   }
