#pragma once

#include <GfxRenderer.h>
#include <HalPowerManager.h>

#include <cstdint>
#include <functional>

// ── Forward declarations ──
struct Rect {
  int x;
  int y;
  int width;
  int height;
};

// ── Layout metrics ──────────────────────────────────────────────────────────

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

// ── Component data structures ───────────────────────────────────────────────

struct StatusBarData {
  const char* folderName;  // nullable
  int currentIndex;        // 1-based, 0 = hide count
  int totalCount;
  int batteryPercent;      // 0-100
  bool showClock;
};

struct ListItem {
  const char* title;
  const char* subtitle;    // nullable
  const char* value;       // nullable, shown right-aligned
  const uint8_t* icon;     // nullable, 24x24 bitmap
  bool enabled;
};

struct ListViewConfig {
  int selectedIndex;
  int scrollOffset;        // first visible item index
  bool showScrollBar;
  bool showSeparators;
};

struct GridViewConfig {
  int selectedIndex;       // global index of focused image
  int pageOffset;          // global index of first image on current page
  int totalCount;
  int cols;
  int rows;
};

enum class ThumbState : uint8_t { NotLoaded, Loading, Loaded, Failed };

struct GridThumbInfo {
  ThumbState state;
  const uint8_t* bmpData;   // BMP pixel data for Loaded state, nullptr otherwise
  int bmpWidth;
  int bmpHeight;
};

using ThumbLoader = std::function<GridThumbInfo(int globalIndex)>;

enum class DialogType : uint8_t { Confirm, Alert, Select };

struct DialogConfig {
  const char* title;
  const char* message;        // for Confirm/Alert
  const char* confirmLabel;   // default "确定"
  const char* cancelLabel;    // nullptr = single button
  // Select type
  const char* const* options;
  int optionCount;
  int currentOption;          // current value (shows checkmark)
};

enum class ToastType : uint8_t { Success, Error, Info };

struct OverlayData {
  const char* filename;
  int currentIndex;
  int totalCount;
  const char* buttonLabels[4];
};

struct EmptyStateConfig {
  const uint8_t* icon;       // 32x32, nullable
  int iconSize;
  const char* title;
  const char* subtitle;      // supports \n for line break
  bool showStatusBar;
  bool showButtonHints;
  const char* buttonLabels[4];
};

// ── AlbumTheme ──────────────────────────────────────────────────────────────

class AlbumTheme {
 public:
  static AlbumTheme& getInstance() {
    static AlbumTheme instance;
    return instance;
  }

  const AlbumMetrics& getMetrics() const { return metrics_; }

  int getContentHeight(const GfxRenderer& renderer) const {
    return renderer.getScreenHeight() - metrics_.statusBarHeight - metrics_.buttonHintsHeight;
  }

  int getContentTop() const { return metrics_.statusBarHeight; }

  int getGridCols(const GfxRenderer& renderer) const {
    return isLandscape(renderer) ? metrics_.gridCols_landscape : metrics_.gridCols_portrait;
  }

  int getGridRows(const GfxRenderer& renderer) const {
    return isLandscape(renderer) ? metrics_.gridRows_landscape : metrics_.gridRows_portrait;
  }

  int getPageSize(const GfxRenderer& renderer) const {
    return getGridCols(renderer) * getGridRows(renderer);
  }

  int getListVisibleRows(const GfxRenderer& renderer) const {
    return getContentHeight(renderer) / metrics_.listRowHeight;
  }

  // ── Drawing methods ─────────────────────────────────────────────────────

  void drawStatusBar(GfxRenderer& renderer, const StatusBarData& data) const;

  void drawButtonHints(GfxRenderer& renderer, const char* label1, const char* label2,
                       const char* label3, const char* label4) const;

  void drawProgressBar(GfxRenderer& renderer, int x, int y, int width, int height,
                       int progress) const;

  void drawListView(GfxRenderer& renderer, const Rect& rect, const ListItem items[], int itemCount,
                    const ListViewConfig& config) const;

  void drawGridView(GfxRenderer& renderer, const GridViewConfig& config,
                    ThumbLoader loader) const;

  void drawDialog(GfxRenderer& renderer, const DialogConfig& config, int focusedIndex) const;

  void drawToast(GfxRenderer& renderer, const char* message, ToastType type) const;

  void drawInfoOverlay(GfxRenderer& renderer, const OverlayData& data) const;

  void drawEmptyState(GfxRenderer& renderer, const EmptyStateConfig& config) const;

  void drawBootScreen(GfxRenderer& renderer, const char* statusText, int progress,
                      const char* version) const;

  void drawShutdownScreen(GfxRenderer& renderer) const;

 private:
  AlbumTheme() = default;
  AlbumMetrics metrics_;

  static bool isLandscape(const GfxRenderer& renderer) {
    return renderer.getScreenWidth() > renderer.getScreenHeight();
  }

  void drawBatteryIcon(const GfxRenderer& renderer, int x, int y, int percent) const;
  void drawScrollBar(const GfxRenderer& renderer, const Rect& rect, int totalItems,
                     int visibleItems, int scrollOffset) const;
};

#define THEME AlbumTheme::getInstance()
