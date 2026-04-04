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
  // Recursively scan directory and subdirectories for image files.
  // Stores relative paths from dirPath (e.g., "subfolder/photo.jpg").
  static int scanDirectory(const char* dirPath, ImageEntry* entries, int maxEntries);
  static ImageFormat detectFormat(const char* filename);
  static bool isSupportedImage(const char* filename);

 private:
  static int scanRecursive(const char* basePath, const char* relPath,
                           ImageEntry* entries, int maxEntries, int count, int depth);
};
