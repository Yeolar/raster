/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/io/Compression.h"
#include "rddoc/util/Hash.h"
#include "rddoc/util/String.h"

#include <random>
#include <thread>
#include <unordered_map>
#include <boost/noncopyable.hpp>
#include <gtest/gtest.h>

namespace rdd { namespace io { namespace test {

TEST(CompressionTest, Example) {
  std::string buf = "aaaabbbbccccdddd";
  ByteRange data = ByteRange((uint8_t*)buf.data(), buf.size());

  std::unique_ptr<Codec> codec = getCodec(CodecType::ZLIB);
  auto original = IOBuf::wrapBuffer(data);
  auto compressed = codec->compress(original.get());
  auto uncompressed = codec->uncompress(compressed.get());

  std::string str;
  for (auto& range : *compressed) {
    hexlify(range, str, true);
  }
  EXPECT_EQ("789c4b4c4c4c4c028264204801020034140629", str);
  str = "";
  for (auto& range : *uncompressed) {
    str += range.str();
  }
  EXPECT_EQ("aaaabbbbccccdddd", str);
}

class DataHolder : private boost::noncopyable {
 public:
  uint64_t hash(size_t size) const;
  ByteRange data(size_t size) const;

 protected:
  explicit DataHolder(size_t sizeLog2);
  const size_t size_;
  std::unique_ptr<uint8_t[]> data_;
  mutable std::unordered_map<uint64_t, uint64_t> hashCache_;
};

DataHolder::DataHolder(size_t sizeLog2)
  : size_(size_t(1) << sizeLog2),
    data_(new uint8_t[size_]) {
}

uint64_t DataHolder::hash(size_t size) const {
  auto p = hashCache_.find(size);
  if (p != hashCache_.end()) {
    return p->second;
  }

  uint64_t h = rdd::hash::fnv64_buf(data_.get(), size);
  hashCache_[size] = h;
  return h;
}

ByteRange DataHolder::data(size_t size) const {
  return ByteRange(data_.get(), size);
}

uint64_t hashIOBuf(const IOBuf* buf) {
  uint64_t h = rdd::hash::FNV_64_HASH_START;
  for (auto& range : *buf) {
    h = rdd::hash::fnv64_buf(range.data(), range.size(), h);
  }
  return h;
}

class ConstantDataHolder : public DataHolder {
 public:
  explicit ConstantDataHolder(size_t sizeLog2);
};

ConstantDataHolder::ConstantDataHolder(size_t sizeLog2)
  : DataHolder(sizeLog2) {
  memset(data_.get(), 'a', size_);
}

constexpr size_t dataSizeLog2 = 27;  // 128MiB
ConstantDataHolder constantDataHolder(dataSizeLog2);

TEST(CompressionTestNeedsUncompressedLength, Simple) {
  EXPECT_FALSE(getCodec(CodecType::NO_COMPRESSION)->needsUncompressedLength());
  EXPECT_FALSE(getCodec(CodecType::ZLIB)->needsUncompressedLength());
}

class CompressionTest
    : public testing::TestWithParam<std::tr1::tuple<int, CodecType>> {
  protected:
   void SetUp() override {
     auto tup = GetParam();
     uncompressedLength_ = uint64_t(1) << std::tr1::get<0>(tup);
     codec_ = getCodec(std::tr1::get<1>(tup));
   }

   void runSimpleTest(const DataHolder& dh);

   uint64_t uncompressedLength_;
   std::unique_ptr<Codec> codec_;
};

void CompressionTest::runSimpleTest(const DataHolder& dh) {
  auto original = IOBuf::wrapBuffer(dh.data(uncompressedLength_));
  auto compressed = codec_->compress(original.get());
  if (!codec_->needsUncompressedLength()) {
    auto uncompressed = codec_->uncompress(compressed.get());

    EXPECT_EQ(uncompressedLength_, uncompressed->computeChainDataLength());
    EXPECT_EQ(dh.hash(uncompressedLength_), hashIOBuf(uncompressed.get()));
  }
  {
    auto uncompressed = codec_->uncompress(compressed.get(),
                                           uncompressedLength_);
    EXPECT_EQ(uncompressedLength_, uncompressed->computeChainDataLength());
    EXPECT_EQ(dh.hash(uncompressedLength_), hashIOBuf(uncompressed.get()));
  }
}

TEST_P(CompressionTest, ConstantData) {
  runSimpleTest(constantDataHolder);
}

INSTANTIATE_TEST_CASE_P(
    CompressionTest,
    CompressionTest,
    testing::Combine(testing::Values(0, 1, 12, 22, 25, 27),
                     testing::Values(CodecType::NO_COMPRESSION,
                                     CodecType::ZLIB)));

}}}  // namespaces

