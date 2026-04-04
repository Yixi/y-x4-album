#include "ViewerActivity.h"

#include <I18n.h>
#include <Logging.h>

#include "ActivityManager.h"
#include "ImageInfoActivity.h"
#include "SlideshowActivity.h"
#include "components/AlbumTheme.h"
#include "fontIds.h"
#include "settings/AlbumSettings.h"

void ViewerActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("VIEWER", "Opening image index: %d / %d", currentIndex_, imageIndex_.count());

  // Load scale mode from settings
  scaleMode_ = static_cast<ScaleMode>(SETTINGS.data.scaleMode);

  needsDecode_ = true;
  partialRefreshCount_ = 0;

  // Don't show overlay on first enter to avoid extra refreshes.
  // User can press Confirm to show it.
  overlayVisible_ = false;
  overlayAutoHide_ = false;

  requestUpdate();
}

void ViewerActivity::onExit() { Activity::onExit(); }

void ViewerActivity::loop() {
  using Button = MappedInputManager::Button;

  // ── Auto-hide overlay ──
  if (overlayVisible_ && overlayAutoHide_) {
    if (millis() - overlayShowTime_ >= OVERLAY_AUTO_HIDE_MS) {
      hideOverlay();
      return;
    }
  }

  // ── Long press detection ──
  if (mappedInput.isPressed(Button::Confirm) &&
      mappedInput.getHeldTime() > LONG_PRESS_MS) {
    startSlideshow();
    return;
  }
  if (mappedInput.isPressed(Button::Up) &&
      mappedInput.getHeldTime() > LONG_PRESS_MS) {
    navigateTo(0);
    return;
  }
  if (mappedInput.isPressed(Button::Down) &&
      mappedInput.getHeldTime() > LONG_PRESS_MS) {
    navigateTo(imageIndex_.count() - 1);
    return;
  }

  // ── Overlay visible: different button mapping ──
  if (overlayVisible_ && !overlayAutoHide_) {
    if (mappedInput.wasPressed(Button::Confirm)) {
      openImageInfo();
      return;
    }
    if (mappedInput.wasPressed(Button::Left)) {
      hideOverlay();
      return;
    }
    if (mappedInput.wasPressed(Button::Right)) {
      startSlideshow();
      return;
    }
    if (mappedInput.wasPressed(Button::Back)) {
      hideOverlay();
      return;
    }
    if (mappedInput.wasPressed(Button::Up)) {
      navigateRelative(-1);
      return;
    }
    if (mappedInput.wasPressed(Button::Down)) {
      navigateRelative(1);
      return;
    }
    return;
  }

  // ── Normal mode (overlay hidden or auto-hiding) ──
  if (mappedInput.wasPressed(Button::Confirm)) {
    showOverlay(false);
    return;
  }
  if (mappedInput.wasPressed(Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasPressed(Button::Up) || mappedInput.wasPressed(Button::Left)) {
    navigateRelative(-1);
    return;
  }
  if (mappedInput.wasPressed(Button::Down) || mappedInput.wasPressed(Button::Right)) {
    navigateRelative(1);
    return;
  }
}

void ViewerActivity::render(RenderLock&&) {
  if (needsDecode_) {
    needsDecode_ = false;

    int screenW = renderer.getScreenWidth();
    int screenH = renderer.getScreenHeight();

    char path[384];
    imageIndex_.getFullPath(currentIndex_, path, sizeof(path));

    LOG_INF("VIEWER", "Decoding: %s (%s)", path,
            scaleMode_ == ScaleMode::Fit ? "Fit" : "Fill");

    bool bgWhite = SETTINGS.data.bgWhite;

    if (SETTINGS.data.renderMode == 0) {
      if (!ImageDecoder::decodeGrayscale(path, renderer, screenW, screenH,
                                         scaleMode_, bgWhite)) {
        LOG_ERR("VIEWER", "Grayscale decode failed, falling back to BW");
        if (!ImageDecoder::decodeBW(path, renderer, screenW, screenH,
                                    scaleMode_, bgWhite)) {
          renderer.clearScreen(0xFF);
          renderer.drawCenteredText(UI_12_FONT_ID, screenH / 2 - 10,
                                    tr(STR_DECODE_FAILED));
          renderer.displayBuffer(HalDisplay::FULL_REFRESH);
        }
      }
    } else {
      if (!ImageDecoder::decodeBW(path, renderer, screenW, screenH,
                                  scaleMode_, bgWhite)) {
        renderer.clearScreen(0xFF);
        renderer.drawCenteredText(UI_12_FONT_ID, screenH / 2 - 10,
                                  tr(STR_DECODE_FAILED));
        renderer.displayBuffer(HalDisplay::FULL_REFRESH);
      }
    }

    partialRefreshCount_ = 0;

    if (overlayVisible_) {
      needsOverlayRedraw_ = true;
    }
  }

  if (needsOverlayRedraw_) {
    needsOverlayRedraw_ = false;

    const char* filename = imageIndex_.at(currentIndex_).filename;

    auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_INFO),
                                        tr(STR_ROTATE), tr(STR_START_SLIDESHOW));

    OverlayData overlay;
    overlay.filename = filename;
    overlay.currentIndex = currentIndex_ + 1;
    overlay.totalCount = imageIndex_.count();
    overlay.buttonLabels[0] = labels.btn1;
    overlay.buttonLabels[1] = labels.btn2;
    overlay.buttonLabels[2] = labels.btn3;
    overlay.buttonLabels[3] = labels.btn4;

    THEME.drawInfoOverlay(renderer, overlay);

    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    partialRefreshCount_++;
  }
}

// ── Navigation ──

void ViewerActivity::navigateTo(int newIndex) {
  if (newIndex < 0) newIndex = 0;
  if (newIndex >= imageIndex_.count()) newIndex = imageIndex_.count() - 1;
  if (newIndex == currentIndex_) return;

  LOG_DBG("VIEWER", "Navigate: %d -> %d", currentIndex_, newIndex);
  currentIndex_ = newIndex;
  needsDecode_ = true;

  // Don't show overlay during navigation to avoid extra refreshes
  overlayVisible_ = false;
  overlayAutoHide_ = false;

  requestUpdate();
}

void ViewerActivity::navigateRelative(int delta) {
  int newIndex = currentIndex_ + delta;

  if (SETTINGS.data.loopBrowsing) {
    if (newIndex < 0) newIndex = imageIndex_.count() - 1;
    if (newIndex >= imageIndex_.count()) newIndex = 0;
  }

  navigateTo(newIndex);
}

// ── Overlay control ──

void ViewerActivity::showOverlay(bool autoHide) {
  overlayVisible_ = true;
  overlayAutoHide_ = autoHide;
  overlayShowTime_ = millis();
  needsOverlayRedraw_ = true;
  requestUpdate();
}

void ViewerActivity::hideOverlay() {
  overlayVisible_ = false;
  overlayAutoHide_ = false;

  // Re-decode to remove overlay from screen
  // TODO: optimize by caching the rendered image instead of re-decoding
  needsDecode_ = true;
  requestUpdate();
}

void ViewerActivity::toggleScaleMode() {
  scaleMode_ = (scaleMode_ == ScaleMode::Fit) ? ScaleMode::Fill : ScaleMode::Fit;
  needsDecode_ = true;
  requestUpdate();
}

void ViewerActivity::openImageInfo() {
  char path[384];
  imageIndex_.getFullPath(currentIndex_, path, sizeof(path));

  auto infoActivity = std::make_unique<ImageInfoActivity>(
      renderer, mappedInput, path);

  startActivityForResult(std::move(infoActivity), [this](bool /*cancelled*/) {
    needsDecode_ = true;
    requestUpdate();
  });
}

void ViewerActivity::startSlideshow() {
  auto slideshow = std::make_unique<SlideshowActivity>(
      renderer, mappedInput, imageIndex_, currentIndex_);

  startActivityForResult(std::move(slideshow), [this](bool /*cancelled*/) {
    needsDecode_ = true;
    requestUpdate();
  });
}
