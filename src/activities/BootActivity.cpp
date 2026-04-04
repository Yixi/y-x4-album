#include "BootActivity.h"

#include <Logging.h>

#include "fontIds.h"

void BootActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();
}

void BootActivity::onExit() { Activity::onExit(); }

void BootActivity::render(RenderLock&& lock) {
  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 200, "Y-X4 Album");
  renderer.drawCenteredText(UI_10_FONT_ID, 240, "Loading...");
  renderer.displayBuffer(GfxRenderer::FULL_REFRESH);
}
