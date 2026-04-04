#pragma once

#include <GfxRenderer.h>

#include <cstdint>

#include "ImageScanner.h"

struct Rect {
  int x, y, width, height;
};

enum class ScaleMode : uint8_t { Fit, Fill, Stretch };

struct ImageInfo {
  int width;
  int height;
  ImageFormat format;
  uint32_t fileSize;
};

class ImageDecoder {
 public:
  static bool getImageInfo(const char* path, ImageInfo& info);

  static bool decodeGrayscale(const char* path, GfxRenderer& renderer,
                              int screenW, int screenH,
                              ScaleMode mode, bool bgWhite);

  static bool decodeBW(const char* path, GfxRenderer& renderer,
                       int screenW, int screenH,
                       ScaleMode mode, bool bgWhite);

  static bool generateThumbnail(const char* srcPath, const char* dstPath,
                                int maxWidth, int maxHeight);

 private:
  static bool decodeJpeg(const char* path, GfxRenderer& renderer,
                         int screenW, int screenH, ScaleMode mode,
                         bool grayscaleLsb);
  static bool decodePng(const char* path, GfxRenderer& renderer,
                        int screenW, int screenH, ScaleMode mode,
                        bool grayscaleLsb);
  static bool decodeBmp(const char* path, GfxRenderer& renderer,
                        int screenW, int screenH, ScaleMode mode);

  static int calcJpegScale(int imgW, int imgH, int targetW, int targetH);
  static Rect calcDestRect(int imgW, int imgH, int screenW, int screenH,
                           ScaleMode mode);
};
