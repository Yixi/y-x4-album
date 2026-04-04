#pragma once

#include <HalStorage.h>

#include "Activity.h"
#include "album/ImageScanner.h"
#include "components/AlbumTheme.h"
#include "util/ButtonNavigator.h"

struct BrowserEntry {
  char name[128];
  bool isDir;
  uint32_t fileSize;
};

class FileBrowserActivity final : public Activity {
 public:
  explicit FileBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("FileBrowser", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int MAX_ENTRIES = 200;

  char currentDir_[256] = "/";
  BrowserEntry* entries_ = nullptr;
  int entryCount_ = 0;
  int selectedIndex_ = 0;
  int scrollOffset_ = 0;
  bool needsRedraw_ = true;
  ButtonNavigator nav_{200, 400};

  bool scanCurrentDir();
  void freeEntries();
  void navigateUp();
  void enterSelected();
  void openGallery(const char* dirPath);
  int getVisibleRows() const;
};
