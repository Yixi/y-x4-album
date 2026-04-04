#include "GalleryActivity.h"

#include <HalPowerManager.h>
#include <I18n.h>
#include <Logging.h>

#include "ActivityManager.h"
#include "ViewerActivity.h"
#include "fontIds.h"
#include "settings/AlbumSettings.h"
#include "settings/AlbumState.h"

void GalleryActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("GALLERY", "Opening directory: %s", initialDir_);

  ThumbnailCache::init();

  if (!imageIndex_.build(initialDir_, 200)) {
    LOG_ERR("GALLERY", "Failed to build image index");
  }

  imageIndex_.sort(static_cast<SortMode>(SETTINGS.data.sortMode));

  // Restore last position
  if (APP_STATE.data.lastImage[0] != '\0') {
    int idx = imageIndex_.findByFilename(APP_STATE.data.lastImage);
    if (idx >= 0) {
      focusIndex_ = idx;
      pageOffset_ = (focusIndex_ / getPageSize()) * getPageSize();
    }
  }

  // Pre-generate thumbnails with progress bar
  if (imageIndex_.count() > 0) {
    preGenerateThumbnails();
  }

  needsFullRedraw_ = true;
  lastRenderedPage_ = -1;
  requestUpdate();
}

void GalleryActivity::onExit() {
  if (imageIndex_.count() > 0 && focusIndex_ < imageIndex_.count()) {
    strncpy(APP_STATE.data.lastImage, imageIndex_.at(focusIndex_).filename,
            sizeof(APP_STATE.data.lastImage) - 1);
    strncpy(APP_STATE.data.lastDir, initialDir_, sizeof(APP_STATE.data.lastDir) - 1);
    APP_STATE.data.lastGalleryPage = static_cast<uint16_t>(pageOffset_ / getPageSize());
    APP_STATE.data.lastGalleryIndex = static_cast<uint8_t>(focusIndex_ - pageOffset_);
  }

  imageIndex_.clear();
  Activity::onExit();
}

void GalleryActivity::preGenerateThumbnails() {
  int total = imageIndex_.count();
  int generated = 0;
  int alreadyCached = 0;

  // Show initial progress
  renderer.clearScreen(0xFF);
  char msg[64];
  snprintf(msg, sizeof(msg), "%s (0/%d)", tr(STR_LOADING), total);
  renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2 - 20, msg);
  THEME.drawProgressBar(renderer, (renderer.getScreenWidth() - 400) / 2,
                        renderer.getScreenHeight() / 2 + 10, 400, 16, 0);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);

  for (int i = 0; i < total; i++) {
    const ImageEntry& entry = imageIndex_.at(i);
    char fullPath[256];
    imageIndex_.getFullPath(i, fullPath, sizeof(fullPath));

    if (ThumbnailCache::exists(fullPath, entry.fileSize, entry.modTime)) {
      alreadyCached++;
      continue;
    }

    // Generate this thumbnail
    ThumbnailCache::generate(fullPath, entry.fileSize, entry.modTime);
    generated++;

    // Update progress every 3 thumbnails or at end
    if (generated % 3 == 0 || i == total - 1) {
      int progress = ((alreadyCached + generated) * 100) / total;
      renderer.clearScreen(0xFF);
      snprintf(msg, sizeof(msg), "%s (%d/%d)", tr(STR_LOADING), alreadyCached + generated, total);
      renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2 - 20, msg);
      THEME.drawProgressBar(renderer, (renderer.getScreenWidth() - 400) / 2,
                            renderer.getScreenHeight() / 2 + 10, 400, 16, progress);
      renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    }

    // Yield to prevent watchdog timeout
    vTaskDelay(1);
  }

  LOG_INF("GALLERY", "Thumbnails: %d generated, %d cached, %d total",
          generated, alreadyCached, total);
}

void GalleryActivity::loop() {
  if (imageIndex_.count() == 0) {
    nav_.onPress({MappedInputManager::Button::Back}, [this]() { openFileBrowser(); });
    nav_.onPress({MappedInputManager::Button::Confirm}, [this]() { openFileBrowser(); });
    return;
  }

  nav_.onPressAndContinuous({MappedInputManager::Button::Right}, [this]() { moveFocus(1); });
  nav_.onPressAndContinuous({MappedInputManager::Button::Left}, [this]() { moveFocus(-1); });
  nav_.onPressAndContinuous({MappedInputManager::Button::Down}, [this]() { moveRow(1); });
  nav_.onPressAndContinuous({MappedInputManager::Button::Up}, [this]() { moveRow(-1); });

  nav_.onPress({MappedInputManager::Button::PageForward}, [this]() {
    goToPage(pageOffset_ + getPageSize());
  });
  nav_.onPress({MappedInputManager::Button::PageBack}, [this]() {
    goToPage(pageOffset_ - getPageSize());
  });

  nav_.onPress({MappedInputManager::Button::Confirm}, [this]() { openSelectedImage(); });
  nav_.onPress({MappedInputManager::Button::Back}, [this]() { openFileBrowser(); });
}

void GalleryActivity::render(RenderLock&&) {
  if (!needsFullRedraw_ && !needsFocusUpdate_) return;

  int currentPage = pageOffset_ / getPageSize();
  bool pageChanged = (currentPage != lastRenderedPage_);

  if (needsFullRedraw_ || pageChanged) {
    // Full grid redraw (page changed or first render)
    renderFullGrid();
    lastRenderedPage_ = currentPage;
    needsFullRedraw_ = false;
    needsFocusUpdate_ = false;
    renderer.displayBuffer(pageChanged && lastRenderedPage_ >= 0
                               ? HalDisplay::FAST_REFRESH
                               : HalDisplay::FULL_REFRESH);
  } else if (needsFocusUpdate_) {
    // Only focus changed within same page — just redraw borders
    renderFocusOnly();
    needsFocusUpdate_ = false;
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  }
}

void GalleryActivity::renderFullGrid() {
  renderer.clearScreen(0xFF);

  if (imageIndex_.count() == 0) {
    EmptyStateConfig cfg = {};
    cfg.title = tr(STR_NO_IMAGES);
    cfg.subtitle = tr(STR_NO_IMAGES_SUBTITLE);
    cfg.showStatusBar = true;
    cfg.showButtonHints = true;
    cfg.buttonLabels[0] = tr(STR_BROWSE);
    THEME.drawEmptyState(renderer, cfg);
    return;
  }

  // Status bar
  StatusBarData status = {};
  status.folderName = initialDir_;
  status.currentIndex = focusIndex_ + 1;
  status.totalCount = imageIndex_.count();
  status.batteryPercent = powerManager.getBatteryPercentage();
  THEME.drawStatusBar(renderer, status);

  // Grid view — thumbnails are already pre-generated, just read from cache
  GridViewConfig gridCfg = {};
  gridCfg.selectedIndex = focusIndex_;
  gridCfg.pageOffset = pageOffset_;
  gridCfg.totalCount = imageIndex_.count();
  gridCfg.cols = getCols();
  gridCfg.rows = getRows();

  THEME.drawGridView(renderer, gridCfg, [this](int globalIndex) -> GridThumbInfo {
    return loadThumb(globalIndex);
  });

  // Button hints
  THEME.drawButtonHints(renderer, tr(STR_BROWSE), tr(STR_OPEN), nullptr, nullptr);
}

void GalleryActivity::renderFocusOnly() {
  // Lightweight update: redraw status bar (for index) and grid borders only
  // Status bar update
  StatusBarData status = {};
  status.folderName = initialDir_;
  status.currentIndex = focusIndex_ + 1;
  status.totalCount = imageIndex_.count();
  status.batteryPercent = powerManager.getBatteryPercentage();

  // Clear just the status bar area and redraw
  renderer.fillRect(0, 0, renderer.getScreenWidth(), THEME.getMetrics().statusBarHeight, false);
  THEME.drawStatusBar(renderer, status);

  // Redraw grid borders only (erase old selection, draw new selection)
  int cols = getCols();
  int rows = getRows();
  int tw = THEME.getMetrics().thumbWidth;
  int th = THEME.getMetrics().thumbHeight;
  int spacingH = THEME.getMetrics().gridSpacingH;
  int spacingV = THEME.getMetrics().gridSpacingV;
  int marginLeft = THEME.getMetrics().gridMarginLeft;
  int marginTop = THEME.getMetrics().gridMarginTop_landscape;

  int pageSize = cols * rows;
  int thinBorder = THEME.getMetrics().thumbBorderNormal;
  int thickBorder = THEME.getMetrics().thumbBorderSelected;

  for (int i = 0; i < pageSize; i++) {
    int globalIdx = pageOffset_ + i;
    if (globalIdx >= imageIndex_.count()) break;

    int col = i % cols;
    int row = i / cols;
    int cellX = marginLeft + col * (tw + spacingH);
    int cellY = marginTop + row * (th + spacingV);
    bool selected = (globalIdx == focusIndex_);
    bool wasSelected = (globalIdx == prevFocusIndex_);

    if (!selected && !wasSelected) continue;  // Skip unchanged cells

    // Clear the border area (white)
    int clearW = thickBorder;
    for (int b = 0; b < clearW; b++) {
      renderer.drawRect(cellX + b, cellY + b, tw - 2 * b, th - 2 * b, false);
    }

    // Redraw with correct border width
    int borderW = selected ? thickBorder : thinBorder;
    for (int b = 0; b < borderW; b++) {
      renderer.drawRect(cellX + b, cellY + b, tw - 2 * b, th - 2 * b, true);
    }
  }
}

// ── Navigation ──────────────────────────────────────────────────────────────

int GalleryActivity::getPageSize() const {
  int ps = getCols() * getRows();
  return ps > 0 ? ps : 1;
}

int GalleryActivity::getCols() const { return THEME.getGridCols(renderer); }
int GalleryActivity::getRows() const { return THEME.getGridRows(renderer); }

void GalleryActivity::moveFocus(int delta) {
  int total = imageIndex_.count();
  if (total == 0) return;

  int newIndex = focusIndex_ + delta;

  if (SETTINGS.data.loopBrowsing) {
    newIndex = ((newIndex % total) + total) % total;
  } else {
    if (newIndex < 0) newIndex = 0;
    if (newIndex >= total) newIndex = total - 1;
  }

  if (newIndex == focusIndex_) return;

  prevFocusIndex_ = focusIndex_;
  focusIndex_ = newIndex;

  // Check if page changed
  int pageSize = getPageSize();
  int oldPage = prevFocusIndex_ / pageSize;
  int newPage = newIndex / pageSize;

  if (newPage != oldPage) {
    pageOffset_ = newPage * pageSize;
    needsFullRedraw_ = true;
  } else {
    needsFocusUpdate_ = true;
  }

  requestUpdate();
}

void GalleryActivity::moveRow(int delta) {
  moveFocus(delta * getCols());
}

void GalleryActivity::goToPage(int newOffset) {
  int total = imageIndex_.count();
  int pageSize = getPageSize();
  if (total == 0 || pageSize == 0) return;

  int maxOffset = ((total - 1) / pageSize) * pageSize;
  if (newOffset < 0) newOffset = 0;
  if (newOffset > maxOffset) newOffset = maxOffset;
  if (newOffset == pageOffset_) return;

  prevFocusIndex_ = focusIndex_;
  pageOffset_ = newOffset;
  focusIndex_ = pageOffset_;

  needsFullRedraw_ = true;
  requestUpdate();
}

void GalleryActivity::openSelectedImage() {
  if (focusIndex_ < 0 || focusIndex_ >= imageIndex_.count()) return;

  auto viewer = std::make_unique<ViewerActivity>(
      renderer, mappedInput, imageIndex_, focusIndex_);
  startActivityForResult(std::move(viewer), [this](bool) {
    needsFullRedraw_ = true;
    requestUpdate();
  });
}

void GalleryActivity::openFileBrowser() {
  LOG_DBG("GALLERY", "File browser requested");
}

void GalleryActivity::openSettings() {
  LOG_DBG("GALLERY", "Settings requested");
}

// ── Thumbnail loading (no generation — already pre-generated) ──────────────

GridThumbInfo GalleryActivity::loadThumb(int globalIndex) {
  GridThumbInfo info = {};
  info.state = ThumbState::NotLoaded;
  info.cachePath[0] = '\0';

  if (globalIndex < 0 || globalIndex >= imageIndex_.count()) {
    return info;
  }

  const ImageEntry& entry = imageIndex_.at(globalIndex);
  char fullPath[256];
  imageIndex_.getFullPath(globalIndex, fullPath, sizeof(fullPath));

  ThumbnailCache::getCachePath(fullPath, entry.fileSize, entry.modTime,
                               info.cachePath, sizeof(info.cachePath));

  if (ThumbnailCache::exists(fullPath, entry.fileSize, entry.modTime)) {
    info.state = ThumbState::Loaded;
  } else {
    // Should not happen after preGenerate, but handle gracefully
    info.state = ThumbState::Failed;
  }

  info.bmpWidth = ThumbnailCache::THUMB_WIDTH;
  info.bmpHeight = ThumbnailCache::THUMB_HEIGHT;
  return info;
}
