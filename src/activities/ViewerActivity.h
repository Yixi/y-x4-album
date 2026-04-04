#pragma once

#include "Activity.h"
#include "album/ImageIndex.h"

class ViewerActivity final : public Activity {
 public:
  ViewerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                 ImageIndex& imageIndex, int startIndex)
      : Activity("Viewer", renderer, mappedInput),
        imageIndex(imageIndex), currentIndex(startIndex) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  ImageIndex& imageIndex;
  int currentIndex;
  bool overlayVisible = false;
  int partialRefreshCount = 0;
};
