#include "AlbumState.h"

#include <Logging.h>
#include <cstring>

AlbumState AlbumState::instance;

void AlbumState::clear() {
  memset(&data, 0, sizeof(data));
  data.version = 1;
  strncpy(data.lastDir, "/", sizeof(data.lastDir));
}

bool AlbumState::loadFromFile() {
  // TODO: read from SD card, verify CRC
  LOG_INF("STATE", "Loading state from %s", STATE_PATH);
  clear();
  return true;
}

bool AlbumState::saveToFile() {
  // TODO: write to SD card with CRC
  LOG_DBG("STATE", "Saving state to %s", STATE_PATH);
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
