#pragma once

#include <GfxRenderer.h>
#include <HalStorage.h>

#include <cstdint>

#include "ImageScanner.h"
#include "components/AlbumTheme.h"

enum class ScaleMode : uint8_t { Fit, Fill, Stretch };

struct ImageInfo {
  int width;
  int height;
  ImageFormat format;
  uint32_t fileSize;
};

class ImageDecoder {
 public:
  // Get image dimensions without full decode
  static bool getImageInfo(const char* path, ImageInfo& info);

  // Render image to screen using BMP conversion + drawBitmap (crosspoint approach).
  // Grayscale mode: converts to grayscale BMP, then 3-pass LSB/MSB rendering.
  // BW mode: converts to 1-bit BMP, single pass.
  static bool decodeGrayscale(const char* path, GfxRenderer& renderer,
                              int screenW, int screenH,
                              ScaleMode mode, bool bgWhite);

  static bool decodeBW(const char* path, GfxRenderer& renderer,
                       int screenW, int screenH,
                       ScaleMode mode, bool bgWhite);

  // Generate 1-bit BMP thumbnail on SD card
  static bool generateThumbnail(const char* srcPath, const char* dstPath,
                                int thumbW, int thumbH);

  // Get cache path for converted BMP
  static void getCacheBmpPath(const char* srcPath, char* outPath, int maxLen);

 private:
  // Convert source image to BMP on SD card (grayscale or 1-bit)
  static bool convertToBmp(const char* srcPath, const char* dstPath,
                           int targetW, int targetH, bool oneBit);

  // Render a BMP file to screen using Bitmap + drawBitmap (matching crosspoint-reader)
  static bool renderBmpGrayscale(const char* bmpPath, GfxRenderer& renderer,
                                 int screenW, int screenH, ScaleMode mode);
  static bool renderBmpBW(const char* bmpPath, GfxRenderer& renderer,
                          int screenW, int screenH, ScaleMode mode);

  // Calculate destination position for centered image
  static void calcCenterPos(int imgW, int imgH, int screenW, int screenH,
                            int& outX, int& outY);
};
