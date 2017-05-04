/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stddef.h>
#include <algorithm>
#include <limits>
#include <type_traits>
#include <boost/mpl/has_xxx.hpp>

namespace rdd {

template <class Container>
typename std::enable_if<
  std::is_arithmetic<typename Container::value_type>::value,
  typename Container::value_type>::type
max(const Container& container) {
  typename Container::value_type max_value =
    std::numeric_limits<typename Container::value_type>::lowest();
  for (auto& i : container) {
    if (max_value < i) {
      max_value = i;
    }
  }
  return max_value;
}

template <class Container>
typename std::enable_if<
  std::is_arithmetic<typename Container::value_type>::value,
  typename Container::value_type>::type
sum(const Container& container) {
  typename Container::value_type sum_value = 0;
  for (auto& i : container) {
    sum_value += i;
  }
  return sum_value;
}

namespace detail {
BOOST_MPL_HAS_XXX_TRAIT_DEF(mapped_type);
}

template <class Container>
typename std::enable_if<
  detail::has_mapped_type<Container>::value, bool>::type
contain(const Container& container,
        const typename Container::key_type& key) {
  return container.find(key) != container.end();
}

template <class Container>
typename std::enable_if<
  !detail::has_mapped_type<Container>::value, bool>::type
contain(const Container& container,
        const typename Container::value_type& value) {
  return std::find(container.begin(), container.end(), value)
    != container.end();
}

template <class Container>
typename std::enable_if<
  !detail::has_mapped_type<Container>::value>::type
subrange(Container& container, size_t begin, size_t end) {
  container.erase(container.begin() + std::min(end, container.size()),
                  container.end());
  container.erase(container.begin(),
                  container.begin() + std::min(begin, container.size()));
}

}

