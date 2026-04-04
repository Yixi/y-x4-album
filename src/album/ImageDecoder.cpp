#include "ImageDecoder.h"

#include <Bitmap.h>
#include <BitmapHelpers.h>
#include <HalStorage.h>
#include <JPEGDEC.h>
#include <JpegToBmpConverter.h>
#include <Logging.h>
#include <PNGdec.h>
#include <PngToBmpConverter.h>

#include <cstring>

#include "settings/AlbumSettings.h"

// ── JPEG file I/O callbacks for JPEGDEC ────────────────────────────────────

struct JpegContext {
  GfxRenderer* renderer;
  Rect dest;
  int imgW;
  int imgH;
  AtkinsonDitherer* ditherer;
};

static void* jpegOpenCb(const char* filename, int32_t* pFileSize) {
  auto* file = new FsFile();
  if (!Storage.openFileForRead("JPEG", filename, *file)) {
    delete file;
    return nullptr;
  }
  *pFileSize = file->fileSize();
  return file;
}

static void jpegCloseCb(void* pHandle) {
  auto* file = static_cast<FsFile*>(pHandle);
  file->close();
  delete file;
}

static int32_t jpegReadCb(JPEGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->read(pBuf, iLen);
}

static int32_t jpegSeekCb(JPEGFILE* pFile, int32_t iPosition) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->seekSet(iPosition) ? iPosition : -1;
}

// JPEG MCU draw callback: map decoded pixels to framebuffer
static int jpegDrawCb(JPEGDRAW* pDraw) {
  auto* ctx = static_cast<JpegContext*>(pDraw->pUser);
  GfxRenderer& renderer = *ctx->renderer;
  const Rect& dest = ctx->dest;

  for (int row = 0; row < pDraw->iHeight; row++) {
    int srcY = pDraw->y + row;
    // Map source Y to destination Y using nearest-neighbor
    int destY = srcY * dest.height / ctx->imgH + dest.y;
    if (destY < dest.y || destY >= dest.y + dest.height) continue;

    for (int col = 0; col < pDraw->iWidthUsed; col++) {
      int srcX = pDraw->x + col;
      int destX = srcX * dest.width / ctx->imgW + dest.x;
      if (destX < dest.x || destX >= dest.x + dest.width) continue;

      uint16_t rgb565 = pDraw->pPixels[row * pDraw->iWidth + col];
      uint8_t gray = ImageDecoder::rgb565ToGray(rgb565);

      // Dithering: quantize to 4-level (for grayscale) or 2-level (for BW)
      if (ctx->ditherer) {
        uint8_t q = ctx->ditherer->processPixel(gray, destX);
        // In GRAYSCALE_LSB mode: write LSB of 2-bit value
        // In GRAYSCALE_MSB mode: write MSB of 2-bit value
        // In BW mode: write 1-bit
        bool pixelOn = (renderer.getRenderMode() == GfxRenderer::BW) ? (q < 2) : ((q & 1) == 0);
        if (renderer.getRenderMode() == GfxRenderer::GRAYSCALE_MSB) {
          pixelOn = (q <= 1);
        }
        renderer.drawPixel(destX, destY, pixelOn);
      } else {
        // Simple threshold
        renderer.drawPixel(destX, destY, gray < 128);
      }
    }

    if (ctx->ditherer) {
      ctx->ditherer->nextRow();
    }
  }
  return 1;  // Continue decoding
}

// ── PNG file I/O callbacks for PNGdec ──────────────────────────────────────

struct PngContext {
  GfxRenderer* renderer;
  Rect dest;
  int imgW;
  int imgH;
  AtkinsonDitherer* ditherer;
  PNG* png;  // for getLineAsRGB565 in callback
};

static void* pngOpenCb(const char* filename, int32_t* pFileSize) {
  auto* file = new FsFile();
  if (!Storage.openFileForRead("PNG", filename, *file)) {
    delete file;
    return nullptr;
  }
  *pFileSize = file->fileSize();
  return file;
}

static void pngCloseCb(void* pHandle) {
  auto* file = static_cast<FsFile*>(pHandle);
  file->close();
  delete file;
}

static int32_t pngReadCb(PNGFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->read(pBuf, iLen);
}

static int32_t pngSeekCb(PNGFILE* pFile, int32_t iPosition) {
  auto* file = static_cast<FsFile*>(pFile->fHandle);
  return file->seekSet(iPosition) ? iPosition : -1;
}

// PNG scanline draw callback
static int pngDrawCb(PNGDRAW* pDraw) {
  auto* ctx = static_cast<PngContext*>(pDraw->pUser);
  GfxRenderer& renderer = *ctx->renderer;
  const Rect& dest = ctx->dest;

  int srcY = pDraw->y;
  int destY = srcY * dest.height / ctx->imgH + dest.y;
  if (destY < dest.y || destY >= dest.y + dest.height) return 1;

  // Get RGB565 line for efficient processing
  if (pDraw->iWidth > 2048) return 1;

  static uint16_t lineBuf[2048];  // 4KB static buffer (not on stack)
  ctx->png->getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFFFF);

  for (int col = 0; col < pDraw->iWidth; col++) {
    int destX = col * dest.width / ctx->imgW + dest.x;
    if (destX < dest.x || destX >= dest.x + dest.width) continue;

    uint8_t gray = ImageDecoder::rgb565ToGray(lineBuf[col]);

    if (ctx->ditherer) {
      uint8_t q = ctx->ditherer->processPixel(gray, destX);
      bool pixelOn;
      if (renderer.getRenderMode() == GfxRenderer::BW) {
        pixelOn = (q < 2);
      } else if (renderer.getRenderMode() == GfxRenderer::GRAYSCALE_MSB) {
        pixelOn = (q <= 1);
      } else {
        pixelOn = ((q & 1) == 0);
      }
      renderer.drawPixel(destX, destY, pixelOn);
    } else {
      renderer.drawPixel(destX, destY, gray < 128);
    }
  }

  if (ctx->ditherer) {
    ctx->ditherer->nextRow();
  }
  return 1;
}

// ── ImageDecoder public API ────────────────────────────────────────────────

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
    if (jpeg->open(path, jpegOpenCb, jpegCloseCb, jpegReadCb, jpegSeekCb, nullptr)) {
      info.width = jpeg->getWidth();
      info.height = jpeg->getHeight();
      jpeg->close();
      ok = true;
    }
    delete jpeg;
  } else if (fmt == ImageFormat::PNG) {
    auto* png = new PNG();
    if (!png) { file.close(); return false; }
    if (png->open(path, pngOpenCb, pngCloseCb, pngReadCb, pngSeekCb, nullptr) == PNG_SUCCESS) {
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

bool ImageDecoder::decodeGrayscale(const char* path, GfxRenderer& renderer,
                                   int screenW, int screenH,
                                   ScaleMode mode, bool bgWhite) {
  LOG_INF("DECODER", "Grayscale decode: %s", path);

  ImageInfo info;
  if (!getImageInfo(path, info)) return false;

  // Calculate scaled image dimensions after JPEG downsampling
  int scaledW = info.width;
  int scaledH = info.height;
  if (info.format == ImageFormat::JPEG) {
    int scale = calcJpegScale(info.width, info.height, screenW, screenH);
    scaledW = info.width >> scale;
    scaledH = info.height >> scale;
  }

  Rect dest = calcDestRect(scaledW, scaledH, screenW, screenH, mode);

  // Step 1: Save BW framebuffer
  if (!renderer.storeBwBuffer()) {
    LOG_ERR("DECODER", "Failed to store BW buffer (OOM)");
    return false;
  }

  // Step 2: Render LSB plane
  renderer.clearScreen(bgWhite ? 0xFF : 0x00);
  renderer.setRenderMode(GfxRenderer::GRAYSCALE_LSB);

  bool ok = false;
  if (info.format == ImageFormat::JPEG) {
    ok = decodeJpeg(path, renderer, dest, scaledW, scaledH);
  } else if (info.format == ImageFormat::PNG) {
    ok = decodePng(path, renderer, dest);
  } else if (info.format == ImageFormat::BMP) {
    ok = decodeBmp(path, renderer, dest);
  }

  if (!ok) {
    renderer.restoreBwBuffer();
    LOG_ERR("DECODER", "LSB decode failed");
    return false;
  }

  renderer.copyGrayscaleLsbBuffers();

  // Step 3: Render MSB plane (second decode pass)
  renderer.clearScreen(bgWhite ? 0xFF : 0x00);
  renderer.setRenderMode(GfxRenderer::GRAYSCALE_MSB);

  if (info.format == ImageFormat::JPEG) {
    ok = decodeJpeg(path, renderer, dest, scaledW, scaledH);
  } else if (info.format == ImageFormat::PNG) {
    ok = decodePng(path, renderer, dest);
  } else if (info.format == ImageFormat::BMP) {
    ok = decodeBmp(path, renderer, dest);
  }

  if (!ok) {
    renderer.restoreBwBuffer();
    LOG_ERR("DECODER", "MSB decode failed");
    return false;
  }

  renderer.copyGrayscaleMsbBuffers();

  // Step 4: Display gray buffer
  renderer.displayGrayBuffer();

  // Step 5: Restore BW buffer
  renderer.restoreBwBuffer();
  renderer.setRenderMode(GfxRenderer::BW);

  LOG_INF("DECODER", "Grayscale decode complete");
  return true;
}

bool ImageDecoder::decodeBW(const char* path, GfxRenderer& renderer,
                            int screenW, int screenH,
                            ScaleMode mode, bool bgWhite) {
  LOG_INF("DECODER", "BW decode: %s", path);

  ImageInfo info;
  if (!getImageInfo(path, info)) return false;

  int scaledW = info.width;
  int scaledH = info.height;
  if (info.format == ImageFormat::JPEG) {
    int scale = calcJpegScale(info.width, info.height, screenW, screenH);
    scaledW = info.width >> scale;
    scaledH = info.height >> scale;
  }

  Rect dest = calcDestRect(scaledW, scaledH, screenW, screenH, mode);

  renderer.clearScreen(bgWhite ? 0xFF : 0x00);
  renderer.setRenderMode(GfxRenderer::BW);

  bool ok = false;
  if (info.format == ImageFormat::JPEG) {
    ok = decodeJpeg(path, renderer, dest, scaledW, scaledH);
  } else if (info.format == ImageFormat::PNG) {
    ok = decodePng(path, renderer, dest);
  } else if (info.format == ImageFormat::BMP) {
    ok = decodeBmp(path, renderer, dest);
  }

  if (!ok) {
    LOG_ERR("DECODER", "BW decode failed");
    return false;
  }

  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
  LOG_INF("DECODER", "BW decode complete");
  return true;
}

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
  // BMP thumbnails: TODO if needed

  srcFile.close();
  dstFile.close();

  if (!ok) {
    LOG_ERR("DECODER", "Thumbnail generation failed for: %s", srcPath);
    Storage.remove(dstPath);
  }
  return ok;
}

// ── Private decode methods ─────────────────────────────────────────────────

bool ImageDecoder::decodeJpeg(const char* path, GfxRenderer& renderer,
                              const Rect& dest, int imgW, int imgH) {
  auto* jpeg = new JPEGDEC();
  if (!jpeg) {
    LOG_ERR("DECODER", "JPEG alloc failed");
    return false;
  }

  // Allocate ditherer for the destination width
  AtkinsonDitherer ditherer(dest.width);

  JpegContext ctx = {&renderer, dest, imgW, imgH, &ditherer};

  if (!jpeg->open(path, jpegOpenCb, jpegCloseCb, jpegReadCb, jpegSeekCb, jpegDrawCb)) {
    LOG_ERR("DECODER", "JPEG open failed: %s", path);
    delete jpeg;
    return false;
  }

  jpeg->setUserPointer(&ctx);
  jpeg->setPixelType(RGB565_LITTLE_ENDIAN);

  // Calculate optimal downsampling
  int scale = calcJpegScale(jpeg->getWidth(), jpeg->getHeight(),
                            dest.width, dest.height);
  int options = 0;
  if (scale == 1) options = JPEG_SCALE_HALF;
  else if (scale == 2) options = JPEG_SCALE_QUARTER;
  else if (scale >= 3) options = JPEG_SCALE_EIGHTH;

  int rc = jpeg->decode(0, 0, options);
  jpeg->close();

  bool ok = (rc != 0);
  if (!ok) {
    LOG_ERR("DECODER", "JPEG decode error: %d", jpeg->getLastError());
  }
  delete jpeg;
  return ok;
}

bool ImageDecoder::decodePng(const char* path, GfxRenderer& renderer,
                             const Rect& dest) {
  auto* png = new PNG();
  if (!png) {
    LOG_ERR("DECODER", "PNG alloc failed");
    return false;
  }

  if (png->open(path, pngOpenCb, pngCloseCb, pngReadCb, pngSeekCb, pngDrawCb) != PNG_SUCCESS) {
    LOG_ERR("DECODER", "PNG open failed: %s", path);
    delete png;
    return false;
  }

  int imgW = png->getWidth();
  int imgH = png->getHeight();

  if (imgW > 2048) {
    LOG_ERR("DECODER", "PNG too wide (%d px), max 2048", imgW);
    png->close();
    delete png;
    return false;
  }

  AtkinsonDitherer ditherer(dest.width);
  PngContext ctx = {&renderer, dest, imgW, imgH, &ditherer, png};

  int rc = png->decode(&ctx, 0);
  png->close();

  bool ok = (rc == PNG_SUCCESS);
  if (!ok) {
    LOG_ERR("DECODER", "PNG decode error: %d", png->getLastError());
  }
  delete png;
  return ok;
}

bool ImageDecoder::decodeBmp(const char* path, GfxRenderer& renderer,
                             const Rect& dest) {
  FsFile file;
  if (!Storage.openFileForRead("BMP", path, file)) {
    LOG_ERR("DECODER", "BMP open failed: %s", path);
    return false;
  }

  Bitmap bmp(file);
  if (bmp.parseHeaders() != BmpReaderError::Ok) {
    LOG_ERR("DECODER", "BMP header parse failed: %s", path);
    file.close();
    return false;
  }

  // Use GfxRenderer's built-in bitmap drawing with scaling
  renderer.drawBitmap(bmp, dest.x, dest.y, dest.width, dest.height);

  file.close();
  return true;
}

// ── Utility methods ────────────────────────────────────────────────────────

int ImageDecoder::calcJpegScale(int imgW, int imgH, int targetW, int targetH) {
  for (int scale = 0; scale <= 3; scale++) {
    int div = 1 << scale;
    // Select smallest scale that keeps decoded size >= target
    if (imgW / div <= targetW * 2 && imgH / div <= targetH * 2) {
      return scale;
    }
  }
  return 3;
}

Rect ImageDecoder::calcDestRect(int imgW, int imgH, int screenW, int screenH,
                                ScaleMode mode) {
  Rect r = {0, 0, screenW, screenH};

  if (mode == ScaleMode::Stretch || imgW <= 0 || imgH <= 0) return r;

  float scaleX = static_cast<float>(screenW) / imgW;
  float scaleY = static_cast<float>(screenH) / imgH;
  float scale;

  if (mode == ScaleMode::Fit) {
    scale = (scaleX < scaleY) ? scaleX : scaleY;
  } else {  // Fill
    scale = (scaleX > scaleY) ? scaleX : scaleY;
  }

  r.width = static_cast<int>(imgW * scale);
  r.height = static_cast<int>(imgH * scale);
  r.x = (screenW - r.width) / 2;
  r.y = (screenH - r.height) / 2;
  return r;
}
