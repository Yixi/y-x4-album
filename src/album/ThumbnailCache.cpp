#include "ThumbnailCache.h"

#include <Logging.h>
#include <cstdio>
#include <cstring>

bool ThumbnailCache::init() {
  // TODO: ensure cache directory exists on SD card
  LOG_DBG("THUMB", "Initializing thumbnail cache");
  return true;
}

bool ThumbnailCache::exists(const char* imagePath, uint32_t fileSize, uint32_t modTime) {
  char cachePath[256];
  getCachePath(imagePath, fileSize, modTime, cachePath, sizeof(cachePath));
  // TODO: check if file exists on SD card
  return false;
}

void ThumbnailCache::getCachePath(const char* imagePath, uint32_t fileSize, uint32_t modTime,
                                  char* outPath, int maxLen) {
  char keyBuf[512];
  snprintf(keyBuf, sizeof(keyBuf), "%s:%u:%u", imagePath, fileSize, modTime);
  uint32_t hash = fnv1aHash(keyBuf);
  snprintf(outPath, maxLen, "%s/%08x.bmp", CACHE_DIR, hash);
}

bool ThumbnailCache::generate(const char* imagePath, uint32_t fileSize, uint32_t modTime) {
  // TODO: decode source image, scale to thumbnail, write 1-bit BMP
  LOG_DBG("THUMB", "Generating thumbnail for: %s", imagePath);
  return false;
}

bool ThumbnailCache::clearAll() {
  // TODO: delete all files in cache directory
  LOG_INF("THUMB", "Clearing all cached thumbnails");
  return true;
}

int ThumbnailCache::getCachedCount() {
  // TODO: count files in cache directory
  return 0;
}

uint32_t ThumbnailCache::fnv1aHash(const char* str) {
  uint32_t hash = 0x811c9dc5;
  while (*str) {
    hash ^= static_cast<uint8_t>(*str++);
    hash *= 0x01000193;
  }
  return hash;
}
