#pragma once

#include <jni/jni.hpp>

namespace jni
   {
    template < class TheTag >
    class Object
       {
        public:
            using TagType = TheTag;

        private:
            UniqueGlobalRef<jobject> reference;

        public:
            Object(JNIEnv& env, jobject* obj)
               {
                if (obj) reference = NewGlobalRef(env, *obj);
               }

            explicit operator bool() const { return reference.operator bool(); }
            jobject* Get() const { return reference.get(); }
       };


    template < class T >
    struct Tagger
       {
        using TaggedType = T;
        using UntaggedType = T;

        T Tag( JNIEnv&, T t ) const { return t; }
        T Untag( T t ) const { return t; }
       };

    template < class TheTag >
    struct Tagger< Object<TheTag> >
       {
        using TaggedType = Object<TheTag>;
        using UntaggedType = jobject*;

        Object<TheTag> Tag( JNIEnv& env, jobject* obj ) const { return Object<TheTag>( env,  obj ); }
        jobject* Untag( const Object<TheTag>& obj ) const { return obj.Get(); }
       };

    template < class T, class U >
    auto Tag( JNIEnv& env, U&& u )
       {
        return Tagger<T>().Tag( env, std::forward<U>( u ) );
       }

    template < class T >
    auto Untag( T&& t )
       {
        return Tagger<T>().Untag( std::forward<T>( t ) );
       }
   }
