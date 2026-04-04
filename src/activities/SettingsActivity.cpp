#include "SettingsActivity.h"

#include <Logging.h>

#include "fontIds.h"

void SettingsActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("SETTINGS", "Opening settings");
  needsRedraw = true;
  requestUpdate();
}

void SettingsActivity::onExit() { Activity::onExit(); }

void SettingsActivity::loop() {
  // TODO: handle button navigation and setting changes
}

void SettingsActivity::render(RenderLock&& lock) {
  if (!needsRedraw) return;
  needsRedraw = false;

  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 220, "Settings");
  renderer.displayBuffer(GfxRenderer::FULL_REFRESH);
}
