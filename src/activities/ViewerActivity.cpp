#include "ViewerActivity.h"

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

  // Show overlay on first enter, auto-hide after 3s
  overlayVisible_ = true;
  overlayAutoHide_ = true;
  overlayShowTime_ = millis();

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
      // Rotate display — not implemented yet, just hide overlay
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
    // Side buttons still navigate in overlay mode
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

  // Navigation: Up/Left = previous, Down/Right = next
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

    // Build full path
    char path[384];
    imageIndex_.getFullPath(currentIndex_, path, sizeof(path));

    LOG_INF("VIEWER", "Decoding: %s (%s)", path,
            scaleMode_ == ScaleMode::Fit ? "Fit" : "Fill");

    bool bgWhite = SETTINGS.data.bgWhite;

    // Use grayscale rendering for best quality
    if (SETTINGS.data.renderMode == 0) {
      // Grayscale (4-level)
      if (!ImageDecoder::decodeGrayscale(path, renderer, screenW, screenH,
                                         scaleMode_, bgWhite)) {
        LOG_ERR("VIEWER", "Grayscale decode failed, falling back to BW");
        if (!ImageDecoder::decodeBW(path, renderer, screenW, screenH,
                                    scaleMode_, bgWhite)) {
          // Show error on screen
          renderer.clearScreen(0xFF);
          renderer.drawCenteredText(UI_12_FONT_ID, screenH / 2 - 10,
                                    "Failed to decode image");
          renderer.displayBuffer(HalDisplay::FULL_REFRESH);
        }
      }
    } else {
      // BW (1-bit)
      if (!ImageDecoder::decodeBW(path, renderer, screenW, screenH,
                                  scaleMode_, bgWhite)) {
        renderer.clearScreen(0xFF);
        renderer.drawCenteredText(UI_12_FONT_ID, screenH / 2 - 10,
                                  "Failed to decode image");
        renderer.displayBuffer(HalDisplay::FULL_REFRESH);
      }
    }

    partialRefreshCount_ = 0;

    // Draw overlay on top if visible (first entry)
    if (overlayVisible_) {
      needsOverlayRedraw_ = true;
    }
  }

  if (needsOverlayRedraw_) {
    needsOverlayRedraw_ = false;

    char path[384];
    imageIndex_.getFullPath(currentIndex_, path, sizeof(path));
    const char* filename = imageIndex_.at(currentIndex_).filename;

    // Determine button labels based on overlay state
    auto labels = mappedInput.mapLabels("返回", "信息", "旋转", "幻灯片");

    OverlayData overlay;
    overlay.filename = filename;
    overlay.currentIndex = currentIndex_ + 1;
    overlay.totalCount = imageIndex_.count();
    overlay.buttonLabels[0] = labels.btn1;
    overlay.buttonLabels[1] = labels.btn2;
    overlay.buttonLabels[2] = labels.btn3;
    overlay.buttonLabels[3] = labels.btn4;

    THEME.drawInfoOverlay(renderer, overlay);

    // Overlay is drawn on top of image, use fast refresh
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

  // Re-show overlay with auto-hide when navigating
  overlayVisible_ = true;
  overlayAutoHide_ = true;
  overlayShowTime_ = millis();

  requestUpdate();
}

void ViewerActivity::navigateRelative(int delta) {
  int newIndex = currentIndex_ + delta;

  // Wrap around if loop browsing enabled
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

  // Need to re-decode to remove overlay from screen
  // Only do full refresh periodically
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
    // Re-render current image when returning from info
    needsDecode_ = true;
    requestUpdate();
  });
}

void ViewerActivity::startSlideshow() {
  auto slideshow = std::make_unique<SlideshowActivity>(
      renderer, mappedInput, imageIndex_, currentIndex_);

  startActivityForResult(std::move(slideshow), [this](bool /*cancelled*/) {
    // Re-render current image when returning from slideshow
    needsDecode_ = true;
    requestUpdate();
  });
}
