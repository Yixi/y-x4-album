#include "FileBrowserActivity.h"

#include <Logging.h>

#include "fontIds.h"

void FileBrowserActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("BROWSER", "Opening file browser");
  needsRedraw = true;
  requestUpdate();
}

void FileBrowserActivity::onExit() { Activity::onExit(); }

void FileBrowserActivity::loop() {
  // TODO: handle button navigation
}

void FileBrowserActivity::render(RenderLock&& lock) {
  if (!needsRedraw) return;
  needsRedraw = false;

  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 220, "File Browser");
  renderer.displayBuffer(GfxRenderer::FULL_REFRESH);
}
