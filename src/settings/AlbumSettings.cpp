#include "AlbumSettings.h"

#include <HalStorage.h>
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
  LOG_INF("SETTINGS", "Loading from %s", SETTINGS_PATH);

  FsFile file;
  if (!Storage.openFileForRead("SETTINGS", SETTINGS_PATH, file)) {
    LOG_INF("SETTINGS", "No settings file, using defaults");
    resetToDefaults();
    return true;
  }

  AlbumSettingsData tmp;
  int bytesRead = file.read(&tmp, sizeof(tmp));
  file.close();

  if (bytesRead != sizeof(tmp)) {
    LOG_ERR("SETTINGS", "Short read (%d/%zu bytes), using defaults", bytesRead,
            sizeof(tmp));
    resetToDefaults();
    return false;
  }

  // Verify CRC
  uint32_t expected = calculateCrc(tmp);
  if (tmp.crc32 != expected) {
    LOG_ERR("SETTINGS", "CRC mismatch (got 0x%08X, expected 0x%08X), using defaults",
            tmp.crc32, expected);
    resetToDefaults();
    return false;
  }

  // Verify version
  if (tmp.version != CURRENT_VERSION) {
    LOG_INF("SETTINGS", "Version mismatch (v%d vs v%d), using defaults",
            tmp.version, CURRENT_VERSION);
    resetToDefaults();
    return true;
  }

  memcpy(&data, &tmp, sizeof(data));
  LOG_INF("SETTINGS", "Settings loaded OK");
  return true;
}

bool AlbumSettings::saveToFile() {
  LOG_INF("SETTINGS", "Saving to %s", SETTINGS_PATH);

  // Ensure parent directory exists
  Storage.ensureDirectoryExists("/.y-x4-album");

  // Calculate and store CRC
  data.crc32 = calculateCrc(data);

  FsFile file;
  if (!Storage.openFileForWrite("SETTINGS", SETTINGS_PATH, file)) {
    LOG_ERR("SETTINGS", "Cannot open for write: %s", SETTINGS_PATH);
    return false;
  }

  size_t written = file.write(&data, sizeof(data));
  file.flush();
  file.close();

  if (written != sizeof(data)) {
    LOG_ERR("SETTINGS", "Short write (%zu/%zu bytes)", written, sizeof(data));
    return false;
  }

  LOG_INF("SETTINGS", "Settings saved OK (%zu bytes)", written);
  return true;
}

uint32_t AlbumSettings::calculateCrc(const AlbumSettingsData& d) {
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
