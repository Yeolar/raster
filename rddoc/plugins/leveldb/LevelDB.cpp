/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/plugins/leveldb/LevelDB.h"
#include "rddoc/util/Logging.h"

namespace rdd {

// https://rawgit.com/google/leveldb/master/doc/index.html

bool LevelDB::Destroy(const fs::path& dir) {
  leveldb::Status s = leveldb::DestroyDB(dir.string(), leveldb::Options());
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: destroy " << dir << " error: " << s.ToString();
    return false;
  }
  return true;
}

bool LevelDB::open(const fs::path& dir) {
  leveldb::Options opt;
  opt.create_if_missing = true;
  leveldb::Status s = leveldb::DB::Open(opt, dir.string(), &db_);
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: open " << dir << " error: " << s.ToString();
    return false;
  }
  return true;
}

bool LevelDB::Get(const std::string& key, std::string& value) {
  if (!db_) {
    RDDLOG(ERROR) << "leveldb: db not opened";
    return false;
  }
  leveldb::Status s = db_->Get(leveldb::ReadOptions(), key, &value);
  if (!s.ok()) {
    RDDLOG(V3) << "leveldb: get (" << key << ")" << " error: " << s.ToString();
    return false;
  }
  return true;
}

bool LevelDB::Put(const std::string& key, const std::string& value, bool sync) {
  if (!db_) {
    RDDLOG(ERROR) << "leveldb: db not opened";
    return false;
  }
  leveldb::WriteOptions opt;
  opt.sync = sync;
  leveldb::Status s = db_->Put(opt, key, value);
  if (!s.ok()) {
    RDDLOG(V3) << "leveldb: put (" << key << "," << value << ")"
      << " error: " << s.ToString();
    return false;
  }
  return true;
}

bool LevelDB::Delete(const std::string& key, bool sync) {
  if (!db_) {
    RDDLOG(ERROR) << "leveldb: db not opened";
    return false;
  }
  leveldb::WriteOptions opt;
  opt.sync = sync;
  leveldb::Status s = db_->Delete(opt, key);
  if (!s.ok()) {
    RDDLOG(V3) << "leveldb: delete (" << key << ")"
      << " error: " << s.ToString();
    return false;
  }
  return true;
}

bool LevelDB::Write(Batch* batch, bool sync) {
  if (!db_) {
    RDDLOG(ERROR) << "leveldb: db not opened";
    return false;
  }
  leveldb::WriteOptions opt;
  opt.sync = sync;
  leveldb::Status s = db_->Write(opt, batch);
  if (!s.ok()) {
    RDDLOG(V3) << "leveldb: update batch" << " error: " << s.ToString();
    return false;
  }
  return true;
}

}

