#pragma once

#include <cstdint>

enum class ImageFormat : uint8_t { JPEG, PNG, BMP, Unknown };

struct ImageEntry {
  char filename[128];
  uint32_t fileSize;
  uint32_t modTime;
  ImageFormat format;
};

class ImageScanner {
 public:
  static int scanDirectory(const char* dirPath, ImageEntry* entries, int maxEntries);
  static ImageFormat detectFormat(const char* filename);
  static bool isSupportedImage(const char* filename);
};
