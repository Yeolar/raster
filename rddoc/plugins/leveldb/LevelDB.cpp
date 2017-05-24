/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/plugins/leveldb/LevelDB.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/ScopeGuard.h"

namespace rdd {

// https://rawgit.com/google/leveldb/master/doc/index.html

LevelDB::Status LevelDB::Destroy(const fs::path& dir) {
  LevelDB::Status s = leveldb::DestroyDB(dir.string(), leveldb::Options());
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: destroy " << dir << " error: " << s.ToString();
  }
  return s;
}

LevelDB::Status LevelDB::open(const fs::path& dir) {
  leveldb::Options opt;
  opt.create_if_missing = true;
  LevelDB::Status s = leveldb::DB::Open(opt, dir.string(), &db_);
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: open " << dir << " error: " << s.ToString();
  }
  return s;
}

LevelDB::Status LevelDB::foreach(const IterFunc& func) {
  RDDCHECK(db_) << "leveldb: db not opened";
  auto it = iterator();
  SCOPE_EXIT {
    delete it;
  };
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    func(it);
  }
  LevelDB::Status s = it->status();
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: iterate error: " << s.ToString();
  }
  return s;
}

LevelDB::Status LevelDB::Get(const std::string& key, std::string& value) {
  RDDCHECK(db_) << "leveldb: db not opened";
  LevelDB::Status s = db_->Get(leveldb::ReadOptions(), key, &value);
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: get (" << key << ")"
      << " error: " << s.ToString();
  }
  return s;
}

LevelDB::Status LevelDB::Put(const std::string& key, const std::string& value,
                             bool sync) {
  RDDCHECK(db_) << "leveldb: db not opened";
  leveldb::WriteOptions opt;
  opt.sync = sync;
  LevelDB::Status s = db_->Put(opt, key, value);
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: put (" << key << "," << value << ")"
      << " error: " << s.ToString();
  }
  return s;
}

LevelDB::Status LevelDB::Delete(const std::string& key, bool sync) {
  RDDCHECK(db_) << "leveldb: db not opened";
  leveldb::WriteOptions opt;
  opt.sync = sync;
  LevelDB::Status s = db_->Delete(opt, key);
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: delete (" << key << ")"
      << " error: " << s.ToString();
  }
  return s;
}

LevelDB::Status LevelDB::Write(Batch* batch, bool sync) {
  RDDCHECK(db_) << "leveldb: db not opened";
  leveldb::WriteOptions opt;
  opt.sync = sync;
  LevelDB::Status s = db_->Write(opt, batch);
  if (!s.ok()) {
    RDDLOG(ERROR) << "leveldb: update batch error: " << s.ToString();
  }
  return s;
}

}

