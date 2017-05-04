/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>

namespace rdd {

template <class Map>
typename Map::mapped_type get_default(
    const Map& map, const typename Map::key_type& key,
    const typename Map::mapped_type& dflt = typename Map::mapped_type()) {
  auto pos = map.find(key);
  return (pos != map.end() ? pos->second : dflt);
}

template <class Map>
typename Map::mapped_type get_or_throw(
    const Map& map, const typename Map::key_type& key) {
  auto pos = map.find(key);
  if (pos != map.end()) {
    return pos->second;
  }
  throw std::out_of_range(key);
}

template <class Map>
const typename Map::mapped_type& get_ref_default(
    const Map& map, const typename Map::key_type& key,
    const typename Map::mapped_type& dflt) {
  auto pos = map.find(key);
  return (pos != map.end() ? pos->second : dflt);
}

template <class Map>
const typename Map::mapped_type& get_ref_or_throw(
    const Map& map, const typename Map::key_type& key) {
  auto pos = map.find(key);
  if (pos != map.end()) {
    return pos->second;
  }
  throw std::out_of_range(key);
}

template <class Map>
const typename Map::mapped_type* get_ptr(
    const Map& map, const typename Map::key_type& key) {
  auto pos = map.find(key);
  return (pos != map.end() ? &pos->second : nullptr);
}

template <class Map>
typename Map::mapped_type* get_ptr(
    Map& map, const typename Map::key_type& key) {
  auto pos = map.find(key);
  return (pos != map.end() ? &pos->second : nullptr);
}

}

