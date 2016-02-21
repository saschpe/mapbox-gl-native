#pragma once

#include <jni/basics.hpp>
#include <jni/errors.hpp>
#include <jni/invoke.hpp>

#include <jni.h>

#include <functional>

namespace jni
   {
    using jboolean = bool;
    using ::jbyte;
    using ::jchar;
    using ::jshort;
    using ::jint;
    using ::jlong;
    using ::jfloat;
    using ::jdouble;

    /* "cardinal indices and sizes" */
    // typedef jint     jsize;

    // Instead of faux-value types that are actually pointer types, we pass
    // and return references to the underlying element type.
    using jobject = std::pointer_traits< ::jobject >::element_type;
    using jclass  = std::pointer_traits< ::jclass  >::element_type;
//    using ::jstring;
//    using ::jarray;
//    using ::jobjectArray;
//    using ::jbooleanArray;
//    using ::jbyteArray;
//    using ::jcharArray;
//    using ::jshortArray;
//    using ::jintArray;
//    using ::jlongArray;
//    using ::jfloatArray;
//    using ::jdoubleArray;
//    using ::jthrowable;
//    using ::jweak;

    using jfieldID  = std::pointer_traits< ::jfieldID   >::element_type;
    using jmethodID = std::pointer_traits< ::jmethodID  >::element_type;


    using ::jvalue;

    inline jvalue MakeAnything( ThingToMake<jvalue>,       jboolean z ) { jvalue v; v.z =         z ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jbyte    b ) { jvalue v; v.b =         b ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jchar    c ) { jvalue v; v.c =         c ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jshort   s ) { jvalue v; v.s =         s ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jint     i ) { jvalue v; v.i =         i ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jlong    j ) { jvalue v; v.j =         j ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jfloat   f ) { jvalue v; v.f =         f ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>,       jdouble  d ) { jvalue v; v.d =         d ; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, const jobject& l ) { jvalue v; v.l = Forward(l); return v; };


/*
    jobjectRefType
    JNINativeMethod
*/

    using ::JavaVM;
    using ::JNIEnv;


    const jclass& FindClass(JNIEnv&, const char*);


    struct GlobalRefDeleter
       {
        void operator()(jobject*) const;
        JNIEnv& env;
       };

    template < class T >
    using UniqueGlobalRef = std::unique_ptr< T, GlobalRefDeleter >;

    template < class R >
    UniqueGlobalRef<R> UniqueGlobalRefCast(UniqueGlobalRef<jobject>&& in)
       {
        return UniqueGlobalRef<R>(static_cast<R*>(in.release()),
                                  std::move(in.get_deleter()));
       }


    UniqueGlobalRef<jobject> NewGlobalRef(JNIEnv&, const jobject&);

    template < class T >
    UniqueGlobalRef<T> NewGlobalRef(JNIEnv& env, const T& t)
       {
        return UniqueGlobalRefCast<T>(NewGlobalRef(env, static_cast<jobject>(t)));
       }

    void DeleteGlobalRef(JNIEnv&, UniqueGlobalRef<jobject>&&);


    const jmethodID& GetMethodID(JNIEnv&, const jclass&, const char* name, const char* sig);
    const jmethodID& GetStaticMethodID(JNIEnv&, const jclass&, const char* name, const char* sig);


    template < class... Args >
    jobject* CallStaticObjectMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jobject*>(),
                      std::mem_fn(&::JNIEnv::CallStaticObjectMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jboolean CallStaticBooleanMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jboolean>(),
                      std::mem_fn(&::JNIEnv::CallStaticBooleanMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jbyte CallStaticByteMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jbyte>(),
                      std::mem_fn(&::JNIEnv::CallStaticByteMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jchar CallStaticCharMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jchar>(),
                      std::mem_fn(&::JNIEnv::CallStaticCharMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jshort CallStaticShortMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jshort>(),
                      std::mem_fn(&::JNIEnv::CallStaticShortMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jint CallStaticIntMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jint>(),
                      std::mem_fn(&::JNIEnv::CallStaticIntMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jlong CallStaticLongMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jlong>(),
                      std::mem_fn(&::JNIEnv::CallStaticLongMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jfloat CallStaticFloatMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jfloat>(),
                      std::mem_fn(&::JNIEnv::CallStaticFloatMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    jdouble CallStaticDoubleMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        return Invoke(Result<jdouble>(),
                      std::mem_fn(&::JNIEnv::CallStaticDoubleMethodA),
                      In(env, clazz, method),
                      InPacked(std::forward<Args>(args)...),
                      CheckJavaException(env));
       }

    template < class... Args >
    void CallStaticVoidMethod(JNIEnv& env, const jclass& clazz, const jmethodID& method, Args&&... args)
       {
        Invoke(NoResult(),
               std::mem_fn(&::JNIEnv::CallStaticVoidMethodA),
               In(env, clazz, method),
               InPacked(std::forward<Args>(args)...),
               CheckJavaException(env));
       }


    jboolean ExceptionCheck(JNIEnv&);


    // UniqueEnv refers to a JNIEnv obtained via AttachCurrentThread, and represents
    // the obligation to close it via DetachCurrentThread. It stores a reference to
    // the JavaVM to which the JNIEnv belongs, to pass to DetachCurrentThread.
    struct JNIEnvDeleter
       {
        using pointer = JNIEnv*;
        void operator()(JNIEnv*) const;
        JavaVM& vm;
       };

    using UniqueEnv = std::unique_ptr<JNIEnv, JNIEnvDeleter>;

    UniqueEnv AttachCurrentThread(JavaVM&);
    void DetachCurrentThread(JavaVM&, UniqueEnv&&);
   }
