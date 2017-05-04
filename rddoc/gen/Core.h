/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once
#define RDD_GEN_CORE_H

namespace rdd { namespace gen {

template<class Value, class Self>
class GenImpl;

template<class Self>
class Operator;

namespace detail {

template<class Self>
struct FBounded;

template<class First, class Second>
class Composed;

template<class Value, class First, class Second>
class Chain;

} // detail

}} // rdd::gen

#include "rddoc/gen/Core-inl.h"
