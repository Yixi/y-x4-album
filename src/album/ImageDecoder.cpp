#include "ImageDecoder.h"

#include <Logging.h>

bool ImageDecoder::getImageInfo(const char* path, ImageInfo& info) {
  // TODO: read image headers to get dimensions
  LOG_DBG("DECODER", "Getting info for: %s", path);
  return false;
}

bool ImageDecoder::decodeGrayscale(const char* path, GfxRenderer& renderer,
                                   int screenW, int screenH,
                                   ScaleMode mode, bool bgWhite) {
  // TODO: implement grayscale rendering (LSB + MSB dual pass)
  LOG_DBG("DECODER", "Decode grayscale: %s", path);
  return false;
}

bool ImageDecoder::decodeBW(const char* path, GfxRenderer& renderer,
                            int screenW, int screenH,
                            ScaleMode mode, bool bgWhite) {
  // TODO: implement BW rendering
  LOG_DBG("DECODER", "Decode BW: %s", path);
  return false;
}

bool ImageDecoder::generateThumbnail(const char* srcPath, const char* dstPath,
                                     int maxWidth, int maxHeight) {
  // TODO: generate thumbnail BMP
  LOG_DBG("DECODER", "Generate thumbnail: %s -> %s", srcPath, dstPath);
  return false;
}

bool ImageDecoder::decodeJpeg(const char* path, GfxRenderer& renderer,
                              int screenW, int screenH, ScaleMode mode,
                              bool grayscaleLsb) {
  // TODO: JPEGDEC decode
  return false;
}

bool ImageDecoder::decodePng(const char* path, GfxRenderer& renderer,
                             int screenW, int screenH, ScaleMode mode,
                             bool grayscaleLsb) {
  // TODO: PNGdec decode
  return false;
}

bool ImageDecoder::decodeBmp(const char* path, GfxRenderer& renderer,
                             int screenW, int screenH, ScaleMode mode) {
  // TODO: Bitmap row read
  return false;
}

int ImageDecoder::calcJpegScale(int imgW, int imgH, int targetW, int targetH) {
  for (int scale = 0; scale <= 3; scale++) {
    int div = 1 << scale;
    if (imgW / div <= targetW * 2 && imgH / div <= targetH * 2) {
      return scale;
    }
  }
  return 3;
}

Rect ImageDecoder::calcDestRect(int imgW, int imgH, int screenW, int screenH,
                                ScaleMode mode) {
  Rect r = {0, 0, screenW, screenH};

  if (mode == ScaleMode::Stretch) return r;

  float scaleX = static_cast<float>(screenW) / imgW;
  float scaleY = static_cast<float>(screenH) / imgH;
  float scale = (mode == ScaleMode::Fit) ? (scaleX < scaleY ? scaleX : scaleY)
                                         : (scaleX > scaleY ? scaleX : scaleY);

  r.width = static_cast<int>(imgW * scale);
  r.height = static_cast<int>(imgH * scale);
  r.x = (screenW - r.width) / 2;
  r.y = (screenH - r.height) / 2;
  return r;
}
