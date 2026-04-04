#pragma once

#include "Activity.h"
#include "album/ImageIndex.h"

class SlideshowActivity final : public Activity {
 public:
  SlideshowActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                    ImageIndex& imageIndex, int startIndex)
      : Activity("Slideshow", renderer, mappedInput),
        imageIndex(imageIndex), currentIndex(startIndex) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  ImageIndex& imageIndex;
  int currentIndex;
  bool paused = false;
  unsigned long lastSwitchTime = 0;
};
