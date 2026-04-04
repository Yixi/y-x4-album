#include "BootActivity.h"

#include <Logging.h>

#include "components/AlbumTheme.h"
#include "fontIds.h"

void BootActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();
}

void BootActivity::onExit() { Activity::onExit(); }

void BootActivity::render(RenderLock&& lock) {
  renderer.clearScreen(0xFF);
  THEME.drawBootScreen(renderer, "Initializing...", -1, ALBUM_VERSION);
  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}
