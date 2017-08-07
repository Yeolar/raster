/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <functional>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include "raster/io/FSUtil.h"

namespace rdd {

// https://rawgit.com/google/leveldb/master/doc/index.html

class LevelDB {
public:
  typedef leveldb::Status Status;
  typedef leveldb::WriteBatch Batch;
  typedef leveldb::Iterator Iterator;
  typedef std::function<void(Iterator*)> IterFunc;

  static Status Destroy(const fs::path& dir);

  LevelDB() : db_(nullptr) {}

  ~LevelDB() {
    close();
  }

  Status open(const fs::path& dir);

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

  Status foreach(const IterFunc& func);

  Status Get(const std::string& key, std::string& value);

  Status Put(const std::string& key, const std::string& value,
             bool sync = false);

  Status Delete(const std::string& key, bool sync = false);

  Status Write(Batch* batch, bool sync = true);

private:
  leveldb::DB* db_;
};

} // namespace rdd
