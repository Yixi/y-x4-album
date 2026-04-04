#include "ImageIndex.h"

#include <Logging.h>
#include <cstdlib>
#include <cstring>

static const ImageEntry kEmptyEntry = {};

ImageIndex::~ImageIndex() { clear(); }

bool ImageIndex::build(const char* dirPath, int maxEntries) {
  clear();

  snprintf(dirPath_, sizeof(dirPath_), "%s", dirPath);

  entries_ = static_cast<ImageEntry*>(malloc(sizeof(ImageEntry) * maxEntries));
  if (!entries_) {
    LOG_ERR("INDEX", "Failed to allocate %d entries", maxEntries);
    return false;
  }
  capacity_ = maxEntries;

  count_ = ImageScanner::scanDirectory(dirPath, entries_, maxEntries);
  LOG_INF("INDEX", "Built index: %d images in %s", count_, dirPath);
  return true;
}

void ImageIndex::clear() {
  if (entries_) {
    free(entries_);
    entries_ = nullptr;
  }
  count_ = 0;
  capacity_ = 0;
  dirPath_[0] = '\0';
}

void ImageIndex::sort(SortMode mode) {
  // TODO: implement sorting
  LOG_DBG("INDEX", "Sort mode: %d", static_cast<int>(mode));
}

const ImageEntry& ImageIndex::at(int index) const {
  if (index < 0 || index >= count_) return kEmptyEntry;
  return entries_[index];
}

int ImageIndex::pageCount(int pageSize) const {
  if (pageSize <= 0 || count_ == 0) return 0;
  return (count_ + pageSize - 1) / pageSize;
}

int ImageIndex::pageOf(int index, int pageSize) const {
  if (pageSize <= 0) return 0;
  return index / pageSize;
}

void ImageIndex::getFullPath(int index, char* outPath, int maxLen) const {
  if (index < 0 || index >= count_) {
    outPath[0] = '\0';
    return;
  }
  snprintf(outPath, maxLen, "%s/%s", dirPath_, entries_[index].filename);
}

int ImageIndex::findByFilename(const char* filename) const {
  for (int i = 0; i < count_; i++) {
    if (strcmp(entries_[i].filename, filename) == 0) return i;
  }
  return -1;
}
