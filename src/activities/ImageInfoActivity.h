#pragma once

#include "Activity.h"
#include "album/ImageIndex.h"

class ImageInfoActivity final : public Activity {
 public:
  ImageInfoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                    const char* imagePath)
      : Activity("ImageInfo", renderer, mappedInput) {
    if (imagePath) {
      snprintf(path, sizeof(path), "%s", imagePath);
    }
  }

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  char path[384] = {};
  bool needsRedraw = true;
};
