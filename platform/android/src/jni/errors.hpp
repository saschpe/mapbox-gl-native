#pragma once

#include <jni/basics.hpp>

#include <jni.h>

#include <system_error>

namespace jni
   {
    enum class error : int {};
   }

template <> struct std::is_error_code_enum<jni::error> : public true_type {};

namespace jni
   {
    const error jni_ok        = error(JNI_OK);
    const error jni_err       = error(JNI_ERR);
    const error jni_edetached = error(JNI_EDETACHED);
    const error jni_eversion  = error(JNI_EVERSION);

    const std::error_category& ErrorCategory();
    std::error_code make_error_code(error);

    inline void CheckErrorCode(jint err)
       { if (err != JNI_OK) throw std::system_error(err, ErrorCategory()); }


    // An exception class indicating the presence of a pending Java exception.
    // Note that it does not extract the message or other information from the
    // Java exception; it's not possible to do so without clearing the pending
    // Java exception, and the calling code needs the option not to do that.
    // In most cases, the desired behavior is that the thrown JavaException is
    // caught by an exception handler just before returning to JVM control, and
    // discarded there. Upon returning to JVM control, Java exception handling
    // will take over, processing the still-pending Java exception.

    class PendingJavaException : public std::exception
       {
        public:
            const char* what() const noexcept final { return "pending Java exception"; }
       };

    inline void CheckJavaException( JNIEnv& env )
       { if (env.ExceptionCheck()) throw PendingJavaException(); }

    template < class R >
    R CheckJavaException( JNIEnv& env, R&& r )
       { CheckJavaException(env); return std::move(r); }
   }
