#pragma once

#include <jni/basics.hpp>
#include <jni/errors.hpp>

#include <jni.h>

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

    inline jvalue MakeAnything( ThingToMake<jvalue>, jboolean z ) { jvalue v; v.z =  z; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jbyte    b ) { jvalue v; v.b =  b; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jchar    c ) { jvalue v; v.c =  c; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jshort   s ) { jvalue v; v.s =  s; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jint     i ) { jvalue v; v.i =  i; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jlong    j ) { jvalue v; v.j =  j; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jfloat   f ) { jvalue v; v.f =  f; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jdouble  d ) { jvalue v; v.d =  d; return v; };
    inline jvalue MakeAnything( ThingToMake<jvalue>, jobject& l ) { jvalue v; v.l = &l; return v; };


/*
    jobjectRefType
    JNINativeMethod
*/

    using ::JavaVM;
    using ::JNIEnv;


    jclass& FindClass(JNIEnv&, const char*);


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


    UniqueGlobalRef<jobject> NewGlobalRef(JNIEnv&, jobject&);

    template < class T >
    UniqueGlobalRef<T> NewGlobalRef(JNIEnv& env, T& t)
       {
        return UniqueGlobalRefCast<T>(NewGlobalRef(env, static_cast<jobject&>(t)));
       }

    void DeleteGlobalRef(JNIEnv&, UniqueGlobalRef<jobject>&&);


    jmethodID& GetMethodID(JNIEnv&, jclass&, const char* name, const char* sig);
    jmethodID& GetStaticMethodID(JNIEnv&, jclass&, const char* name, const char* sig);


    template < class... Args >
    jobject& NewObject(JNIEnv& env, jclass& clazz, jmethodID& method, Args&&... args)
       {
        return *CheckJavaException(env,
            env.NewObjectA(&clazz, &method,
                MakeArray<jvalue>(std::forward<Args>(args)...).data()));
       }


    template < class R > struct CallMethods;

    template <> struct CallMethods< jobject* >
       {
        static constexpr auto       Method = &JNIEnv::CallObjectMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticObjectMethodA;
       };

    template <> struct CallMethods< jboolean >
       {
        static constexpr auto       Method = &JNIEnv::CallBooleanMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticBooleanMethodA;
       };

    template <> struct CallMethods< jbyte >
       {
        static constexpr auto       Method = &JNIEnv::CallByteMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticByteMethodA;
       };

    template <> struct CallMethods< jchar >
       {
        static constexpr auto       Method = &JNIEnv::CallCharMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticCharMethodA;
       };

    template <> struct CallMethods< jshort >
       {
        static constexpr auto       Method = &JNIEnv::CallShortMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticShortMethodA;
       };

    template <> struct CallMethods< jint >
       {
        static constexpr auto       Method = &JNIEnv::CallIntMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticIntMethodA;
       };

    template <> struct CallMethods< jlong >
       {
        static constexpr auto       Method = &JNIEnv::CallLongMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticLongMethodA;
       };

    template <> struct CallMethods< jfloat >
       {
        static constexpr auto       Method = &JNIEnv::CallFloatMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticFloatMethodA;
       };

    template <> struct CallMethods< jdouble >
       {
        static constexpr auto       Method = &JNIEnv::CallDoubleMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticDoubleMethodA;
       };

    template <> struct CallMethods< void >
       {
        static constexpr auto       Method = &JNIEnv::CallVoidMethodA;
        static constexpr auto StaticMethod = &JNIEnv::CallStaticVoidMethodA;
       };

    template < class R, class... Args >
    R CallMethod(JNIEnv& env, jobject& obj, jmethodID& method, Args&&... args)
       {
        CheckJavaExceptionOnExit checker(env);
        auto packedArgs = MakeArray<jvalue>(std::forward<Args>(args)...);
        return (env.*(CallMethods<R>::Method))(&obj, &method, packedArgs.data());
       }

    template < class R, class... Args >
    R CallStaticMethod(JNIEnv& env, jclass& clazz, jmethodID& method, Args&&... args)
       {
        CheckJavaExceptionOnExit checker(env);
        auto packedArgs = MakeArray<jvalue>(std::forward<Args>(args)...);
        return (env.*(CallMethods<R>::StaticMethod))(&clazz, &method, packedArgs.data());
       }


    jboolean ExceptionCheck(JNIEnv&);


    // UniqueEnv refers to a JNIEnv obtained via AttachCurrentThread, and represents
    // the obligation to close it via DetachCurrentThread. It stores a reference to
    // the JavaVM to which the JNIEnv belongs, to pass to DetachCurrentThread.
    struct JNIEnvDeleter
       {
        void operator()(JNIEnv*) const;
        JavaVM& vm;
       };

    using UniqueEnv = std::unique_ptr<JNIEnv, JNIEnvDeleter>;

    UniqueEnv AttachCurrentThread(JavaVM&);
    void DetachCurrentThread(JavaVM&, UniqueEnv&&);
   }
