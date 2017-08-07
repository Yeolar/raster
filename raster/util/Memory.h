/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <utility>

namespace rdd {

/**
 * For exception safety and consistency with make_shared. Erase me when
 * we have std::make_unique().
 *
 * @author Louis Brandy (ldbrandy@fb.com)
 * @author Xu Ning (xning@fb.com)
 */

template<typename T, typename Dp = std::default_delete<T>, typename... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T, Dp>>::type
make_unique(Args&&... args) {
  return std::unique_ptr<T, Dp>(new T(std::forward<Args>(args)...));
}

// Allows 'make_unique<T[]>(10)'. (N3690 s20.9.1.4 p3-4)
template<typename T, typename Dp = std::default_delete<T>>
typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T, Dp>>::type
make_unique(const size_t n) {
  return std::unique_ptr<T, Dp>(new typename std::remove_extent<T>::type[n]());
}

// Disallows 'make_unique<T[10]>()'. (N3690 s20.9.1.4 p5)
template<typename T, typename Dp = std::default_delete<T>, typename... Args>
typename std::enable_if<
  std::extent<T>::value != 0, std::unique_ptr<T, Dp>>::type
make_unique(Args&&...) = delete;

/**
 * static_function_deleter
 *
 * So you can write this:
 *
 *  using RSA_deleter = folly::static_function_deleter<RSA, &RSA_free>;
 *  auto rsa = std::unique_ptr<RSA, RSA_deleter>(RSA_new());
 *  RSA_generate_key_ex(rsa.get(), bits, exponent, nullptr);
 *  rsa = nullptr;  // calls RSA_free(rsa.get())
 *
 * This would be sweet as well for BIO, but unfortunately BIO_free has signature
 * int(BIO*) while we require signature void(BIO*). So you would need to make a
 * wrapper for it:
 *
 *  inline void BIO_free_fb(BIO* bio) { CHECK_EQ(1, BIO_free(bio)); }
 *  using BIO_deleter = folly::static_function_deleter<BIO, &BIO_free_fb>;
 *  auto buf = std::unique_ptr<BIO, BIO_deleter>(BIO_new(BIO_s_mem()));
 *  buf = nullptr;  // calls BIO_free(buf.get())
 */

template <typename T, void(*f)(T*)>
struct static_function_deleter {
  void operator()(T* t) const {
    f(t);
  }
};

/**
 * StlAllocator wraps a SimpleAllocator into a STL-compliant
 * allocator, maintaining an instance pointer to the simple allocator
 * object.  The underlying SimpleAllocator object must outlive all
 * instances of StlAllocator using it.
 */

template <class Alloc, class T>
class StlAllocator {
public:
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;
  typedef const T* const_pointer;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template <class U>
  struct rebind {
    typedef StlAllocator<Alloc, U> other;
  };

  StlAllocator() : alloc_(nullptr) {}
  explicit StlAllocator(Alloc* a) : alloc_(a) {}

  template <class U>
  StlAllocator(const StlAllocator<Alloc, U>& other) : alloc_(other.alloc()) {}

  T* address(T& x) const { return &x; }
  const T* address(const T& x) const { return &x; }

  T* allocate(size_t n, const void* hint = nullptr) {
    return static_cast<T*>(alloc_->allocate(n * sizeof(T)));
  }

  void deallocate(T* p, size_t n) {
    alloc_->deallocate(p);
  }

  size_t max_size() const {
    return std::numeric_limits<size_t>::max();
  }

  template <class... Args>
  void construct(T* p, Args&&... args) {
    new (p) T(std::forward<Args>(args)...);
  }

  void destroy(T* p) { p->~T(); }

  Alloc* alloc() const { return alloc_; }

  bool operator!=(const StlAllocator<Alloc, T>& other) const {
    return alloc_ != other.alloc_;
  }

  bool operator==(const StlAllocator<Alloc, T>& other) const {
    return alloc_ == other.alloc_;
  }

private:
  Alloc* alloc_;
};

struct CacheLocality {
  enum {
    /// Memory locations on the same cache line are subject to false
    /// sharing, which is very bad for performance.  Microbenchmarks
    /// indicate that pairs of cache lines also see interference under
    /// heavy use of atomic operations (observed for atomic increment on
    /// Sandy Bridge).
    kFalseSharingRange = 128
  };
};

/// An attribute that will cause a variable or field to be aligned so that
/// it doesn't have false sharing with anything at a smaller memory address.
#define RDD_ALIGN_TO_AVOID_FALSE_SHARING RDD_ALIGNED(128)

} // namespace rdd
