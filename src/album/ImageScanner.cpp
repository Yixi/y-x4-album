#include "ImageScanner.h"

#include <Logging.h>
#include <cstring>

int ImageScanner::scanDirectory(const char* dirPath, ImageEntry* entries, int maxEntries) {
  // TODO: implement directory scanning with HalStorage
  LOG_DBG("SCANNER", "Scanning directory: %s (max %d)", dirPath, maxEntries);
  return 0;
}

ImageFormat ImageScanner::detectFormat(const char* filename) {
  if (!filename) return ImageFormat::Unknown;

  const char* dot = strrchr(filename, '.');
  if (!dot) return ImageFormat::Unknown;

  if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) return ImageFormat::JPEG;
  if (strcasecmp(dot, ".png") == 0) return ImageFormat::PNG;
  if (strcasecmp(dot, ".bmp") == 0) return ImageFormat::BMP;

  return ImageFormat::Unknown;
}

bool ImageScanner::isSupportedImage(const char* filename) {
  return detectFormat(filename) != ImageFormat::Unknown;
}
