#pragma once

#include <jni/jni.hpp>
#include <jni/object.hpp>

namespace jni
   {
    template < class R, class... Args >
    constexpr const char * MethodSignature();


    template < class TheTag, class... Args >
    class Constructor
       {
        public:
            using TagType = TheTag;

        private:
            JNIEnv& env;
            jclass& clazz;
            jmethodID& method;

        public:
            Constructor( Class<TagType>& c )
              : env( c.Env() ),
                clazz( *c ),
                method( GetMethodID( env, clazz, "<init>", MethodSignature<void, Args...>() ) )
               {}

            JNIEnv& Env() const { return env; }

            Object<TagType> operator()( Args&... args ) const
              {
               return Tag<Object<TagType>>( env,
                   &NewObject( env, clazz, method,
                       Untag( std::forward<Args>(args) )... ) );
              }
       };


    template < class TheTag, class >
    class Method;

    template < class TheTag, class R, class... Args >
    class Method< TheTag, R (Args...) >
       {
        public:
            using TagType = TheTag;

        private:
            using TaggedResultType = R;
            using UntaggedResultType = typename Tagger<R>::UntaggedType;

            JNIEnv& env;
            jmethodID& method;

        public:
            Method( Class<TagType>& clazz, const char* name )
              : env( clazz.Env() ),
                method( GetMethodID( env, *clazz, name, MethodSignature<R, Args...>() ) )
               {}

            JNIEnv& Env() const { return env; }

            R operator()( Object<TagType>& instance, Args&... args ) const
              {
               return Tag<R>( env,
                   CallMethod<UntaggedResultType>( env, instance.Get(), method,
                       Untag( std::forward<Args>(args) )... ) );
              }
       };

    template < class TheTag, class... Args >
    class Method< TheTag, void (Args...) >
       {
        public:
            using TagType = TheTag;

        private:
            JNIEnv& env;
            jmethodID& method;

        public:
            Method( Class<TagType>& clazz, const char* name )
              : env( clazz.Env() ),
                method( GetMethodID( env, *clazz, name, MethodSignature<void, Args...>() ) )
               {}

            JNIEnv& Env() const { return env; }

            void operator()( Object<TagType>& instance, Args&... args ) const
              {
               return CallMethod<void>( env, instance.Get(), method,
                   Untag<TagType>( std::forward<Args>(args) )... );
              }
       };


    template < class TheTag, class >
    class StaticMethod;

    template < class TheTag, class R, class... Args >
    class StaticMethod< TheTag, R (Args...) >
       {
        public:
            using TagType = TheTag;

        private:
            using TaggedResultType = R;
            using UntaggedResultType = typename Tagger<R>::UntaggedType;

            JNIEnv& env;
            jclass& clazz;
            jmethodID& method;

        public:
            StaticMethod( Class<TagType>& c, const char* name )
              : env( c.Env() ),
                clazz( *c ),
                method( GetStaticMethodID( env, clazz, name, MethodSignature<R, Args...>() ) )
               {}

            JNIEnv& Env() const { return env; }

            R operator()( Args&... args ) const
              {
               return Tag<R>( env,
                   CallStaticMethod<UntaggedResultType>( env, clazz, method,
                       Untag( std::forward<Args>(args) )... ) );
              }
       };

    template < class TheTag, class... Args >
    class StaticMethod< TheTag, void (Args...) >
       {
        public:
            using TagType = TheTag;

        private:
            JNIEnv& env;
            jclass& clazz;
            jmethodID& method;

        public:
            StaticMethod( Class<TagType>& c, const char* name )
              : env( c.Env() ),
                clazz( *c ),
                method( GetStaticMethodID( env, clazz, name, MethodSignature<void, Args...>() ) )
               {}

            JNIEnv& Env() const { return env; }

            void operator()( Args&... args ) const
              {
               return CallStaticMethod<void>( env, clazz, method,
                   Untag( std::forward<Args>(args) )... );
              }
       };
   }
