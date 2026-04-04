#pragma once

#include "Activity.h"

class FileBrowserActivity final : public Activity {
 public:
  explicit FileBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("FileBrowser", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // TODO: file list, navigation state
  bool needsRedraw = true;
};
