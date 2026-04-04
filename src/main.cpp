#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <HalSystem.h>
#include <I18n.h>
#include <Logging.h>
#include <SPI.h>
#include <builtinFonts/all.h>

#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "activities/ActivityManager.h"
#include "components/AlbumTheme.h"
#include "fontIds.h"
#include "settings/AlbumSettings.h"
#include "settings/AlbumState.h"
#include "util/ButtonNavigator.h"

HalDisplay display;
HalGPIO gpio;
MappedInputManager mappedInputManager(gpio);
GfxRenderer renderer(display);
ActivityManager activityManager(renderer, mappedInputManager);

// UI Fonts
EpdFont ui10RegularFont(&ubuntu_10_regular);
EpdFont ui10BoldFont(&ubuntu_10_bold);
EpdFontFamily ui10FontFamily(&ui10RegularFont, &ui10BoldFont);

EpdFont ui12RegularFont(&ubuntu_12_regular);
EpdFont ui12BoldFont(&ubuntu_12_bold);
EpdFontFamily ui12FontFamily(&ui12RegularFont, &ui12BoldFont);

EpdFont smallFont(&notosans_8_regular);
EpdFontFamily smallFontFamily(&smallFont);

void enterDeepSleep() {
  HalPowerManager::Lock powerLock;
  APP_STATE.saveToFile();
  activityManager.goToSleep();
  display.deepSleep();
  LOG_DBG("MAIN", "Entering deep sleep");
  powerManager.startDeepSleep(gpio);
}

void setupDisplayAndFonts() {
  display.begin();
  renderer.begin();
  activityManager.begin();
  LOG_DBG("MAIN", "Display initialized");

  renderer.insertFont(UI_10_FONT_ID, ui10FontFamily);
  renderer.insertFont(UI_12_FONT_ID, ui12FontFamily);
  renderer.insertFont(SMALL_FONT_ID, smallFontFamily);
  LOG_DBG("MAIN", "Fonts setup");
}

void setup() {
  HalSystem::begin();
  gpio.begin();
  powerManager.begin();

  if (gpio.isUsbConnected()) {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && (millis() - start) < 3000) {
      delay(10);
    }
  }

  if (!Storage.begin()) {
    LOG_ERR("MAIN", "SD card initialization failed");
    setupDisplayAndFonts();
    // TODO: show SD card error screen
    return;
  }

  SETTINGS.loadFromFile();
  I18N.loadSettings();
  ButtonNavigator::setMappedInputManager(mappedInputManager);

  LOG_DBG("MAIN", "Starting Y-X4 Album version " ALBUM_VERSION);

  setupDisplayAndFonts();

  activityManager.goToBoot();

  APP_STATE.loadFromFile();

  // Boot to gallery
  activityManager.goToGallery(APP_STATE.data.lastDir);
}

void loop() {
  static unsigned long lastMemPrint = 0;

  gpio.update();

  if (Serial && millis() - lastMemPrint >= 10000) {
    LOG_INF("MEM", "Free: %d bytes, Min Free: %d bytes, MaxAlloc: %d bytes", ESP.getFreeHeap(),
            ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    lastMemPrint = millis();
  }

  // Auto-sleep
  static unsigned long lastActivityTime = millis();
  if (gpio.wasAnyPressed() || gpio.wasAnyReleased() || activityManager.preventAutoSleep()) {
    lastActivityTime = millis();
    powerManager.setPowerSaving(false);
  }

  const unsigned long sleepTimeoutMs = static_cast<unsigned long>(SETTINGS.data.autoSleepMinutes) * 60000UL;
  if (sleepTimeoutMs > 0 && millis() - lastActivityTime >= sleepTimeoutMs) {
    LOG_DBG("SLP", "Auto-sleep triggered after %lu ms of inactivity", sleepTimeoutMs);
    enterDeepSleep();
    return;
  }

  // Power button long press
  if (gpio.isPressed(HalGPIO::BTN_POWER) && gpio.getHeldTime() > 1500) {
    enterDeepSleep();
    return;
  }

  activityManager.loop();

  if (activityManager.skipLoopDelay()) {
    yield();
  } else {
    if (millis() - lastActivityTime >= HalPowerManager::IDLE_POWER_SAVING_MS) {
      powerManager.setPowerSaving(true);
      delay(50);
    } else {
      delay(10);
    }
  }
}
