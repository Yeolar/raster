/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rdd {

namespace utility_detail {

template <typename...>
struct make_seq_cat;
template <
    template <typename T, T...> class S,
    typename T,
    T... Ta,
    T... Tb,
    T... Tc>
struct make_seq_cat<S<T, Ta...>, S<T, Tb...>, S<T, Tc...>> {
  using type =
      S<T,
        Ta...,
        (sizeof...(Ta) + Tb)...,
        (sizeof...(Ta) + sizeof...(Tb) + Tc)...>;
};

// Not parameterizing by `template <typename T, T...> class, typename` because
// clang precisely v4.0 fails to compile that. Note that clang v3.9 and v5.0
// handle that code correctly.
//
// For this to work, `S0` is required to be `Sequence<T>` and `S1` is required
// to be `Sequence<T, 0>`.

template <std::size_t Size>
struct make_seq {
  template <typename S0, typename S1>
  using apply = typename make_seq_cat<
      typename make_seq<Size / 2>::template apply<S0, S1>,
      typename make_seq<Size / 2>::template apply<S0, S1>,
      typename make_seq<Size % 2>::template apply<S0, S1>>::type;
};
template <>
struct make_seq<1> {
  template <typename S0, typename S1>
  using apply = S1;
};
template <>
struct make_seq<0> {
  template <typename S0, typename S1>
  using apply = S0;
};

}

template <class T, T... Ints>
struct integer_sequence {
  using value_type = T;

  static constexpr std::size_t size() noexcept {
    return sizeof...(Ints);
  }
};

template <std::size_t... Ints>
using index_sequence = integer_sequence<std::size_t, Ints...>;

template <typename T, std::size_t Size>
using make_integer_sequence = typename utility_detail::make_seq<
    Size>::template apply<integer_sequence<T>, integer_sequence<T, 0>>;

template <std::size_t Size>
using make_index_sequence = make_integer_sequence<std::size_t, Size>;

/*
 * Defines a function rdd::applyTuple, which takes a function and a
 * std::tuple of arguments and calls the function with those
 * arguments.
 *
 * Example:
 *
 *    int x = rdd::applyTuple(std::plus<int>(), std::make_tuple(12, 12));
 *    ASSERT(x == 24);
 */

namespace tuple_detail {

inline constexpr std::size_t sum() {
  return 0;
}
template <typename... Args>
inline constexpr std::size_t sum(std::size_t v1, Args... vs) {
  return v1 + sum(vs...);
}

template <typename... Tuples>
struct TupleSizeSum {
  static constexpr auto value = sum(std::tuple_size<Tuples>::value...);
};

template <typename... Tuples>
using MakeIndexSequenceFromTuple = rdd::make_index_sequence<
    TupleSizeSum<typename std::decay<Tuples>::type...>::value>;

// This is to allow using this with pointers to member functions,
// where the first argument in the tuple will be the this pointer.
template <class F>
inline constexpr F&& makeCallable(F&& f) {
  return std::forward<F>(f);
}
template <class M, class C>
inline constexpr auto makeCallable(M(C::*d)) -> decltype(std::mem_fn(d)) {
  return std::mem_fn(d);
}

template <class F, class Tuple, std::size_t... Indexes>
inline constexpr auto call(F&& f, Tuple&& t, rdd::index_sequence<Indexes...>)
    -> decltype(
        std::forward<F>(f)(std::get<Indexes>(std::forward<Tuple>(t))...)) {
  return std::forward<F>(f)(std::get<Indexes>(std::forward<Tuple>(t))...);
}

template <class Tuple, std::size_t... Indexes>
inline constexpr auto forwardTuple(Tuple&& t, rdd::index_sequence<Indexes...>)
    -> decltype(
        std::forward_as_tuple(std::get<Indexes>(std::forward<Tuple>(t))...)) {
  return std::forward_as_tuple(std::get<Indexes>(std::forward<Tuple>(t))...);
}

} // namespace tuple_detail

/**
 * Invoke a callable object with a set of arguments passed as a tuple, or a
 *     series of tuples
 *
 * Example: the following lines are equivalent
 *     func(1, 2, 3, "foo");
 *     applyTuple(func, std::make_tuple(1, 2, 3, "foo"));
 *     applyTuple(func, std::make_tuple(1, 2), std::make_tuple(3, "foo"));
 */

template <class F, class... Tuples>
inline constexpr auto applyTuple(F&& f, Tuples&&... t)
    -> decltype(tuple_detail::call(
        tuple_detail::makeCallable(std::forward<F>(f)),
        std::tuple_cat(tuple_detail::forwardTuple(
            std::forward<Tuples>(t),
            tuple_detail::MakeIndexSequenceFromTuple<Tuples>{})...),
        tuple_detail::MakeIndexSequenceFromTuple<Tuples...>{})) {
  return tuple_detail::call(
      tuple_detail::makeCallable(std::forward<F>(f)),
      std::tuple_cat(tuple_detail::forwardTuple(
          std::forward<Tuples>(t),
          tuple_detail::MakeIndexSequenceFromTuple<Tuples>{})...),
      tuple_detail::MakeIndexSequenceFromTuple<Tuples...>{});
}

/**
 *  Backports from C++17 of:
 *    std::in_place_t
 *    std::in_place_type_t
 *    std::in_place_index_t
 *    std::in_place
 *    std::in_place_type
 *    std::in_place_index
 */

struct in_place_tag {};
template <class>
struct in_place_type_tag {};
template <std::size_t>
struct in_place_index_tag {};

using in_place_t = in_place_tag (&)(in_place_tag);
template <class T>
using in_place_type_t = in_place_type_tag<T> (&)(in_place_type_tag<T>);
template <std::size_t I>
using in_place_index_t = in_place_index_tag<I> (&)(in_place_index_tag<I>);

inline in_place_tag in_place(in_place_tag = {}) {
  return {};
}
template <class T>
inline in_place_type_tag<T> in_place_type(in_place_type_tag<T> = {}) {
  return {};
}
template <std::size_t I>
inline in_place_index_tag<I> in_place_index(in_place_index_tag<I> = {}) {
  return {};
}

} // namespace rdd
