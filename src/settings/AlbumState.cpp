#include "AlbumState.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cstring>

AlbumState AlbumState::instance;

void AlbumState::clear() {
  memset(&data, 0, sizeof(data));
  data.version = 1;
  strncpy(data.lastDir, "/", sizeof(data.lastDir));
}

bool AlbumState::loadFromFile() {
  LOG_INF("STATE", "Loading from %s", STATE_PATH);

  FsFile file;
  if (!Storage.openFileForRead("STATE", STATE_PATH, file)) {
    LOG_INF("STATE", "No state file, using defaults");
    clear();
    return true;
  }

  AlbumStateData tmp;
  int bytesRead = file.read(&tmp, sizeof(tmp));
  file.close();

  if (bytesRead != sizeof(tmp)) {
    LOG_ERR("STATE", "Short read (%d/%zu bytes), using defaults", bytesRead, sizeof(tmp));
    clear();
    return false;
  }

  uint32_t expected = calculateCrc(tmp);
  if (tmp.crc32 != expected) {
    LOG_ERR("STATE", "CRC mismatch, using defaults");
    clear();
    return false;
  }

  memcpy(&data, &tmp, sizeof(data));
  LOG_INF("STATE", "State loaded OK (lastDir=%s)", data.lastDir);
  return true;
}

bool AlbumState::saveToFile() {
  LOG_DBG("STATE", "Saving to %s", STATE_PATH);

  Storage.ensureDirectoryExists("/.y-x4-album");

  data.crc32 = calculateCrc(data);

  FsFile file;
  if (!Storage.openFileForWrite("STATE", STATE_PATH, file)) {
    LOG_ERR("STATE", "Cannot open for write: %s", STATE_PATH);
    return false;
  }

  size_t written = file.write(&data, sizeof(data));
  file.flush();
  file.close();

  if (written != sizeof(data)) {
    LOG_ERR("STATE", "Short write (%zu/%zu bytes)", written, sizeof(data));
    return false;
  }

  LOG_DBG("STATE", "State saved OK");
  return true;
}

uint32_t AlbumState::calculateCrc(const AlbumStateData& d) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&d);
  const size_t len = offsetof(AlbumStateData, crc32);

  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= bytes[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
  }
  return ~crc;
}
