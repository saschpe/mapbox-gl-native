#include <jni/errors.hpp>

#include <string>

namespace jni
   {
    class ErrorCategoryImpl : public std::error_category
       {
        public:
            const char* name() const noexcept override;
            std::string message(int ev) const override;
       };
   }

const char* jni::ErrorCategoryImpl::name() const noexcept
   {
    return "JNI";
   }

std::string jni::ErrorCategoryImpl::message(int ev) const
   {
    switch (static_cast<error>(ev))
       {
        case jni_ok:        return "OK";
        case jni_err:       return "Unspecified error";
        case jni_edetached: return "Detached error";
        case jni_eversion:  return "Version error";
        default:            return "Unknown error";
       }
   }

const std::error_category& jni::ErrorCategory()
   {
    static ErrorCategoryImpl instance;
    return instance;
   }
