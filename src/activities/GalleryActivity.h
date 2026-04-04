#pragma once

#include "Activity.h"
#include "album/ImageIndex.h"
#include "album/ThumbnailCache.h"
#include "components/AlbumTheme.h"
#include "util/ButtonNavigator.h"

class GalleryActivity final : public Activity {
 public:
  explicit GalleryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Gallery", renderer, mappedInput) {}

  GalleryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const char* dirPath)
      : Activity("Gallery", renderer, mappedInput) {
    if (dirPath) {
      snprintf(initialDir_, sizeof(initialDir_), "%s", dirPath);
    }
  }

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  ImageIndex imageIndex_;
  char initialDir_[256] = "/";
  int focusIndex_ = 0;
  int prevFocusIndex_ = -1;
  int pageOffset_ = 0;
  int lastRenderedPage_ = -1;
  bool needsFullRedraw_ = true;
  bool needsFocusUpdate_ = false;
  ButtonNavigator nav_{200, 400};

  int getPageSize() const;
  int getCols() const;
  int getRows() const;

  void preGenerateThumbnails();
  void renderFullGrid();
  void renderFocusOnly();

  void moveFocus(int delta);
  void moveRow(int delta);
  void goToPage(int pageOffset);
  void openSelectedImage();
  void openFileBrowser();
  void openSettings();

  GridThumbInfo loadThumb(int globalIndex);
};
