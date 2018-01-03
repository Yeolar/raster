/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <vector>

#include "raster/io/File.h"
#include "raster/io/Path.h"
#include "raster/util/Random.h"
#include "raster/util/String.h"

namespace rdd {

Path currentPath();

Path absolute(const Path& path, const Path& base = currentPath());

Path canonical(const Path& path, const Path& base = currentPath());

std::vector<Path> ls(const Path& path);

std::vector<Path> ls(const File& dir);

void createDirectory(const Path& path);

void createDirectory(const File& dir, const Path& child);

File openDirectory(const Path& path);

File openDirectory(const File& dir, const Path& child);

File openFile(const File& dir, const Path& child, int flags);

File tryOpenFile(const File& dir, const Path& child, int flags);

void remove(const Path& path);

void removeFile(const File& dir, const Path& path);

void rename(const Path& oldPath, const Path& newPath);

void rename(const File& oldDir,
            const Path& oldChild,
            const File& newDir,
            const Path& newChild);

void syncDirectory(const Path& path);

Path tempDirectoryPath();

Path uniquePath(const std::string& model = "%%%%%%%%%%%%%%%%");

Path generateUniquePath(Path path, StringPiece namePrefix);

} // namespace rdd
