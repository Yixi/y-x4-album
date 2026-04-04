#include "AlbumSettings.h"

#include <Logging.h>
#include <cstring>

AlbumSettings AlbumSettings::instance;

void AlbumSettings::resetToDefaults() {
  memset(&data, 0, sizeof(data));
  data.version = CURRENT_VERSION;
  data.orientation = 0;
  data.renderMode = 0;
  data.scaleMode = 0;
  data.bgWhite = 1;
  data.fullRefreshInterval = 5;
  data.showStatusBar = 1;
  data.sortMode = 0;
  data.loopBrowsing = 1;
  data.galleryColumns = 5;
  data.slideshowInterval = 30;
  data.slideshowOrder = 0;
  data.slideshowLoop = 1;
  data.slideshowScope = 0;
  data.autoSleepMinutes = 10;
  data.sleepDisplay = 0;
  data.language = 0;
  data.sideButtonReversed = 0;
  strncpy(data.rootDir, "/", sizeof(data.rootDir));
}

bool AlbumSettings::loadFromFile() {
  // TODO: read from SD card, verify CRC
  LOG_INF("SETTINGS", "Loading settings from %s", SETTINGS_PATH);
  resetToDefaults();
  return true;
}

bool AlbumSettings::saveToFile() {
  // TODO: write to SD card with CRC
  LOG_INF("SETTINGS", "Saving settings to %s", SETTINGS_PATH);
  return true;
}

uint32_t AlbumSettings::calculateCrc(const AlbumSettingsData& d) {
  // CRC-32 over all data bytes except the crc32 field itself
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&d);
  const size_t len = offsetof(AlbumSettingsData, crc32);

  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= bytes[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
  }
  return ~crc;
}
