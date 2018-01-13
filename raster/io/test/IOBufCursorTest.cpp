/*
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

#include "raster/io/IOBuf.h"
#include "raster/io/Cursor.h"
#include "raster/util/Range.h"
#include <gtest/gtest.h>

using rdd::ByteRange;
using rdd::IOBuf;
using rdd::StringPiece;
using std::unique_ptr;
using namespace rdd::io;

TEST(IOBuf, RWCursor) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(20));
  iobuf1->append(20);
  unique_ptr<IOBuf> iobuf2(IOBuf::create(20));
  iobuf2->append(20);

  iobuf1->prependChain(std::move(iobuf2));

  EXPECT_TRUE(iobuf1->isChained());

  RWPrivateCursor wcursor(iobuf1.get());
  Cursor rcursor(iobuf1.get());
  wcursor.writeLE((uint64_t)1);
  wcursor.writeLE((uint64_t)1);
  wcursor.writeLE((uint64_t)1);
  wcursor.write((uint8_t)1);

  EXPECT_EQ(1, rcursor.readLE<uint64_t>());
  rcursor.skip(8);
  EXPECT_EQ(1, rcursor.readLE<uint32_t>());
  rcursor.skip(0);
  EXPECT_EQ(0, rcursor.read<uint8_t>());
  EXPECT_EQ(0, rcursor.read<uint8_t>());
  EXPECT_EQ(0, rcursor.read<uint8_t>());
  EXPECT_EQ(0, rcursor.read<uint8_t>());
  EXPECT_EQ(1, rcursor.read<uint8_t>());
}

TEST(IOBuf, skip) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(20));
  iobuf1->append(20);
  RWPrivateCursor wcursor(iobuf1.get());
  wcursor.write((uint8_t)1);
  wcursor.write((uint8_t)2);
  Cursor cursor(iobuf1.get());
  cursor.skip(1);
  EXPECT_EQ(2, cursor.read<uint8_t>());
}

TEST(IOBuf, reset) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(20));
  iobuf1->append(20);
  RWPrivateCursor wcursor(iobuf1.get());
  wcursor.write((uint8_t)1);
  wcursor.write((uint8_t)2);
  wcursor.reset(iobuf1.get());
  EXPECT_EQ(1, wcursor.read<uint8_t>());
}

TEST(IOBuf, copy_assign_convert) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(20));
  iobuf1->append(20);
  RWPrivateCursor wcursor(iobuf1.get());
  RWPrivateCursor cursor2(wcursor);
  RWPrivateCursor cursor3(iobuf1.get());

  wcursor.write((uint8_t)1);
  cursor3 = wcursor;
  wcursor.write((uint8_t)2);
  Cursor cursor4(wcursor);
  RWPrivateCursor cursor5(wcursor);
  wcursor.write((uint8_t)3);

  EXPECT_EQ(1, cursor2.read<uint8_t>());
  EXPECT_EQ(2, cursor3.read<uint8_t>());
  EXPECT_EQ(3, cursor4.read<uint8_t>());
}

TEST(IOBuf, arithmetic) {
  IOBuf iobuf1(IOBuf::CREATE, 20);
  iobuf1.append(20);
  RWPrivateCursor wcursor(&iobuf1);
  wcursor += 1;
  wcursor.write((uint8_t)1);
  Cursor cursor(&iobuf1);
  cursor += 1;
  EXPECT_EQ(1, cursor.read<uint8_t>());

  Cursor start(&iobuf1);
  Cursor cursor2 = start + 9;
  EXPECT_EQ(7, cursor2 - cursor);
  EXPECT_NE(cursor, cursor2);
  cursor += 8;
  cursor2 = cursor2 + 1;
  EXPECT_EQ(cursor, cursor2);
}

TEST(IOBuf, endian) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(20));
  iobuf1->append(20);
  RWPrivateCursor wcursor(iobuf1.get());
  Cursor rcursor(iobuf1.get());
  uint16_t v = 1;
  int16_t vu = -1;
  wcursor.writeBE(v);
  wcursor.writeBE(vu);
  // Try a couple combinations to ensure they were generated correctly
  wcursor.writeBE(vu);
  wcursor.writeLE(vu);
  wcursor.writeLE(vu);
  wcursor.writeLE(v);
  EXPECT_EQ(v, rcursor.readBE<uint16_t>());
}

TEST(IOBuf, Cursor) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(1));
  iobuf1->append(1);
  RWPrivateCursor c(iobuf1.get());
  c.write((uint8_t)40); // OK
  try {
    c.write((uint8_t)10); // Bad write, checked should except.
    EXPECT_EQ(true, false);
  } catch (...) {
  }
}

TEST(IOBuf, UnshareCursor) {
  uint8_t buf = 0;
  unique_ptr<IOBuf> iobuf1(IOBuf::wrapBuffer(&buf, 1));
  unique_ptr<IOBuf> iobuf2(IOBuf::wrapBuffer(&buf, 1));
  RWUnshareCursor c1(iobuf1.get());
  RWUnshareCursor c2(iobuf2.get());

  c1.write((uint8_t)10); // This should duplicate the two buffers.
  uint8_t t = c2.read<uint8_t>();
  EXPECT_EQ(0, t);

  iobuf1 = IOBuf::wrapBuffer(&buf, 1);
  iobuf2 = IOBuf::wrapBuffer(&buf, 1);
  RWPrivateCursor c3(iobuf1.get());
  RWPrivateCursor c4(iobuf2.get());

  c3.write((uint8_t)10); // This should _not_ duplicate the two buffers.
  t = c4.read<uint8_t>();
  EXPECT_EQ(10, t);
}

namespace {
void append(std::unique_ptr<IOBuf>& buf, rdd::StringPiece data) {
  EXPECT_LE(data.size(), buf->tailroom());
  memcpy(buf->writableData(), data.data(), data.size());
  buf->append(data.size());
}

void append(Appender& appender, StringPiece data) {
  appender.push(ByteRange(data));
}

std::string toString(const IOBuf& buf) {
  std::string str;
  Cursor cursor(&buf);
  std::pair<const uint8_t*, size_t> p;
  while ((p = cursor.peek()).second) {
    str.append(reinterpret_cast<const char*>(p.first), p.second);
    cursor.skip(p.second);
  }
  return str;
}

}  // namespace

TEST(IOBuf, PullAndPeek) {
  std::unique_ptr<IOBuf> iobuf1(IOBuf::create(10));
  append(iobuf1, "he");
  std::unique_ptr<IOBuf> iobuf2(IOBuf::create(10));
  append(iobuf2, "llo ");
  std::unique_ptr<IOBuf> iobuf3(IOBuf::create(10));
  append(iobuf3, "world");
  iobuf1->prependChain(std::move(iobuf2));
  iobuf1->prependChain(std::move(iobuf3));
  EXPECT_EQ(3, iobuf1->countChainElements());
  EXPECT_EQ(11, iobuf1->computeChainDataLength());

  char buf[12];
  memset(buf, 0, sizeof(buf));
  Cursor(iobuf1.get()).pull(buf, 11);
  EXPECT_EQ("hello world", std::string(buf));

  memset(buf, 0, sizeof(buf));
  EXPECT_EQ(11, Cursor(iobuf1.get()).pullAtMost(buf, 20));
  EXPECT_EQ("hello world", std::string(buf));

  EXPECT_THROW({Cursor(iobuf1.get()).pull(buf, 20);},
               std::out_of_range);

  {
    RWPrivateCursor cursor(iobuf1.get());
    auto p = cursor.peek();
    EXPECT_EQ("he", std::string(reinterpret_cast<const char*>(p.first),
                                p.second));
    cursor.skip(p.second);
    p = cursor.peek();
    EXPECT_EQ("llo ", std::string(reinterpret_cast<const char*>(p.first),
                                  p.second));
    cursor.skip(p.second);
    p = cursor.peek();
    EXPECT_EQ("world", std::string(reinterpret_cast<const char*>(p.first),
                                   p.second));
    cursor.skip(p.second);
    EXPECT_EQ(3, iobuf1->countChainElements());
    EXPECT_EQ(11, iobuf1->computeChainDataLength());
  }

  {
    RWPrivateCursor cursor(iobuf1.get());
    cursor.gather(11);
    auto p = cursor.peek();
    EXPECT_EQ("hello world", std::string(reinterpret_cast<const
                                         char*>(p.first), p.second));
    EXPECT_EQ(1, iobuf1->countChainElements());
    EXPECT_EQ(11, iobuf1->computeChainDataLength());
  }
}

TEST(IOBuf, pushCursorData) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(20));
  iobuf1->append(15);
  iobuf1->trimStart(5);
  unique_ptr<IOBuf> iobuf2(IOBuf::create(10));
  unique_ptr<IOBuf> iobuf3(IOBuf::create(10));
  iobuf3->append(10);

  iobuf1->prependChain(std::move(iobuf2));
  iobuf1->prependChain(std::move(iobuf3));
  EXPECT_TRUE(iobuf1->isChained());

  //write 20 bytes to the buffer chain
  RWPrivateCursor wcursor(iobuf1.get());
  EXPECT_FALSE(wcursor.isAtEnd());
  wcursor.writeBE<uint64_t>(1);
  wcursor.writeBE<uint64_t>(10);
  wcursor.writeBE<uint32_t>(20);
  EXPECT_TRUE(wcursor.isAtEnd());

  // create a read buffer for the buffer chain
  Cursor rcursor(iobuf1.get());
  EXPECT_EQ(1, rcursor.readBE<uint64_t>());
  EXPECT_EQ(10, rcursor.readBE<uint64_t>());
  EXPECT_EQ(20, rcursor.readBE<uint32_t>());
  EXPECT_EQ(0, rcursor.totalLength());
  rcursor.reset(iobuf1.get());
  EXPECT_EQ(20, rcursor.totalLength());

  // create another write buffer
  unique_ptr<IOBuf> iobuf4(IOBuf::create(30));
  iobuf4->append(30);
  RWPrivateCursor wcursor2(iobuf4.get());
  // write buffer chain data into it, now wcursor2 should only
  // have 10 bytes writable space
  wcursor2.push(rcursor, 20);
  EXPECT_EQ(wcursor2.totalLength(), 10);
  // write again with not enough space in rcursor
  EXPECT_THROW(wcursor2.push(rcursor, 20), std::out_of_range);

  // create a read cursor to check iobuf3 data back
  Cursor rcursor2(iobuf4.get());
  EXPECT_EQ(1, rcursor2.readBE<uint64_t>());
  EXPECT_EQ(10, rcursor2.readBE<uint64_t>());
  EXPECT_EQ(20, rcursor2.readBE<uint32_t>());

}

TEST(IOBuf, Gather) {
  std::unique_ptr<IOBuf> iobuf1(IOBuf::create(10));
  append(iobuf1, "he");
  std::unique_ptr<IOBuf> iobuf2(IOBuf::create(10));
  append(iobuf2, "llo ");
  std::unique_ptr<IOBuf> iobuf3(IOBuf::create(10));
  append(iobuf3, "world");
  iobuf1->prependChain(std::move(iobuf2));
  iobuf1->prependChain(std::move(iobuf3));
  EXPECT_EQ(3, iobuf1->countChainElements());
  EXPECT_EQ(11, iobuf1->computeChainDataLength());

  // Attempting to gather() more data than available in the chain should fail.
  // Try from the very beginning of the chain.
  RWPrivateCursor cursor(iobuf1.get());
  EXPECT_THROW(cursor.gather(15), std::overflow_error);
  // Now try from the middle of the chain
  cursor += 3;
  EXPECT_THROW(cursor.gather(10), std::overflow_error);

  // Calling gatherAtMost() should succeed, however, and just gather
  // as much as it can
  cursor.gatherAtMost(10);
  EXPECT_EQ(8, cursor.length());
  EXPECT_EQ(8, cursor.totalLength());
  EXPECT_FALSE(cursor.isAtEnd());
  EXPECT_EQ("lo world",
            rdd::StringPiece(reinterpret_cast<const char*>(cursor.data()),
                               cursor.length()));
  EXPECT_EQ(2, iobuf1->countChainElements());
  EXPECT_EQ(11, iobuf1->computeChainDataLength());

  // Now try gather again on the chain head
  cursor = RWPrivateCursor(iobuf1.get());
  cursor.gather(5);
  // Since gather() doesn't split buffers, everything should be collapsed into
  // a single buffer now.
  EXPECT_EQ(1, iobuf1->countChainElements());
  EXPECT_EQ(11, iobuf1->computeChainDataLength());
  EXPECT_EQ(11, cursor.length());
  EXPECT_EQ(11, cursor.totalLength());
}

TEST(IOBuf, cloneAndInsert) {
  std::unique_ptr<IOBuf> iobuf1(IOBuf::create(10));
  append(iobuf1, "he");
  std::unique_ptr<IOBuf> iobuf2(IOBuf::create(10));
  append(iobuf2, "llo ");
  std::unique_ptr<IOBuf> iobuf3(IOBuf::create(10));
  append(iobuf3, "world");
  iobuf1->prependChain(std::move(iobuf2));
  iobuf1->prependChain(std::move(iobuf3));
  EXPECT_EQ(3, iobuf1->countChainElements());
  EXPECT_EQ(11, iobuf1->computeChainDataLength());

  std::unique_ptr<IOBuf> cloned;

  Cursor(iobuf1.get()).clone(cloned, 3);
  EXPECT_EQ(2, cloned->countChainElements());
  EXPECT_EQ(3, cloned->computeChainDataLength());


  EXPECT_EQ(11, Cursor(iobuf1.get()).cloneAtMost(cloned, 20));
  EXPECT_EQ(3, cloned->countChainElements());
  EXPECT_EQ(11, cloned->computeChainDataLength());


  EXPECT_THROW({Cursor(iobuf1.get()).clone(cloned, 20);},
               std::out_of_range);

  {
    // Check that inserting in the middle of an iobuf splits
    RWPrivateCursor cursor(iobuf1.get());
    Cursor(iobuf1.get()).clone(cloned, 3);
    EXPECT_EQ(2, cloned->countChainElements());
    EXPECT_EQ(3, cloned->computeChainDataLength());

    cursor.skip(1);

    cursor.insert(std::move(cloned));
    cursor.insert(rdd::IOBuf::create(0));
    EXPECT_EQ(7, iobuf1->countChainElements());
    EXPECT_EQ(14, iobuf1->computeChainDataLength());
    // Check that nextBuf got set correctly to the buffer with 1 byte left
    EXPECT_EQ(1, cursor.peek().second);
    cursor.read<uint8_t>();
  }

  {
    // Check that inserting at the end doesn't create empty buf
    RWPrivateCursor cursor(iobuf1.get());
    Cursor(iobuf1.get()).clone(cloned, 1);
    EXPECT_EQ(1, cloned->countChainElements());
    EXPECT_EQ(1, cloned->computeChainDataLength());

    cursor.skip(1);

    cursor.insert(std::move(cloned));
    EXPECT_EQ(8, iobuf1->countChainElements());
    EXPECT_EQ(15, iobuf1->computeChainDataLength());
    // Check that nextBuf got set correctly
    cursor.read<uint8_t>();
  }
  {
    // Check that inserting at the beginning doesn't create empty buf
    RWPrivateCursor cursor(iobuf1.get());
    Cursor(iobuf1.get()).clone(cloned, 1);
    EXPECT_EQ(1, cloned->countChainElements());
    EXPECT_EQ(1, cloned->computeChainDataLength());

    cursor.insert(std::move(cloned));
    EXPECT_EQ(9, iobuf1->countChainElements());
    EXPECT_EQ(16, iobuf1->computeChainDataLength());
    // Check that nextBuf got set correctly
    cursor.read<uint8_t>();
  }
}

TEST(IOBuf, Appender) {
  std::unique_ptr<IOBuf> head(IOBuf::create(10));
  append(head, "hello");

  Appender app(head.get(), 10);
  uint32_t cap = head->capacity();
  uint32_t len1 = app.length();
  EXPECT_EQ(cap - 5, len1);
  app.ensure(len1);  // won't grow
  EXPECT_EQ(len1, app.length());
  app.ensure(len1 + 1);  // will grow
  EXPECT_LE(len1 + 1, app.length());

  append(app, " world");
  EXPECT_EQ("hello world", toString(*head));
}

TEST(IOBuf, Printf) {
  IOBuf head(IOBuf::CREATE, 24);
  Appender app(&head, 32);

  app.printf("%s", "test");
  EXPECT_EQ(head.length(), 4);
  EXPECT_EQ(0, memcmp(head.data(), "test\0", 5));

  app.printf("%d%s %s%s %#x", 32, "this string is",
             "longer than our original allocation size,",
             "and will therefore require a new allocation", 0x12345678);
  // The tailroom should start with a nul byte now.
  EXPECT_GE(head.prev()->tailroom(), 1);
  EXPECT_EQ(0, *head.prev()->tail());

  EXPECT_EQ("test32this string is longer than our original "
            "allocation size,and will therefore require a "
            "new allocation 0x12345678",
            head.moveToString());
}

TEST(IOBuf, CursorOperators) {
  // Test operators on a single-item chain
  {
    std::unique_ptr<IOBuf> chain1(IOBuf::create(20));
    chain1->append(10);

    Cursor curs1(chain1.get());
    EXPECT_EQ(0, curs1 - chain1.get());
    EXPECT_FALSE(curs1.isAtEnd());
    curs1.skip(3);
    EXPECT_EQ(3, curs1 - chain1.get());
    EXPECT_FALSE(curs1.isAtEnd());
    curs1.skip(7);
    EXPECT_EQ(10, curs1 - chain1.get());
    EXPECT_TRUE(curs1.isAtEnd());

    Cursor curs2(chain1.get());
    EXPECT_EQ(0, curs2 - chain1.get());
    EXPECT_EQ(10, curs1 - curs2);
    EXPECT_THROW(curs2 - curs1, std::out_of_range);
  }

  // Test cross-chain operations
  {
    std::unique_ptr<IOBuf> chain1(IOBuf::create(20));
    chain1->append(10);
    std::unique_ptr<IOBuf> chain2 = chain1->clone();

    Cursor curs1(chain1.get());
    Cursor curs2(chain2.get());
    EXPECT_THROW(curs1 - curs2, std::out_of_range);
    EXPECT_THROW(curs1 - chain2.get(), std::out_of_range);
  }

  // Test operations on multi-item chains
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(20));
    chain->append(10);
    chain->appendChain(chain->clone());
    EXPECT_EQ(20, chain->computeChainDataLength());

    Cursor curs1(chain.get());
    curs1.skip(5);
    Cursor curs2(chain.get());
    curs2.skip(3);
    EXPECT_EQ(2, curs1 - curs2);
    EXPECT_EQ(5, curs1 - chain.get());
    EXPECT_THROW(curs2 - curs1, std::out_of_range);

    curs1.skip(7);
    EXPECT_EQ(9, curs1 - curs2);
    EXPECT_EQ(12, curs1 - chain.get());
    EXPECT_THROW(curs2 - curs1, std::out_of_range);

    curs2.skip(7);
    EXPECT_EQ(2, curs1 - curs2);
    EXPECT_THROW(curs2 - curs1, std::out_of_range);
  }

  // Test isAtEnd() with empty buffers at the end of a chain
  {
    auto iobuf1 = IOBuf::create(20);
    iobuf1->append(15);
    iobuf1->trimStart(5);

    Cursor c(iobuf1.get());
    EXPECT_FALSE(c.isAtEnd());
    c.skip(10);
    EXPECT_TRUE(c.isAtEnd());

    iobuf1->prependChain(IOBuf::create(10));
    iobuf1->prependChain(IOBuf::create(10));
    EXPECT_TRUE(c.isAtEnd());
    iobuf1->prev()->append(5);
    EXPECT_FALSE(c.isAtEnd());
    c.skip(5);
    EXPECT_TRUE(c.isAtEnd());
  }
}

TEST(IOBuf, StringOperations) {
  // Test a single buffer with two null-terminated strings and an extra uint8_t
  // at the end
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(16));
    Appender app(chain.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("hello\0world\0\x01"), 13);

    Cursor curs(chain.get());
    EXPECT_STREQ("hello", curs.readTerminatedString().c_str());
    EXPECT_STREQ("world", curs.readTerminatedString().c_str());
    EXPECT_EQ(1, curs.read<uint8_t>());
  }

  // Test multiple buffers where the first is empty and the string starts in
  // the second buffer.
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(8));
    chain->prependChain(IOBuf::create(12));
    Appender app(chain.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("hello world\0"), 12);

    Cursor curs(chain.get());
    EXPECT_STREQ("hello world", curs.readTerminatedString().c_str());
  }

  // Test multiple buffers with a single null-terminated string spanning them
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(8));
    chain->prependChain(IOBuf::create(8));
    chain->append(8);
    chain->next()->append(4);
    RWPrivateCursor rwc(chain.get());
    rwc.push(reinterpret_cast<const uint8_t*>("hello world\0"), 12);

    Cursor curs(chain.get());
    EXPECT_STREQ("hello world", curs.readTerminatedString().c_str());
  }

  // Test a reading a null-terminated string that's longer than the maximum
  // allowable length
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(16));
    Appender app(chain.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("hello world\0"), 12);

    Cursor curs(chain.get());
    EXPECT_THROW(curs.readTerminatedString('\0', 5), std::length_error);
  }

  // Test reading a null-terminated string from a chain with an empty buffer at
  // the front
  {
    std::unique_ptr<IOBuf> buf(IOBuf::create(8));
    Appender app(buf.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("hello\0"), 6);
    std::unique_ptr<IOBuf> chain(IOBuf::create(8));
    chain->prependChain(std::move(buf));

    Cursor curs(chain.get());
    EXPECT_STREQ("hello", curs.readTerminatedString().c_str());
  }

  // Test reading a two fixed-length strings from a single buffer with an extra
  // uint8_t at the end
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(16));
    Appender app(chain.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("helloworld\x01"), 11);

    Cursor curs(chain.get());
    EXPECT_STREQ("hello", curs.readFixedString(5).c_str());
    EXPECT_STREQ("world", curs.readFixedString(5).c_str());
    EXPECT_EQ(1, curs.read<uint8_t>());
  }

  // Test multiple buffers where the first is empty and a fixed-length string
  // starts in the second buffer.
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(8));
    chain->prependChain(IOBuf::create(16));
    Appender app(chain.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("hello world"), 11);

    Cursor curs(chain.get());
    EXPECT_STREQ("hello world", curs.readFixedString(11).c_str());
  }

  // Test multiple buffers with a single fixed-length string spanning them
  {
    std::unique_ptr<IOBuf> chain(IOBuf::create(8));
    chain->prependChain(IOBuf::create(8));
    chain->append(7);
    chain->next()->append(4);
    RWPrivateCursor rwc(chain.get());
    rwc.push(reinterpret_cast<const uint8_t*>("hello world"), 11);

    Cursor curs(chain.get());
    EXPECT_STREQ("hello world", curs.readFixedString(11).c_str());
  }

  // Test reading a fixed-length string from a chain with an empty buffer at
  // the front
  {
    std::unique_ptr<IOBuf> buf(IOBuf::create(8));
    Appender app(buf.get(), 0);
    app.push(reinterpret_cast<const uint8_t*>("hello"), 5);
    std::unique_ptr<IOBuf> chain(IOBuf::create(8));
    chain->prependChain(std::move(buf));

    Cursor curs(chain.get());
    EXPECT_STREQ("hello", curs.readFixedString(5).c_str());
  }
}

