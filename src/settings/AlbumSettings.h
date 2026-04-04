#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct AlbumSettingsData {
  uint8_t version;

  // Display
  uint8_t orientation;
  uint8_t renderMode;
  uint8_t scaleMode;
  uint8_t bgWhite;
  uint8_t fullRefreshInterval;
  uint8_t showStatusBar;

  // Browsing
  uint8_t sortMode;
  uint8_t loopBrowsing;
  uint8_t galleryColumns;

  // Slideshow
  uint16_t slideshowInterval;
  uint8_t slideshowOrder;
  uint8_t slideshowLoop;
  uint8_t slideshowScope;

  // System
  uint16_t autoSleepMinutes;
  uint8_t sleepDisplay;
  uint8_t language;

  // Buttons
  uint8_t sideButtonReversed;

  // Album root directory
  char rootDir[128];

  // CRC
  uint32_t crc32;
};
#pragma pack(pop)

static_assert(sizeof(AlbumSettingsData) < 256, "Settings struct too large");

class AlbumSettings {
 public:
  static constexpr const char* SETTINGS_PATH = "/.y-x4-album/settings.bin";
  static constexpr uint8_t CURRENT_VERSION = 1;

  AlbumSettingsData data;

  bool loadFromFile();
  bool saveToFile();
  void resetToDefaults();

  static AlbumSettings& getInstance() { return instance; }

 private:
  static AlbumSettings instance;
  static uint32_t calculateCrc(const AlbumSettingsData& d);
};

#define SETTINGS AlbumSettings::getInstance()
