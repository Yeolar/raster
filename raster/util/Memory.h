/*
 * Copyright 2017 Facebook, Inc.
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <system_error>
#include <utility>
#include <unistd.h>
#include <sys/mman.h>

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

class MMapAlloc {
 private:
  size_t computeSize(size_t size) {
    long pagesize = sysconf(_SC_PAGESIZE);
    size_t mmapLength = ((size - 1) & ~(pagesize - 1)) + pagesize;
    assert(size <= mmapLength && mmapLength < size + pagesize);
    assert((mmapLength % pagesize) == 0);
    return mmapLength;
  }

 public:
  void* allocate(size_t size) {
    auto len = computeSize(size);

    // MAP_HUGETLB is a perf win, but requires cooperation from the
    // deployment environment (and a change to computeSize()).
    void* mem = static_cast<void*>(mmap(
        nullptr,
        len,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        -1,
        0));
    if (mem == reinterpret_cast<void*>(-1)) {
      throw std::system_error(errno, std::system_category());
    }
    return mem;
  }

  void deallocate(void* p, size_t size) {
    auto len = computeSize(size);
    munmap(p, len);
  }
};

template <typename Allocator>
struct GivesZeroFilledMemory : public std::false_type {};

template <>
struct GivesZeroFilledMemory<MMapAlloc> : public std::true_type {};

inline void* alignAddress(void* addr) {
  return (void*)(((size_t)addr) & ~((size_t)sysconf(_SC_PAGESIZE) - 1));
}

} // namespace rdd
