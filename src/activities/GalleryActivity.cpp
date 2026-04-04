#include "GalleryActivity.h"

#include <Logging.h>

#include "fontIds.h"

void GalleryActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("GALLERY", "Opening directory: %s", initialDir);
  // TODO: scan directory and build image index
  needsRedraw = true;
  requestUpdate();
}

void GalleryActivity::onExit() {
  imageIndex.clear();
  Activity::onExit();
}

void GalleryActivity::loop() {
  // TODO: handle button navigation
}

void GalleryActivity::render(RenderLock&& lock) {
  if (!needsRedraw) return;
  needsRedraw = false;

  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 220, "Gallery");

  if (imageIndex.count() == 0) {
    renderer.drawCenteredText(UI_10_FONT_ID, 260, "No images found");
  }

  renderer.displayBuffer(GfxRenderer::FULL_REFRESH);
}

int GalleryActivity::getPageSize() const { return getColumns() * getRows(); }

int GalleryActivity::getColumns() const { return 5; }

int GalleryActivity::getRows() const { return 3; }
