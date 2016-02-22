#pragma once

#include <jni/jni.hpp>

namespace jni
   {
    template < class Tag >
    class Class
       {
        private:
            JNIEnv& env;
            UniqueGlobalRef<jclass> reference;

        public:
            Class(JNIEnv& e)
               : env(e),
                 reference(NewGlobalRef(env, FindClass(env, Tag::path)))
               {}

            JNIEnv& Env() const { return env; }

            jclass& operator*() const { return *reference; }
       };
   }
