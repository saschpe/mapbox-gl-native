#pragma once

#include <array>

namespace jni
   {
    // Make handles conversions and construction-like operations for types that aren't wrappers.
    // It's used to to convert to C++ standard types, like std::string. Make is extended by
    // overloading MakeAnything, with ThingToMake<T> as the first parameter type.

    template < class Result > struct ThingToMake {};

    template < class Result, class... P >
    Result Make( P&&... p )
       { return MakeAnything( ThingToMake<Result>(), std::forward<P>(p)... ); }

    template < class T, class... P >
    std::array<T, sizeof...(P)>
    MakeArray( P&&... p ) { return std::array<T, sizeof...(P)>({{ Make<T>( std::forward<P>(p) )... }}); }
   }
