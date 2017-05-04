/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include "rddoc/io/FSUtil.h"

namespace rdd {

// https://rawgit.com/google/leveldb/master/doc/index.html

class LevelDB {
public:
  typedef leveldb::Iterator Iterator;
  typedef leveldb::WriteBatch Batch;

  static bool Destroy(const fs::path& dir);

  LevelDB() : db_(nullptr) {}

  ~LevelDB() {
    close();
  }

  bool open(const fs::path& dir);

  void close() {
    if (db_ != nullptr) {
      delete db_;
    }
    db_ = nullptr;
  }

  operator bool() const {
    return db_ != nullptr;
  }

  Iterator* iterator() {
    return db_->NewIterator(leveldb::ReadOptions());
  }

  bool Get(const std::string& key, std::string& value);
  bool Put(const std::string& key, const std::string& value, bool sync = false);
  bool Delete(const std::string& key, bool sync = false);
  bool Write(Batch* batch, bool sync = true);

private:
  leveldb::DB* db_;
};

}

