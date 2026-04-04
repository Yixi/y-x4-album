#include "ViewerActivity.h"

#include <Logging.h>

void ViewerActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("VIEWER", "Opening image index: %d", currentIndex);
  requestUpdate();
}

void ViewerActivity::onExit() { Activity::onExit(); }

void ViewerActivity::loop() {
  // TODO: handle button navigation (prev/next image, overlay toggle)
}

void ViewerActivity::render(RenderLock&& lock) {
  renderer.clearScreen(0xFF);
  // TODO: decode and render image
  renderer.displayBuffer(GfxRenderer::FULL_REFRESH);
}
