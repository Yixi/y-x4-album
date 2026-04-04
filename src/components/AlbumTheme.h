#pragma once

#include <GfxRenderer.h>

struct AlbumMetrics {
  static constexpr int SCREEN_W_LANDSCAPE = 800;
  static constexpr int SCREEN_H_LANDSCAPE = 480;
  static constexpr int SCREEN_W_PORTRAIT = 480;
  static constexpr int SCREEN_H_PORTRAIT = 800;

  static constexpr int VIEWABLE_MARGIN_TOP = 9;
  static constexpr int VIEWABLE_MARGIN_RIGHT = 3;
  static constexpr int VIEWABLE_MARGIN_BOTTOM = 3;
  static constexpr int VIEWABLE_MARGIN_LEFT = 3;

  int statusBarHeight = 24;
  int buttonHintsHeight = 40;
  int buttonHintsCount = 4;
  int buttonWidth = 106;
  int buttonSpacing = 12;

  int contentSidePadding = 20;
  int contentTopPadding = 8;

  int listRowHeight = 50;
  int listRowWithSubHeight = 70;
  int listIconSize = 24;
  int listScrollBarWidth = 4;
  int listScrollBarOffset = 5;

  int gridCols_landscape = 5;
  int gridRows_landscape = 3;
  int gridCols_portrait = 3;
  int gridRows_portrait = 5;
  int thumbWidth = 140;
  int thumbHeight = 120;
  int thumbBorderNormal = 1;
  int thumbBorderSelected = 3;
  int gridSpacingH = 16;
  int gridSpacingV = 11;
  int gridMarginLeft = 19;
  int gridMarginTop_landscape = 44;

  int dialogMinWidth = 280;
  int dialogMaxWidth = 500;
  int dialogPaddingX = 20;
  int dialogPaddingY = 16;
  int dialogCornerRadius = 6;
  int dialogBorderWidth = 2;
  int dialogButtonHeight = 36;
  int dialogButtonSpacing = 16;

  int toastHeight = 36;
  int toastPaddingX = 16;
  int toastCornerRadius = 4;

  int progressBarHeight = 16;
  int progressBarWidth = 400;

  int batteryWidth = 16;
  int batteryHeight = 10;
  int batteryCapWidth = 2;
};

class AlbumTheme {
 public:
  static AlbumTheme& getInstance() {
    static AlbumTheme instance;
    return instance;
  }

  const AlbumMetrics& getMetrics() const { return metrics; }

  int getContentHeight(bool landscape = true) const {
    return (landscape ? AlbumMetrics::SCREEN_H_LANDSCAPE : AlbumMetrics::SCREEN_H_PORTRAIT)
           - metrics.statusBarHeight - metrics.buttonHintsHeight;
  }

 private:
  AlbumTheme() = default;
  AlbumMetrics metrics;
};

#define THEME AlbumTheme::getInstance()
