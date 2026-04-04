#pragma once

#include "Activity.h"
#include "album/ImageIndex.h"

class GalleryActivity final : public Activity {
 public:
  explicit GalleryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Gallery", renderer, mappedInput) {}

  GalleryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const char* dirPath)
      : Activity("Gallery", renderer, mappedInput) {
    if (dirPath) {
      snprintf(initialDir, sizeof(initialDir), "%s", dirPath);
    }
  }

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  ImageIndex imageIndex;
  char initialDir[256] = "/";
  int focusIndex = 0;
  int currentPage = 0;
  bool needsRedraw = true;

  int getPageSize() const;
  int getColumns() const;
  int getRows() const;
};
