#pragma once

#include <cstdint>

class ThumbnailCache {
 public:
  static constexpr int THUMB_WIDTH = 140;
  static constexpr int THUMB_HEIGHT = 120;
  static constexpr const char* CACHE_DIR = "/.y-x4-album/thumbs";

  static bool init();
  static bool exists(const char* imagePath, uint32_t fileSize, uint32_t modTime);
  static void getCachePath(const char* imagePath, uint32_t fileSize, uint32_t modTime,
                           char* outPath, int maxLen);
  static bool generate(const char* imagePath, uint32_t fileSize, uint32_t modTime);
  static bool clearAll();
  static int getCachedCount();

 private:
  static uint32_t fnv1aHash(const char* str);
};
