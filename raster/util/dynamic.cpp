/*
 * Copyright (C) 2017, Yeolar
 */

#include <bits/functexcept.h>
#include "raster/util/Hash.h"
#include "raster/util/dynamic.h"

namespace rdd {

//////////////////////////////////////////////////////////////////////

#define RDD_DYNAMIC_DEF_TYPEINFO(T) \
  constexpr const char* dynamic::TypeInfo<T>::name; \
  constexpr dynamic::Type dynamic::TypeInfo<T>::type; \
  //

RDD_DYNAMIC_DEF_TYPEINFO(void*)
RDD_DYNAMIC_DEF_TYPEINFO(bool)
RDD_DYNAMIC_DEF_TYPEINFO(std::string)
RDD_DYNAMIC_DEF_TYPEINFO(dynamic::Array)
RDD_DYNAMIC_DEF_TYPEINFO(double)
RDD_DYNAMIC_DEF_TYPEINFO(int64_t)
RDD_DYNAMIC_DEF_TYPEINFO(dynamic::ObjectImpl)

#undef RDD_DYNAMIC_DEF_TYPEINFO

const char* dynamic::typeName() const {
  return typeName(type_);
}

TypeError::TypeError(const std::string& expected, dynamic::Type actual)
  : std::runtime_error(to<std::string>("TypeError: expected dynamic "
      "type `", expected, '\'', ", but had type `",
      dynamic::typeName(actual), '\''))
{}

TypeError::TypeError(const std::string& expected,
    dynamic::Type actual1, dynamic::Type actual2)
  : std::runtime_error(to<std::string>("TypeError: expected dynamic "
      "types `", expected, '\'', ", but had types `",
      dynamic::typeName(actual1), "' and `", dynamic::typeName(actual2),
      '\''))
{}

TypeError::~TypeError() = default;

// This is a higher-order preprocessor macro to aid going from runtime
// types to the compile time type system.
#define RDD_DYNAMIC_APPLY(type, apply) \
  do {                                 \
    switch ((type)) {                  \
      case NULLT:                      \
        apply(void*);                  \
        break;                         \
      case ARRAY:                      \
        apply(Array);                  \
        break;                         \
      case BOOL:                       \
        apply(bool);                   \
        break;                         \
      case DOUBLE:                     \
        apply(double);                 \
        break;                         \
      case INT64:                      \
        apply(int64_t);                \
        break;                         \
      case OBJECT:                     \
        apply(ObjectImpl);             \
        break;                         \
      case STRING:                     \
        apply(std::string);            \
        break;                         \
      default:                         \
        RDDCHECK(0);                   \
        abort();                       \
    }                                  \
  } while (0)

bool dynamic::operator<(dynamic const& o) const {
  if (UNLIKELY(type_ == OBJECT || o.type_ == OBJECT)) {
    throw TypeError("object", type_);
  }
  if (type_ != o.type_) {
    return type_ < o.type_;
  }

#define RDD_X(T) return CompareOp<T>::comp(*getAddress<T>(),   \
                                           *o.getAddress<T>())
  RDD_DYNAMIC_APPLY(type_, RDD_X);
#undef RDD_X
}

bool dynamic::operator==(dynamic const& o) const {
  if (type() != o.type()) {
    if (isNumber() && o.isNumber()) {
      auto& integ = isInt() ? *this : o;
      auto& doubl = isInt() ? o     : *this;
      return integ.asInt() == doubl.asDouble();
    }
    return false;
  }

#define RDD_X(T) return *getAddress<T>() == *o.getAddress<T>();
  RDD_DYNAMIC_APPLY(type_, RDD_X);
#undef RDD_X
}

dynamic& dynamic::operator=(dynamic const& o) {
  if (&o != this) {
    if (type_ == o.type_) {
#define RDD_X(T) *getAddress<T>() = *o.getAddress<T>()
      RDD_DYNAMIC_APPLY(type_, RDD_X);
#undef RDD_X
    } else {
      destroy();
#define RDD_X(T) new (getAddress<T>()) T(*o.getAddress<T>())
      RDD_DYNAMIC_APPLY(o.type_, RDD_X);
#undef RDD_X
      type_ = o.type_;
    }
  }
  return *this;
}

dynamic& dynamic::operator=(dynamic&& o) noexcept {
  if (&o != this) {
    if (type_ == o.type_) {
#define RDD_X(T) *getAddress<T>() = std::move(*o.getAddress<T>())
      RDD_DYNAMIC_APPLY(type_, RDD_X);
#undef RDD_X
    } else {
      destroy();
#define RDD_X(T) new (getAddress<T>()) T(std::move(*o.getAddress<T>()))
      RDD_DYNAMIC_APPLY(o.type_, RDD_X);
#undef RDD_X
      type_ = o.type_;
    }
  }
  return *this;
}

dynamic& dynamic::operator[](dynamic const& k) & {
  if (!isObject() && !isArray()) {
    throw TypeError("object/array", type());
  }
  if (isArray()) {
    return at(k);
  }
  auto& obj = get<ObjectImpl>();
  auto ret = obj.insert({k, nullptr});
  return ret.first->second;
}

dynamic dynamic::getDefault(const dynamic& k, const dynamic& v) const& {
  auto& obj = get<ObjectImpl>();
  auto it = obj.find(k);
  return it == obj.end() ? v : it->second;
}

dynamic dynamic::getDefault(const dynamic& k, dynamic&& v) const& {
  auto& obj = get<ObjectImpl>();
  auto it = obj.find(k);
  // Avoid clang bug with ternary
  if (it == obj.end()) {
    return std::move(v);
  } else {
    return it->second;
  }
}

dynamic dynamic::getDefault(const dynamic& k, const dynamic& v) && {
  auto& obj = get<ObjectImpl>();
  auto it = obj.find(k);
  // Avoid clang bug with ternary
  if (it == obj.end()) {
    return v;
  } else {
    return std::move(it->second);
  }
}

dynamic dynamic::getDefault(const dynamic& k, dynamic&& v) && {
  auto& obj = get<ObjectImpl>();
  auto it = obj.find(k);
  return std::move(it == obj.end() ? v : it->second);
}

const dynamic* dynamic::get_ptr(dynamic const& idx) const& {
  if (auto* parray = get_nothrow<Array>()) {
    if (!idx.isInt()) {
      throw TypeError("int64", idx.type());
    }
    if (idx < 0 || idx >= parray->size()) {
      return nullptr;
    }
    return &(*parray)[idx.asInt()];
  } else if (auto* pobject = get_nothrow<ObjectImpl>()) {
    auto it = pobject->find(idx);
    if (it == pobject->end()) {
      return nullptr;
    }
    return &it->second;
  } else {
    throw TypeError("object/array", type());
  }
}

dynamic const& dynamic::at(dynamic const& idx) const& {
  if (auto* parray = get_nothrow<Array>()) {
    if (!idx.isInt()) {
      throw TypeError("int64", idx.type());
    }
    if (idx < 0 || idx >= parray->size()) {
      std::__throw_out_of_range("out of range in dynamic array");
    }
    return (*parray)[idx.asInt()];
  } else if (auto* pobject = get_nothrow<ObjectImpl>()) {
    auto it = pobject->find(idx);
    if (it == pobject->end()) {
      throw std::out_of_range(to<std::string>(
          "couldn't find key ", idx.asString(), " in dynamic object"));
    }
    return it->second;
  } else {
    throw TypeError("object/array", type());
  }
}

std::size_t dynamic::size() const {
  if (auto* ar = get_nothrow<Array>()) {
    return ar->size();
  }
  if (auto* obj = get_nothrow<ObjectImpl>()) {
    return obj->size();
  }
  if (auto* str = get_nothrow<std::string>()) {
    return str->size();
  }
  throw TypeError("array/object", type());
}

dynamic::const_iterator
dynamic::erase(const_iterator first, const_iterator last) {
  auto& arr = get<Array>();
  return get<Array>().erase(
    arr.begin() + (first - arr.begin()),
    arr.begin() + (last - arr.begin()));
}

std::size_t dynamic::hash() const {
  switch (type()) {
  case OBJECT:
  case ARRAY:
  case NULLT:
    throw TypeError("not null/object/array", type());
  case INT64:
    return std::hash<int64_t>()(getInt());
  case DOUBLE:
    return std::hash<double>()(getDouble());
  case BOOL:
    return std::hash<bool>()(getBool());
  case STRING: {
    // keep it compatible with RDDString
    const auto& str = getString();
    return ::rdd::hash::fnv32_buf(str.data(), str.size());
  }
  default:
    RDDCHECK(0); abort();
  }
}

char const* dynamic::typeName(Type t) {
#define RDD_X(T) return TypeInfo<T>::name
  RDD_DYNAMIC_APPLY(t, RDD_X);
#undef RDD_X
}

void dynamic::destroy() noexcept {
  // This short-circuit speeds up some microbenchmarks.
  if (type_ == NULLT) return;

#define RDD_X(T) detail::Destroy::destroy(getAddress<T>())
  RDD_DYNAMIC_APPLY(type_, RDD_X);
#undef RDD_X
  type_ = NULLT;
  u_.nul = nullptr;
}

} // namespace rdd
