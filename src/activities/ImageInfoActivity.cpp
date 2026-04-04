#include "ImageInfoActivity.h"

#include <Logging.h>

#include "fontIds.h"

void ImageInfoActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("IMGINFO", "Showing info for: %s", path);
  needsRedraw = true;
  requestUpdate();
}

void ImageInfoActivity::onExit() { Activity::onExit(); }

void ImageInfoActivity::loop() {
  // TODO: handle Back button
}

void ImageInfoActivity::render(RenderLock&& lock) {
  if (!needsRedraw) return;
  needsRedraw = false;

  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 220, "Image Info");
  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}
