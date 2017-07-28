/*
 * Copyright (C) 2017, Yeolar
 */

// copy from boost

#pragma once

namespace rdd {

namespace noncopyable_  // protection from unintended ADL
{
  class noncopyable
  {
   protected:
    noncopyable() {}
    ~noncopyable() {}
   private:  // emphasize the following members are private
    noncopyable( const noncopyable& );
    const noncopyable& operator=( const noncopyable& );
  };
}

typedef noncopyable_::noncopyable noncopyable;

} // namespace rdd
