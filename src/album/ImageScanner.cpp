#include "ImageScanner.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>

int ImageScanner::scanDirectory(const char* dirPath, ImageEntry* entries, int maxEntries) {
  if (!entries || maxEntries <= 0) return 0;

  FsFile dir = Storage.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    LOG_ERR("SCANNER", "Cannot open directory: %s", dirPath);
    return 0;
  }

  int count = 0;
  char name[128];

  for (FsFile file = dir.openNextFile(); file && count < maxEntries;
       file = dir.openNextFile()) {
    if (file.isDirectory()) {
      file.close();
      continue;
    }

    file.getName(name, sizeof(name));

    ImageFormat fmt = detectFormat(name);
    if (fmt == ImageFormat::Unknown) {
      file.close();
      continue;
    }

    // Skip hidden files (starting with '.')
    if (name[0] == '.') {
      file.close();
      continue;
    }

    // Store filename only (ImageIndex prepends dirPath when full path is needed)
    strncpy(entries[count].filename, name, sizeof(entries[count].filename) - 1);
    entries[count].filename[sizeof(entries[count].filename) - 1] = '\0';
    entries[count].fileSize = static_cast<uint32_t>(file.fileSize());
    entries[count].modTime = 0;  // HalFile doesn't expose modification time
    entries[count].format = fmt;
    count++;

    file.close();
  }

  dir.close();

  // Sort by filename (case-insensitive, insertion sort — no heap, O(n²) fine for ≤200 files)
  for (int i = 1; i < count; i++) {
    ImageEntry tmp;
    memcpy(&tmp, &entries[i], sizeof(ImageEntry));
    int j = i - 1;
    while (j >= 0 && strcasecmp(entries[j].filename, tmp.filename) > 0) {
      memcpy(&entries[j + 1], &entries[j], sizeof(ImageEntry));
      j--;
    }
    memcpy(&entries[j + 1], &tmp, sizeof(ImageEntry));
  }

  LOG_INF("SCANNER", "Found %d images in %s", count, dirPath);
  return count;
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
