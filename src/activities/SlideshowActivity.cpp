#include "SlideshowActivity.h"

#include <HalPowerManager.h>
#include <I18n.h>
#include <Logging.h>

#include "components/AlbumTheme.h"
#include "fontIds.h"
#include "settings/AlbumSettings.h"

void SlideshowActivity::onEnter() {
  Activity::onEnter();

  intervalMs_ = static_cast<unsigned long>(SETTINGS.data.slideshowInterval) * 1000UL;
  if (intervalMs_ < 1000) intervalMs_ = 5000;
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
  Activity::onExit();
}

void SlideshowActivity::loop() {
  using Button = MappedInputManager::Button;

  if (paused_) {
    if (mappedInput.wasPressed(Button::Confirm)) {
      paused_ = false;
      lastSwitchTime_ = millis();
      needsDecode_ = true;
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

  // Playing: any button pauses
  if (mappedInput.wasAnyPressed()) {
    paused_ = true;
    needsPauseOverlay_ = true;
    requestUpdate();
    return;
  }

  // Auto-advance timer
  unsigned long now = millis();
  if (now - lastSwitchTime_ >= intervalMs_) {
    int nextIdx = shuffleMode_ ? nextShuffleIndex() : nextSequentialIndex();

    if (nextIdx < 0) {
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
    powerManager.setPowerSaving(true);
  }
}

void SlideshowActivity::render(RenderLock&&) {
  if (needsDecode_) {
    needsDecode_ = false;

    powerManager.setPowerSaving(false);

    int screenW = renderer.getScreenWidth();
    int screenH = renderer.getScreenHeight();

    char path[384];
    imageIndex_.getFullPath(currentIndex_, path, sizeof(path));

    LOG_INF("SLIDE", "Showing image %d/%d: %s", currentIndex_ + 1,
            imageIndex_.count(), path);

    ScaleMode mode = static_cast<ScaleMode>(SETTINGS.data.scaleMode);
    bool bgWhite = SETTINGS.data.bgWhite;

    if (SETTINGS.data.renderMode == 0) {
      if (!ImageDecoder::decodeGrayscale(path, renderer, screenW, screenH,
                                         mode, bgWhite)) {
        ImageDecoder::decodeBW(path, renderer, screenW, screenH, mode, bgWhite);
      }
    } else {
      ImageDecoder::decodeBW(path, renderer, screenW, screenH, mode, bgWhite);
    }

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
  if (next < 0) return;

  currentIndex_ = next;
  shuffleCount_++;
  needsDecode_ = true;
  needsPauseOverlay_ = true;
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
    return -1;
  }
  return next;
}

int SlideshowActivity::nextShuffleIndex() const {
  int total = imageIndex_.count();
  if (total <= 1) return loopMode_ ? 0 : -1;

  if (!loopMode_ && shuffleCount_ >= total - 1) return -1;

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

  int boxW = 200;
  int boxH = 100;
  int boxX = centerX - boxW / 2;
  int boxY = centerY - boxH / 2;

  // Black background box
  renderer.fillRect(boxX, boxY, boxW, boxH, true);

  // Pause icon: two vertical bars
  int barW = 8;
  int barH = 28;
  int barGap = 12;
  int barY = boxY + 14;
  renderer.fillRect(centerX - barGap / 2 - barW, barY, barW, barH, false);
  renderer.fillRect(centerX + barGap / 2, barY, barW, barH, false);

  // Pause text
  renderer.drawCenteredText(UI_10_FONT_ID, boxY + 50, tr(STR_PAUSE), false);

  // Interval + index info
  char info[64];
  unsigned long sec = intervalMs_ / 1000;
  if (sec < 60) {
    snprintf(info, sizeof(info), tr(STR_SEC_PER_IMAGE), sec);
  } else {
    snprintf(info, sizeof(info), tr(STR_MIN_PER_IMAGE), sec / 60);
  }
  char full[96];
  snprintf(full, sizeof(full), "%s | %d/%d", info, currentIndex_ + 1, imageIndex_.count());
  renderer.drawCenteredText(SMALL_FONT_ID, boxY + 72, full, false);

  // Bottom button hints bar
  int btnBarY = screenH - 40;
  renderer.fillRect(0, btnBarY, screenW, 40, true);

  auto labels = mappedInput.mapLabels(tr(STR_EXIT), tr(STR_RESUME), tr(STR_PREV), tr(STR_NEXT));
  const AlbumMetrics& m = THEME.getMetrics();
  int totalBtnWidth = m.buttonHintsCount * m.buttonWidth +
                      (m.buttonHintsCount - 1) * m.buttonSpacing;
  int startX = (screenW - totalBtnWidth) / 2;

  const char* btnLabels[4] = {labels.btn1, labels.btn2, labels.btn3, labels.btn4};
  for (int i = 0; i < 4; i++) {
    if (!btnLabels[i]) continue;
    int bx = startX + i * (m.buttonWidth + m.buttonSpacing);
    int by = btnBarY + (40 - m.dialogButtonHeight) / 2;
    renderer.drawRect(bx, by, m.buttonWidth, m.dialogButtonHeight, false);
    int tw = renderer.getTextWidth(UI_10_FONT_ID, btnLabels[i]);
    int tx = bx + (m.buttonWidth - tw) / 2;
    int ty = by + (m.dialogButtonHeight - 12) / 2;
    renderer.drawText(UI_10_FONT_ID, tx, ty, btnLabels[i], false);
  }
}
