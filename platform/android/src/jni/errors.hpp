#pragma once

#include <jni/basics.hpp>

#include <jni.h>

#include <system_error>

namespace jni
   {
    // JNI jint error codes are wrapped by std::error_code with a custom category,
    // following http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-4.html

    enum class error : int {};

    const error jni_ok        = error(JNI_OK);
    const error jni_err       = error(JNI_ERR);
    const error jni_edetached = error(JNI_EDETACHED);
    const error jni_eversion  = error(JNI_EVERSION);

    const std::error_category& ErrorCategory();
    std::error_code make_error_code(error);

    template <>
    struct Wrapper<std::error_code>
       {
        std::error_code operator()(::jint code) const { return std::error_code(code, ErrorCategory()); }
        ::jint Inverse(const std::error_code& e) const { return e.value(); }
       };


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

    // A standalone group that checks for pending Java exceptions.

    struct CheckJavaException
       {
        JNIEnv& env;

        CheckJavaException(JNIEnv& e) : env(e) {}

        bool CheckForFailure() const                             { return env.ExceptionCheck(); }
        std::tuple<PendingJavaException> ThrownParts() const     { return PendingJavaException(); }

        std::tuple<> PassedParts()   const { return std::tuple<>(); }
        std::tuple<> ReturnedParts() const { return std::tuple<>(); }
       };


    // A result group that checks errors, throwing if the result is not jni_ok.

    struct ErrorResult
       {
        using ResultType = std::error_code;

        bool CheckForFailure(const std::error_code&) const;
        std::tuple<std::system_error> ThrownParts(std::error_code&) const;

        std::tuple<> ReturnedParts(const std::error_code&) const { return std::tuple<>(); }
       };
   }

namespace std
   {
    template <> struct is_error_code_enum<jni::error> : public true_type {};
   }
