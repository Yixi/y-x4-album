#include "ImageScanner.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>

static constexpr int MAX_RECURSION_DEPTH = 5;

int ImageScanner::scanDirectory(const char* dirPath, ImageEntry* entries, int maxEntries) {
  if (!entries || maxEntries <= 0) return 0;

  int count = scanRecursive(dirPath, "", entries, maxEntries, 0, 0);

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

  LOG_INF("SCANNER", "Found %d images in %s (recursive)", count, dirPath);
  return count;
}

int ImageScanner::scanRecursive(const char* basePath, const char* relPath,
                                ImageEntry* entries, int maxEntries, int count, int depth) {
  if (depth > MAX_RECURSION_DEPTH || count >= maxEntries) return count;

  // Build full path for this directory
  char fullDir[256];
  if (relPath[0] != '\0') {
    snprintf(fullDir, sizeof(fullDir), "%s/%s", basePath, relPath);
  } else {
    strncpy(fullDir, basePath, sizeof(fullDir) - 1);
    fullDir[sizeof(fullDir) - 1] = '\0';
  }

  FsFile dir = Storage.open(fullDir);
  if (!dir || !dir.isDirectory()) {
    LOG_ERR("SCANNER", "Cannot open directory: %s", fullDir);
    return count;
  }

  char name[128];

  for (FsFile file = dir.openNextFile(); file && count < maxEntries;
       file = dir.openNextFile()) {
    file.getName(name, sizeof(name));

    // Skip hidden files/dirs (starting with '.')
    if (name[0] == '.') {
      file.close();
      continue;
    }

    if (file.isDirectory()) {
      // Recurse into subdirectory
      char childRel[256];
      if (relPath[0] != '\0') {
        snprintf(childRel, sizeof(childRel), "%s/%s", relPath, name);
      } else {
        strncpy(childRel, name, sizeof(childRel) - 1);
        childRel[sizeof(childRel) - 1] = '\0';
      }
      file.close();
      count = scanRecursive(basePath, childRel, entries, maxEntries, count, depth + 1);
      continue;
    }

    ImageFormat fmt = detectFormat(name);
    if (fmt == ImageFormat::Unknown) {
      file.close();
      continue;
    }

    // Store relative path from basePath (e.g., "subfolder/photo.jpg")
    if (relPath[0] != '\0') {
      snprintf(entries[count].filename, sizeof(entries[count].filename), "%s/%s", relPath, name);
    } else {
      strncpy(entries[count].filename, name, sizeof(entries[count].filename) - 1);
      entries[count].filename[sizeof(entries[count].filename) - 1] = '\0';
    }
    entries[count].fileSize = static_cast<uint32_t>(file.fileSize());
    entries[count].modTime = 0;
    entries[count].format = fmt;
    count++;

    file.close();
  }

  dir.close();
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
