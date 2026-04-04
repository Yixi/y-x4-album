#pragma once

#include "Activity.h"
#include "album/ImageDecoder.h"
#include "album/ImageIndex.h"

class ViewerActivity final : public Activity {
 public:
  ViewerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                 ImageIndex& imageIndex, int startIndex)
      : Activity("Viewer", renderer, mappedInput),
        imageIndex_(imageIndex), currentIndex_(startIndex) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // ── State ──
  ImageIndex& imageIndex_;
  int currentIndex_;

  bool overlayVisible_ = false;
  bool overlayAutoHide_ = false;
  unsigned long overlayShowTime_ = 0;
  static constexpr unsigned long OVERLAY_AUTO_HIDE_MS = 3000;

  ScaleMode scaleMode_ = ScaleMode::Fit;
  bool needsDecode_ = true;
  bool needsOverlayRedraw_ = false;
  int partialRefreshCount_ = 0;
  static constexpr int MAX_PARTIAL_BEFORE_FULL = 5;

  static constexpr unsigned long LONG_PRESS_MS = 800;

  // ── Helpers ──
  void navigateTo(int newIndex);
  void navigateRelative(int delta);
  void showOverlay(bool autoHide);
  void hideOverlay();
  void toggleScaleMode();
  void openImageInfo();
  void startSlideshow();
};
