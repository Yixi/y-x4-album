#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct AlbumStateData {
  uint8_t version;
  char lastDir[256];
  char lastImage[128];
  uint16_t lastGalleryPage;
  uint8_t lastGalleryIndex;
  uint32_t crc32;
};
#pragma pack(pop)

class AlbumState {
 public:
  static constexpr const char* STATE_PATH = "/.y-x4-album/state.bin";

  AlbumStateData data;

  bool loadFromFile();
  bool saveToFile();
  void clear();

  static AlbumState& getInstance() { return instance; }

 private:
  static AlbumState instance;
  static uint32_t calculateCrc(const AlbumStateData& d);
};

#define APP_STATE AlbumState::getInstance()
