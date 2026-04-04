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

  // Full-screen grayscale rendering (4-level, requires two decode passes)
  static bool decodeGrayscale(const char* path, GfxRenderer& renderer,
                              int screenW, int screenH,
                              ScaleMode mode, bool bgWhite);

  // Full-screen BW rendering (1-bit, single decode pass)
  static bool decodeBW(const char* path, GfxRenderer& renderer,
                       int screenW, int screenH,
                       ScaleMode mode, bool bgWhite);

  // Generate 1-bit BMP thumbnail on SD card
  static bool generateThumbnail(const char* srcPath, const char* dstPath,
                                int thumbW, int thumbH);

  // RGB565 → 8-bit grayscale
  static inline uint8_t rgb565ToGray(uint16_t rgb565) {
    uint8_t r = (rgb565 >> 11) & 0x1F;
    uint8_t g = (rgb565 >> 5) & 0x3F;
    uint8_t b = rgb565 & 0x1F;
    // Expand to 8-bit and compute luminance
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return static_cast<uint8_t>((r * 77 + g * 150 + b * 29) >> 8);
  }

  // RGB888 → 8-bit grayscale
  static inline uint8_t rgbToGray(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t>((r * 77 + g * 150 + b * 29) >> 8);
  }

 private:
  // Per-format decode (renders directly to framebuffer via callback)
  static bool decodeJpeg(const char* path, GfxRenderer& renderer,
                         const Rect& dest, int imgW, int imgH);
  static bool decodePng(const char* path, GfxRenderer& renderer,
                        const Rect& dest);
  static bool decodeBmp(const char* path, GfxRenderer& renderer,
                        const Rect& dest);

  // JPEG downsampling factor: 0=full, 1=1/2, 2=1/4, 3=1/8
  static int calcJpegScale(int imgW, int imgH, int targetW, int targetH);

  // Calculate destination rectangle for given scale mode
  static Rect calcDestRect(int imgW, int imgH, int screenW, int screenH,
                           ScaleMode mode);
};
