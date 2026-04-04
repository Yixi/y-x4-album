#pragma once

#include "Activity.h"
#include "album/ImageDecoder.h"
#include "album/ImageIndex.h"

class SlideshowActivity final : public Activity {
 public:
  SlideshowActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                    ImageIndex& imageIndex, int startIndex)
      : Activity("Slideshow", renderer, mappedInput),
        imageIndex_(imageIndex), currentIndex_(startIndex) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  ImageIndex& imageIndex_;
  int currentIndex_;

  bool paused_ = false;
  bool needsDecode_ = true;
  bool needsPauseOverlay_ = false;
  unsigned long lastSwitchTime_ = 0;
  unsigned long intervalMs_ = 5000;

  bool shuffleMode_ = false;
  bool loopMode_ = true;

  // Shuffle state: simple LCG-based pseudo-random sequence
  int shuffleCount_ = 0;  // how many images shown in current cycle

  // ── Helpers ──
  void advanceToNext();
  void goToPrevious();
  int nextSequentialIndex() const;
  int nextShuffleIndex() const;
  void drawPauseOverlay();
  const char* intervalLabel() const;
};
