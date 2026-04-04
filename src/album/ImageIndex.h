#pragma once

#include "ImageScanner.h"

#include <cstdint>

enum class SortMode : uint8_t {
  NameAsc, NameDesc, TimeNewest, TimeOldest, SizeLargest
};

class ImageIndex {
 public:
  ImageIndex() = default;
  ~ImageIndex();

  bool build(const char* dirPath, int maxEntries = 200);
  void clear();

  void sort(SortMode mode);

  int count() const { return count_; }
  const ImageEntry& at(int index) const;

  int pageCount(int pageSize) const;
  int pageOf(int index, int pageSize) const;

  void getFullPath(int index, char* outPath, int maxLen) const;
  int findByFilename(const char* filename) const;

 private:
  char dirPath_[256] = {};
  ImageEntry* entries_ = nullptr;
  int count_ = 0;
  int capacity_ = 0;
};
