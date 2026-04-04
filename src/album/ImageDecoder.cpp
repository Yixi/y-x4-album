#include "ImageDecoder.h"

#include <Bitmap.h>
#include <HalStorage.h>
#include <JPEGDEC.h>
#include <JpegToBmpConverter.h>
#include <Logging.h>
#include <PNGdec.h>
#include <PngToBmpConverter.h>

#include <cstring>

#include "ThumbnailCache.h"
#include "settings/AlbumSettings.h"

// ── Cache path generation ─────────────────────────────────────────────────

void ImageDecoder::getCacheBmpPath(const char* srcPath, char* outPath, int maxLen) {
  // Use same FNV-1a hash as ThumbnailCache for consistency
  uint32_t hash = 2166136261u;
  for (const char* p = srcPath; *p; p++) {
    hash ^= static_cast<uint8_t>(*p);
    hash *= 16777619u;
  }
  snprintf(outPath, maxLen, "/.y-x4-album/cache/%08x.bmp", hash);
}

// ── Image info (header only, no full decode) ──────────────────────────────

// JPEG file I/O callbacks for JPEGDEC (used only in getImageInfo)
static void* jpegInfoOpenCb(const char* filename, int32_t* pFileSize) {
  auto* file = new FsFile();
  if (!Storage.openFileForRead("JPEG", filename, *file)) {
    delete file;
    return nullptr;
  }
  *pFileSize = file->fileSize();
  return file;
}

static void jpegInfoCloseCb(void* pHandle) {
  auto* file = static_cast<FsFile*>(pHandle);
  file->close();
  delete file;
}

static int32_t jpegInfoReadCb(JPEGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->read(pBuf, iLen);
}

static int32_t jpegInfoSeekCb(JPEGFILE* pFile, int32_t iPosition) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->seekSet(iPosition) ? iPosition : -1;
}

// PNG file I/O callbacks for PNGdec (used only in getImageInfo)
static void* pngInfoOpenCb(const char* filename, int32_t* pFileSize) {
  auto* file = new FsFile();
  if (!Storage.openFileForRead("PNG", filename, *file)) {
    delete file;
    return nullptr;
  }
  *pFileSize = file->fileSize();
  return file;
}

static void pngInfoCloseCb(void* pHandle) {
  auto* file = static_cast<FsFile*>(pHandle);
  file->close();
  delete file;
}

static int32_t pngInfoReadCb(PNGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->read(pBuf, iLen);
}

static int32_t pngInfoSeekCb(PNGFILE* pFile, int32_t iPosition) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->seekSet(iPosition) ? iPosition : -1;
}

bool ImageDecoder::getImageInfo(const char* path, ImageInfo& info) {
  ImageFormat fmt = ImageScanner::detectFormat(path);
  info.format = fmt;

  FsFile file;
  if (!Storage.openFileForRead("INFO", path, file)) {
    LOG_ERR("DECODER", "Cannot open: %s", path);
    return false;
  }
  info.fileSize = file.fileSize();

  bool ok = false;

  if (fmt == ImageFormat::JPEG) {
    auto* jpeg = new JPEGDEC();
    if (!jpeg) { file.close(); return false; }
    if (jpeg->open(path, jpegInfoOpenCb, jpegInfoCloseCb, jpegInfoReadCb, jpegInfoSeekCb, nullptr)) {
      info.width = jpeg->getWidth();
      info.height = jpeg->getHeight();
      jpeg->close();
      ok = true;
    }
    delete jpeg;
  } else if (fmt == ImageFormat::PNG) {
    auto* png = new PNG();
    if (!png) { file.close(); return false; }
    if (png->open(path, pngInfoOpenCb, pngInfoCloseCb, pngInfoReadCb, pngInfoSeekCb, nullptr) == PNG_SUCCESS) {
      info.width = png->getWidth();
      info.height = png->getHeight();
      png->close();
      ok = true;
    }
    delete png;
  } else if (fmt == ImageFormat::BMP) {
    Bitmap bmp(file);
    if (bmp.parseHeaders() == BmpReaderError::Ok) {
      info.width = bmp.getWidth();
      info.height = bmp.getHeight();
      ok = true;
    }
  }

  file.close();
  if (!ok) {
    LOG_ERR("DECODER", "Failed to read headers: %s", path);
  }
  return ok;
}

// ── BMP conversion (JPEG/PNG → BMP on SD card) ───────────────────────────

bool ImageDecoder::convertToBmp(const char* srcPath, const char* dstPath,
                                int targetW, int targetH, bool oneBit) {
  ImageFormat fmt = ImageScanner::detectFormat(srcPath);

  FsFile srcFile;
  if (!Storage.openFileForRead("CONV_SRC", srcPath, srcFile)) {
    LOG_ERR("DECODER", "Cannot open source: %s", srcPath);
    return false;
  }

  // Ensure cache directory exists
  Storage.mkdir("/.y-x4-album");
  Storage.mkdir("/.y-x4-album/cache");

  FsFile dstFile;
  if (!Storage.openFileForWrite("CONV_DST", dstPath, dstFile)) {
    LOG_ERR("DECODER", "Cannot create BMP: %s", dstPath);
    srcFile.close();
    return false;
  }

  bool ok = false;

  if (fmt == ImageFormat::JPEG) {
    if (oneBit) {
      ok = JpegToBmpConverter::jpegFileTo1BitBmpStreamWithSize(srcFile, dstFile, targetW, targetH);
    } else {
      ok = JpegToBmpConverter::jpegFileToBmpStreamWithSize(srcFile, dstFile, targetW, targetH);
    }
  } else if (fmt == ImageFormat::PNG) {
    if (oneBit) {
      ok = PngToBmpConverter::pngFileTo1BitBmpStreamWithSize(srcFile, dstFile, targetW, targetH);
    } else {
      ok = PngToBmpConverter::pngFileToBmpStreamWithSize(srcFile, dstFile, targetW, targetH);
    }
  }

  srcFile.close();
  dstFile.close();

  if (!ok) {
    LOG_ERR("DECODER", "BMP conversion failed: %s", srcPath);
    Storage.remove(dstPath);
  } else {
    LOG_INF("DECODER", "Converted to BMP: %s", dstPath);
  }
  return ok;
}

// ── Render BMP to screen (crosspoint-reader approach) ─────────────────────

void ImageDecoder::calcCenterPos(int imgW, int imgH, int screenW, int screenH,
                                 int& outX, int& outY) {
  float ratio = static_cast<float>(imgW) / static_cast<float>(imgH);
  float screenRatio = static_cast<float>(screenW) / static_cast<float>(screenH);

  if (imgW <= screenW && imgH <= screenH) {
    // Image fits entirely, center it
    outX = (screenW - imgW) / 2;
    outY = (screenH - imgH) / 2;
  } else if (ratio > screenRatio) {
    // Wider than screen
    outX = 0;
    outY = static_cast<int>((screenH - screenW / ratio) / 2);
  } else {
    // Taller than screen
    outX = static_cast<int>((screenW - screenH * ratio) / 2);
    outY = 0;
  }
}

bool ImageDecoder::renderBmpGrayscale(const char* bmpPath, GfxRenderer& renderer,
                                      int screenW, int screenH, ScaleMode /*mode*/) {
  FsFile file;
  if (!Storage.openFileForRead("RENDER", bmpPath, file)) {
    LOG_ERR("DECODER", "Cannot open BMP for render: %s", bmpPath);
    return false;
  }

  Bitmap bitmap(file, true);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    LOG_ERR("DECODER", "BMP header parse failed: %s", bmpPath);
    file.close();
    return false;
  }

  int x, y;
  calcCenterPos(bitmap.getWidth(), bitmap.getHeight(), screenW, screenH, x, y);

  bool hasGreyscale = bitmap.hasGreyscale();

  if (!hasGreyscale) {
    // 1-bit BMP: single BW pass with full refresh
    renderer.clearScreen();
    renderer.drawBitmap(bitmap, x, y, screenW, screenH, 0, 0);
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
  } else {
    // Grayscale BMP: BW base + LSB/MSB overlay (matches crosspoint SleepActivity)
    // Pass 1: BW base layer — display with HALF_REFRESH to clear ghosting
    renderer.clearScreen();
    renderer.drawBitmap(bitmap, x, y, screenW, screenH, 0, 0);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);

    // Pass 2: Grayscale LSB plane (no screen refresh yet)
    bitmap.rewindToData();
    renderer.clearScreen(0x00);
    renderer.setRenderMode(GfxRenderer::GRAYSCALE_LSB);
    renderer.drawBitmap(bitmap, x, y, screenW, screenH, 0, 0);
    renderer.copyGrayscaleLsbBuffers();

    // Pass 3: Grayscale MSB plane (no screen refresh yet)
    bitmap.rewindToData();
    renderer.clearScreen(0x00);
    renderer.setRenderMode(GfxRenderer::GRAYSCALE_MSB);
    renderer.drawBitmap(bitmap, x, y, screenW, screenH, 0, 0);
    renderer.copyGrayscaleMsbBuffers();

    // Single grayscale refresh — this is the only visible update
    renderer.displayGrayBuffer();
    renderer.setRenderMode(GfxRenderer::BW);
  }

  file.close();
  LOG_INF("DECODER", "Grayscale render complete");
  return true;
}

bool ImageDecoder::renderBmpBW(const char* bmpPath, GfxRenderer& renderer,
                               int screenW, int screenH, ScaleMode /*mode*/) {
  FsFile file;
  if (!Storage.openFileForRead("RENDER", bmpPath, file)) {
    LOG_ERR("DECODER", "Cannot open BMP for render: %s", bmpPath);
    return false;
  }

  Bitmap bitmap(file, true);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    LOG_ERR("DECODER", "BMP header parse failed: %s", bmpPath);
    file.close();
    return false;
  }

  int x, y;
  calcCenterPos(bitmap.getWidth(), bitmap.getHeight(), screenW, screenH, x, y);

  renderer.clearScreen();
  renderer.drawBitmap(bitmap, x, y, screenW, screenH, 0, 0);
  renderer.displayBuffer(HalDisplay::FULL_REFRESH);

  file.close();
  LOG_INF("DECODER", "BW render complete");
  return true;
}

// ── Public decode API ─────────────────────────────────────────────────────

bool ImageDecoder::decodeGrayscale(const char* path, GfxRenderer& renderer,
                                   int screenW, int screenH,
                                   ScaleMode mode, bool bgWhite) {
  LOG_INF("DECODER", "Grayscale decode: %s", path);

  ImageFormat fmt = ImageScanner::detectFormat(path);

  // BMP files can be rendered directly
  if (fmt == ImageFormat::BMP) {
    return renderBmpGrayscale(path, renderer, screenW, screenH, mode);
  }

  // JPEG/PNG: convert to grayscale BMP on SD card, then render
  char bmpPath[256];
  getCacheBmpPath(path, bmpPath, sizeof(bmpPath));

  // Check if cached BMP exists
  FsFile check;
  bool cached = Storage.openFileForRead("CACHE_CHK", bmpPath, check);
  if (cached) {
    check.close();
    LOG_DBG("DECODER", "Using cached BMP: %s", bmpPath);
  } else {
    // Convert to grayscale BMP
    if (!convertToBmp(path, bmpPath, screenW, screenH, false)) {
      LOG_ERR("DECODER", "Conversion failed, trying BW fallback");
      return decodeBW(path, renderer, screenW, screenH, mode, bgWhite);
    }
  }

  return renderBmpGrayscale(bmpPath, renderer, screenW, screenH, mode);
}

bool ImageDecoder::decodeBW(const char* path, GfxRenderer& renderer,
                            int screenW, int screenH,
                            ScaleMode mode, bool bgWhite) {
  LOG_INF("DECODER", "BW decode: %s", path);

  ImageFormat fmt = ImageScanner::detectFormat(path);

  // BMP files can be rendered directly
  if (fmt == ImageFormat::BMP) {
    return renderBmpBW(path, renderer, screenW, screenH, mode);
  }

  // JPEG/PNG: convert to 1-bit BMP on SD card, then render
  char bmpPath[256];
  getCacheBmpPath(path, bmpPath, sizeof(bmpPath));

  // For BW mode, use a different cache key to distinguish from grayscale
  // Append "_bw" before .bmp
  char bwPath[256];
  snprintf(bwPath, sizeof(bwPath), "%.*s_bw.bmp",
           (int)(strlen(bmpPath) - 4), bmpPath);

  FsFile check;
  bool cached = Storage.openFileForRead("CACHE_CHK", bwPath, check);
  if (cached) {
    check.close();
    LOG_DBG("DECODER", "Using cached BW BMP: %s", bwPath);
  } else {
    if (!convertToBmp(path, bwPath, screenW, screenH, true)) {
      LOG_ERR("DECODER", "BW conversion failed");
      return false;
    }
  }

  return renderBmpBW(bwPath, renderer, screenW, screenH, mode);
}

// ── Thumbnail generation ──────────────────────────────────────────────────

bool ImageDecoder::generateThumbnail(const char* srcPath, const char* dstPath,
                                     int thumbW, int thumbH) {
  LOG_DBG("DECODER", "Thumbnail: %s -> %s (%dx%d)", srcPath, dstPath, thumbW, thumbH);

  ImageFormat fmt = ImageScanner::detectFormat(srcPath);

  FsFile srcFile;
  if (!Storage.openFileForRead("THUMB_SRC", srcPath, srcFile)) {
    LOG_ERR("DECODER", "Cannot open source: %s", srcPath);
    return false;
  }

  FsFile dstFile;
  if (!Storage.openFileForWrite("THUMB_DST", dstPath, dstFile)) {
    LOG_ERR("DECODER", "Cannot open dest: %s", dstPath);
    srcFile.close();
    return false;
  }

  bool ok = false;
  if (fmt == ImageFormat::JPEG) {
    ok = JpegToBmpConverter::jpegFileTo1BitBmpStreamWithSize(srcFile, dstFile, thumbW, thumbH);
  } else if (fmt == ImageFormat::PNG) {
    ok = PngToBmpConverter::pngFileTo1BitBmpStreamWithSize(srcFile, dstFile, thumbW, thumbH);
  }

  srcFile.close();
  dstFile.close();

  if (!ok) {
    LOG_ERR("DECODER", "Thumbnail generation failed for: %s", srcPath);
    Storage.remove(dstPath);
  }
  return ok;
}
