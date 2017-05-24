/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/io/File.h"
#include "rddoc/util/Logging.h"
#include <gtest/gtest.h>

using namespace rdd;

namespace {
void expectWouldBlock(ssize_t r) {
  int savedErrno = errno;
  EXPECT_EQ(-1, r);
  EXPECT_EQ(EAGAIN, savedErrno) << strerror(errno);
}
void expectOK(ssize_t r) {
  EXPECT_LE(0, r) << ": errno=" << strerror(errno);
}
}  // namespace

TEST(File, Simple) {
  // Open a file, ensure it's indeed open for reading
  char buf = 'x';
  {
    File f("/etc/hosts");
    EXPECT_NE(-1, f.fd());
    EXPECT_EQ(1, ::read(f.fd(), &buf, 1));
    f.close();
    EXPECT_EQ(-1, f.fd());
  }
}

TEST(File, OwnsFd) {
  // Wrap a file descriptor, make sure that ownsFd works
  // We'll test that the file descriptor is closed by closing the writing
  // end of a pipe and making sure that a non-blocking read from the reading
  // end returns 0.

  char buf = 'x';
  int p[2];
  expectOK(::pipe(p));
  int flags = ::fcntl(p[0], F_GETFL);
  expectOK(flags);
  expectOK(::fcntl(p[0], F_SETFL, flags | O_NONBLOCK));
  expectWouldBlock(::read(p[0], &buf, 1));
  {
    File f(p[1]);
    EXPECT_EQ(p[1], f.fd());
  }
  // Ensure that moving the file doesn't close it
  {
    File f(p[1]);
    EXPECT_EQ(p[1], f.fd());
    File f1(std::move(f));
    EXPECT_EQ(-1, f.fd());
    EXPECT_EQ(p[1], f1.fd());
  }
  expectWouldBlock(::read(p[0], &buf, 1));  // not closed
  {
    File f(p[1], true);
    EXPECT_EQ(p[1], f.fd());
  }
  ssize_t r = ::read(p[0], &buf, 1);  // eof
  expectOK(r);
  EXPECT_EQ(0, r);
  ::close(p[0]);
}

TEST(File, Release) {
  File in(STDOUT_FILENO, false);
  RDDCHECK_EQ(STDOUT_FILENO, in.release());
  RDDCHECK_EQ(-1, in.release());
}
