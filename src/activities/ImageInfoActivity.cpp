#include "ImageInfoActivity.h"

#include <I18n.h>
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
  using Button = MappedInputManager::Button;
  if (mappedInput.wasPressed(Button::Back)) {
    finish();
    return;
  }
}

void ImageInfoActivity::render(RenderLock&& lock) {
  if (!needsRedraw) return;
  needsRedraw = false;

  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 220, tr(STR_IMAGE_INFO));
  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}
