#pragma once

#include <jni/basics.hpp>

#include <PlusPlus/Forwarder.h>
#include <PlusPlus/Invoke.h>
#include <PlusPlus/GroupMakers.h>

namespace jni
   {
    template < class T > struct Forwarder : PlusPlus::ForwardOutputsAndNonscalarsAsNonconstPointers<T> {};

    template < class T >
    auto Forward( T&& t )
       {
        return Forwarder<T>()( std::forward<T>(t) );
       }


    template < class R, class F, class... P >
    auto Invoke( R&& r, F&& f, P&&... p )
       {
        return   PlusPlus::Invoke< Wrapper, Seizer, Forwarder >( std::forward<R>(r),
                                                                 std::forward<F>(f),
                                                                 std::forward<P>(p)... );
       }


    // Machinery for converting template varargs to jvalue arrays for Call*MethodA
    // invocations.

    template < class... P >
    class PackedInParameterGroup
       {
        private:
            std::array<::jvalue, sizeof...(P)> packed;

        public:
            PackedInParameterGroup(P&&... args) : packed({ Make<jvalue>(args)... }) {};

            bool CheckForFailure() const        { return false; }
            std::tuple<> ThrownParts() const    { return std::tuple<>(); }
            std::tuple<> ReturnedParts() const  { return std::tuple<>(); }

            std::tuple<jvalue*> PassedParts() const { return std::tuple<jvalue*>(packed.data()); }
       };

    template <>
    class PackedInParameterGroup<>
       {
        public:
            bool CheckForFailure() const        { return false; }
            std::tuple<> ThrownParts() const    { return std::tuple<>(); }
            std::tuple<> ReturnedParts() const  { return std::tuple<>(); }

            std::tuple<jvalue*> PassedParts() const { return std::tuple<jvalue*>(nullptr); }
       };

    template < class... P >
    PackedInParameterGroup< P... >
    InPacked( P&&... p )
       {
        return PackedInParameterGroup< P... >( std::forward<P>(p)... );
       }


    using PlusPlus::In;
    using PlusPlus::Out;
//    using PlusPlus::OutFailureFlag;
//    using PlusPlus::OutSuccessFlag;
//    using PlusPlus::OutError;
//
//    using PlusPlus::InOut;
//
    using PlusPlus::NotPassed;
//    using PlusPlus::ExceptionToThrow;
//
    using PlusPlus::Result;
    using PlusPlus::NoResult;
//
//    // Modifiers
//    using PlusPlus::FailsWhen;
//    using PlusPlus::SucceedsWhen;
//    using PlusPlus::FailsWhenTrue;
    using PlusPlus::FailsWhenFalse;
//
//    using PlusPlus::Returned;
//    using PlusPlus::NotReturned;
//
//    using PlusPlus::Thrown;
//    using PlusPlus::NotThrown;
   }
