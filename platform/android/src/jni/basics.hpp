#pragma once

#include <PlusPlus/Wrapper.h>
#include <PlusPlus/Seizer.h>

namespace jni
   {
    // Wrap and Unwrap convert between wrapped types and the underlying type used by JNI.
    // All PlusPlus::Boxed types used as parameters or retured are treated as wrapped;
    // other types are marked as wrapped by specializations of jni::Wrapper.
    //
    // See PlusPlus/Wrapper.h for details.

    template < class W > struct Wrapper : PlusPlus::AllBoxedTypesAreWrapped<W> {};

    template < class W, class U >
    auto Wrap( U&& u )
       { return PlusPlus::Wrap< Wrapper, W >( std::forward<U>(u) ); }

    template < class W >
    auto Unwrap( W&& w )
       { return PlusPlus::Unwrap< Wrapper >( std::forward<W>(w) ); }


    // Seize and Release convert between owned resources and the underlying type used by JNI.
    // All std::unique_ptr types used as parameters or retured are treated as seized;
    // other types could be marked as seized by specializations of jni::Seizer, but there aren't any.
    //
    // Release generally only works on rvalues.
    //
    // See PlusPlus/Seizer.h for details.

    template < class S > struct Seizer : PlusPlus::AllUniquePtrsAreSeized<S> {};

    template < class S, class R >
    auto Seize( R&& r )
       { return PlusPlus::Seize<Seizer,S>( std::forward<R>(r) ); }

    template < class S >
    auto Release( S&& s )
       { return PlusPlus::Release<Seizer>( std::forward<S>(s) ); }


    // Make handles conversions and construction-like operations for types that aren't wrappers.
    // It's used to to convert to C++ standard types, like std::string. Make is extended by
    // overloading MakeAnything, with ThingToMake<T> as the first parameter type.

    template < class Result > struct ThingToMake {};

    template < class Result, class... P >
    Result Make( P&&... p )
       { return MakeAnything( ThingToMake<Result>(), std::forward<P>(p)... ); }
   }
