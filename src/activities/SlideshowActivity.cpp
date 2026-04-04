#include "SlideshowActivity.h"

#include <HalPowerManager.h>
#include <Logging.h>

#include "components/AlbumTheme.h"
#include "fontIds.h"
#include "settings/AlbumSettings.h"

void SlideshowActivity::onEnter() {
  Activity::onEnter();

  // Load settings
  intervalMs_ = static_cast<unsigned long>(SETTINGS.data.slideshowInterval) * 1000UL;
  if (intervalMs_ < 1000) intervalMs_ = 5000;  // sanity default
  shuffleMode_ = (SETTINGS.data.slideshowOrder != 0);
  loopMode_ = (SETTINGS.data.slideshowLoop != 0);

  paused_ = false;
  needsDecode_ = true;
  shuffleCount_ = 0;
  lastSwitchTime_ = millis();

  LOG_INF("SLIDE", "Start: index=%d, interval=%lums, shuffle=%d, loop=%d",
          currentIndex_, intervalMs_, shuffleMode_, loopMode_);

  requestUpdate();
}

void SlideshowActivity::onExit() {
  // Ensure power saving is restored (it's managed per-loop, but be safe)
  Activity::onExit();
}

void SlideshowActivity::loop() {
  using Button = MappedInputManager::Button;

  if (paused_) {
    // ── Paused: specific button mapping ──
    if (mappedInput.wasPressed(Button::Confirm)) {
      paused_ = false;
      lastSwitchTime_ = millis();
      needsDecode_ = true;  // redraw without pause overlay
      requestUpdate();
      return;
    }
    if (mappedInput.wasPressed(Button::Back)) {
      finish();
      return;
    }
    if (mappedInput.wasPressed(Button::Left) || mappedInput.wasPressed(Button::Up)) {
      goToPrevious();
      return;
    }
    if (mappedInput.wasPressed(Button::Right) || mappedInput.wasPressed(Button::Down)) {
      advanceToNext();
      return;
    }
    return;
  }

  // ── Playing: any button pauses ──
  if (mappedInput.wasAnyPressed()) {
    paused_ = true;
    needsPauseOverlay_ = true;
    requestUpdate();
    return;
  }

  // ── Auto-advance timer ──
  unsigned long now = millis();
  if (now - lastSwitchTime_ >= intervalMs_) {
    // Enable power saving during decode is handled by HalPowerManager Lock in decoder
    int nextIdx = shuffleMode_ ? nextShuffleIndex() : nextSequentialIndex();

    if (nextIdx < 0) {
      // End of sequence (non-loop mode)
      paused_ = true;
      needsPauseOverlay_ = true;
      requestUpdate();
      return;
    }

    currentIndex_ = nextIdx;
    shuffleCount_++;
    needsDecode_ = true;
    lastSwitchTime_ = now;
    requestUpdate();
  } else {
    // During wait interval, enable power saving
    powerManager.setPowerSaving(true);
  }
}

void SlideshowActivity::render(RenderLock&&) {
  if (needsDecode_) {
    needsDecode_ = false;

    // Disable power saving for decode
    powerManager.setPowerSaving(false);

    int screenW = renderer.getScreenWidth();
    int screenH = renderer.getScreenHeight();

    char path[384];
    imageIndex_.getFullPath(currentIndex_, path, sizeof(path));

    LOG_INF("SLIDE", "Showing image %d/%d: %s", currentIndex_ + 1,
            imageIndex_.count(), path);

    ScaleMode mode = static_cast<ScaleMode>(SETTINGS.data.scaleMode);
    bool bgWhite = SETTINGS.data.bgWhite;

    // Prefer grayscale for best quality
    if (SETTINGS.data.renderMode == 0) {
      if (!ImageDecoder::decodeGrayscale(path, renderer, screenW, screenH,
                                         mode, bgWhite)) {
        ImageDecoder::decodeBW(path, renderer, screenW, screenH, mode, bgWhite);
      }
    } else {
      ImageDecoder::decodeBW(path, renderer, screenW, screenH, mode, bgWhite);
    }

    // Re-enable power saving after decode for wait period
    if (!paused_) {
      powerManager.setPowerSaving(true);
    }
  }

  if (needsPauseOverlay_) {
    needsPauseOverlay_ = false;
    drawPauseOverlay();
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  }
}

// ── Navigation helpers ──

void SlideshowActivity::advanceToNext() {
  int next = shuffleMode_ ? nextShuffleIndex() : nextSequentialIndex();
  if (next < 0) return;  // end of non-loop sequence

  currentIndex_ = next;
  shuffleCount_++;
  needsDecode_ = true;
  needsPauseOverlay_ = true;  // re-draw pause overlay after image
  lastSwitchTime_ = millis();
  requestUpdate();
}

void SlideshowActivity::goToPrevious() {
  int prev = currentIndex_ - 1;
  if (prev < 0) {
    prev = loopMode_ ? imageIndex_.count() - 1 : 0;
  }
  if (prev == currentIndex_) return;

  currentIndex_ = prev;
  needsDecode_ = true;
  needsPauseOverlay_ = true;
  lastSwitchTime_ = millis();
  requestUpdate();
}

int SlideshowActivity::nextSequentialIndex() const {
  int next = currentIndex_ + 1;
  if (next >= imageIndex_.count()) {
    if (loopMode_) return 0;
    return -1;  // end
  }
  return next;
}

int SlideshowActivity::nextShuffleIndex() const {
  int total = imageIndex_.count();
  if (total <= 1) return loopMode_ ? 0 : -1;

  // If we've shown all images in this cycle
  if (!loopMode_ && shuffleCount_ >= total - 1) return -1;

  // Simple pseudo-random: use millis() as entropy, avoid current index
  int next = currentIndex_;
  unsigned long seed = millis();
  while (next == currentIndex_) {
    seed = seed * 1103515245UL + 12345UL;
    next = static_cast<int>((seed >> 16) % static_cast<unsigned long>(total));
  }
  return next;
}

// ── Pause overlay drawing ──

void SlideshowActivity::drawPauseOverlay() {
  int screenW = renderer.getScreenWidth();
  int screenH = renderer.getScreenHeight();
  int centerX = screenW / 2;
  int centerY = screenH / 2;

  // Draw semi-dark center area (pause icon + info)
  int boxW = 200;
  int boxH = 100;
  int boxX = centerX - boxW / 2;
  int boxY = centerY - boxH / 2;

  // Black background box
  renderer.fillRect(boxX, boxY, boxW, boxH, true);

  // Pause icon: two vertical bars (white on black)
  int barW = 8;
  int barH = 28;
  int barGap = 12;
  int barY = boxY + 14;
  renderer.fillRect(centerX - barGap / 2 - barW, barY, barW, barH, false);
  renderer.fillRect(centerX + barGap / 2, barY, barW, barH, false);

  // "暂停" text
  renderer.drawCenteredText(UI_10_FONT_ID, boxY + 50, "暂停", false);

  // Interval + index info
  char info[64];
  snprintf(info, sizeof(info), "%s | %d/%d", intervalLabel(),
           currentIndex_ + 1, imageIndex_.count());
  renderer.drawCenteredText(SMALL_FONT_ID, boxY + 72, info, false);

  // Bottom button hints bar (black background, white text)
  int btnBarY = screenH - 40;
  renderer.fillRect(0, btnBarY, screenW, 40, true);

  auto labels = mappedInput.mapLabels("退出", "继续", "上一张", "下一张");
  const AlbumMetrics& m = THEME.getMetrics();
  int totalBtnWidth = m.buttonHintsCount * m.buttonWidth +
                      (m.buttonHintsCount - 1) * m.buttonSpacing;
  int startX = (screenW - totalBtnWidth) / 2;

  const char* btnLabels[4] = {labels.btn1, labels.btn2, labels.btn3, labels.btn4};
  for (int i = 0; i < 4; i++) {
    if (!btnLabels[i]) continue;
    int bx = startX + i * (m.buttonWidth + m.buttonSpacing);
    int by = btnBarY + (40 - m.dialogButtonHeight) / 2;
    // White border button on black background
    renderer.drawRect(bx, by, m.buttonWidth, m.dialogButtonHeight, false);
    int tw = renderer.getTextWidth(UI_10_FONT_ID, btnLabels[i]);
    int tx = bx + (m.buttonWidth - tw) / 2;
    int ty = by + (m.dialogButtonHeight - 12) / 2;
    renderer.drawText(UI_10_FONT_ID, tx, ty, btnLabels[i], false);
  }
}

const char* SlideshowActivity::intervalLabel() const {
  unsigned long sec = intervalMs_ / 1000;
  if (sec < 60) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%lus/张", sec);
    return buf;
  }
  static char buf[16];
  snprintf(buf, sizeof(buf), "%lumin/张", sec / 60);
  return buf;
}
