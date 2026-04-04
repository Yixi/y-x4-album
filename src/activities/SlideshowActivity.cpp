#include "SlideshowActivity.h"

#include <Logging.h>

void SlideshowActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("SLIDE", "Starting slideshow at index: %d", currentIndex);
  lastSwitchTime = millis();
  requestUpdate();
}

void SlideshowActivity::onExit() { Activity::onExit(); }

void SlideshowActivity::loop() {
  // TODO: auto-advance timer, pause/resume, button handling
}

void SlideshowActivity::render(RenderLock&& lock) {
  renderer.clearScreen(0xFF);
  // TODO: render current image
  renderer.displayBuffer(GfxRenderer::FULL_REFRESH);
}
