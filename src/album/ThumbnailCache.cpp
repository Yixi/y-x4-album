#include "ThumbnailCache.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstdio>
#include <cstring>

#include "ImageDecoder.h"

bool ThumbnailCache::init() {
  LOG_DBG("THUMB", "Initializing thumbnail cache at %s", CACHE_DIR);
  return Storage.ensureDirectoryExists(CACHE_DIR);
}

bool ThumbnailCache::exists(const char* imagePath, uint32_t fileSize, uint32_t modTime) {
  char cachePath[256];
  getCachePath(imagePath, fileSize, modTime, cachePath, sizeof(cachePath));
  return Storage.exists(cachePath);
}

void ThumbnailCache::getCachePath(const char* imagePath, uint32_t fileSize, uint32_t modTime,
                                  char* outPath, int maxLen) {
  char keyBuf[256];
  snprintf(keyBuf, sizeof(keyBuf), "%s:%u:%u", imagePath, fileSize, modTime);
  uint32_t hash = fnv1aHash(keyBuf);
  snprintf(outPath, maxLen, "%s/%08x.bmp", CACHE_DIR, hash);
}

bool ThumbnailCache::generate(const char* imagePath, uint32_t fileSize, uint32_t modTime) {
  char cachePath[256];
  getCachePath(imagePath, fileSize, modTime, cachePath, sizeof(cachePath));

  if (Storage.exists(cachePath)) {
    return true;  // Already cached
  }

  LOG_DBG("THUMB", "Generating: %s -> %s", imagePath, cachePath);
  bool ok = ImageDecoder::generateThumbnail(imagePath, cachePath, THUMB_WIDTH, THUMB_HEIGHT);
  if (!ok) {
    LOG_ERR("THUMB", "Failed to generate thumbnail for: %s", imagePath);
  }
  return ok;
}

bool ThumbnailCache::clearAll() {
  LOG_INF("THUMB", "Clearing all cached thumbnails");

  FsFile dir = Storage.open(CACHE_DIR);
  if (!dir || !dir.isDirectory()) {
    return true;  // Nothing to clear
  }

  char name[128];
  int count = 0;

  for (FsFile file = dir.openNextFile(); file; file = dir.openNextFile()) {
    if (!file.isDirectory()) {
      file.getName(name, sizeof(name));
      char fullPath[256];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", CACHE_DIR, name);
      file.close();
      Storage.remove(fullPath);
      count++;
    } else {
      file.close();
    }
  }

  dir.close();
  LOG_INF("THUMB", "Cleared %d cached thumbnails", count);
  return true;
}

int ThumbnailCache::getCachedCount() {
  FsFile dir = Storage.open(CACHE_DIR);
  if (!dir || !dir.isDirectory()) {
    return 0;
  }

  int count = 0;
  for (FsFile file = dir.openNextFile(); file; file = dir.openNextFile()) {
    if (!file.isDirectory()) {
      count++;
    }
    file.close();
  }

  dir.close();
  return count;
}

uint32_t ThumbnailCache::fnv1aHash(const char* str) {
  uint32_t hash = 0x811c9dc5;
  while (*str) {
    hash ^= static_cast<uint8_t>(*str++);
    hash *= 0x01000193;
  }
  return hash;
}
