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

  // Sort by configured mode
  imageIndex_.sort(static_cast<SortMode>(SETTINGS.data.sortMode));

  // Restore last position if available
  if (APP_STATE.data.lastImage[0] != '\0') {
    int idx = imageIndex_.findByFilename(APP_STATE.data.lastImage);
    if (idx >= 0) {
      focusIndex_ = idx;
      pageOffset_ = (focusIndex_ / getPageSize()) * getPageSize();
    }
  }

  needsRedraw_ = true;
  requestUpdate();
}

void GalleryActivity::onExit() {
  // Save current position
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

void GalleryActivity::loop() {
  if (imageIndex_.count() == 0) {
    // Empty state — only handle Back to open file browser
    nav_.onPress({MappedInputManager::Button::Back}, [this]() { openFileBrowser(); });
    nav_.onPress({MappedInputManager::Button::Confirm}, [this]() { openFileBrowser(); });
    return;
  }

  // Grid navigation
  nav_.onPressAndContinuous({MappedInputManager::Button::Right}, [this]() { moveFocus(1); });
  nav_.onPressAndContinuous({MappedInputManager::Button::Left}, [this]() { moveFocus(-1); });
  nav_.onPressAndContinuous({MappedInputManager::Button::Down}, [this]() { moveRow(1); });
  nav_.onPressAndContinuous({MappedInputManager::Button::Up}, [this]() { moveRow(-1); });

  // Page navigation
  nav_.onPress({MappedInputManager::Button::PageForward}, [this]() {
    goToPage(pageOffset_ + getPageSize());
  });
  nav_.onPress({MappedInputManager::Button::PageBack}, [this]() {
    goToPage(pageOffset_ - getPageSize());
  });

  // Actions
  nav_.onPress({MappedInputManager::Button::Confirm}, [this]() { openSelectedImage(); });
  nav_.onPress({MappedInputManager::Button::Back}, [this]() { openFileBrowser(); });
}

void GalleryActivity::render(RenderLock&& lock) {
  if (!needsRedraw_) return;
  needsRedraw_ = false;

  renderer.clearScreen(0xFF);

  if (imageIndex_.count() == 0) {
    // Empty state
    EmptyStateConfig cfg = {};
    cfg.title = tr(STR_NO_IMAGES);
    cfg.subtitle = tr(STR_NO_IMAGES_SUBTITLE);
    cfg.showStatusBar = true;
    cfg.showButtonHints = true;
    cfg.buttonLabels[0] = tr(STR_BROWSE);
    cfg.buttonLabels[1] = nullptr;
    cfg.buttonLabels[2] = nullptr;
    cfg.buttonLabels[3] = nullptr;
    THEME.drawEmptyState(renderer, cfg);
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
    return;
  }

  // Status bar
  StatusBarData status = {};
  status.folderName = initialDir_;
  status.currentIndex = focusIndex_ + 1;
  status.totalCount = imageIndex_.count();
  status.batteryPercent = powerManager.getBatteryPercentage();
  THEME.drawStatusBar(renderer, status);

  // Grid view
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

  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}

// ── Navigation ──────────────────────────────────────────────────────────────

int GalleryActivity::getPageSize() const {
  int ps = getCols() * getRows();
  return ps > 0 ? ps : 1;  // Prevent division by zero
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
  focusIndex_ = newIndex;

  // Auto-adjust page
  int pageSize = getPageSize();
  if (focusIndex_ < pageOffset_) {
    pageOffset_ = (focusIndex_ / pageSize) * pageSize;
  } else if (focusIndex_ >= pageOffset_ + pageSize) {
    pageOffset_ = (focusIndex_ / pageSize) * pageSize;
  }

  needsRedraw_ = true;
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

  pageOffset_ = newOffset;
  focusIndex_ = pageOffset_;  // Focus on first item of new page

  needsRedraw_ = true;
  requestUpdate();
}

void GalleryActivity::openSelectedImage() {
  if (focusIndex_ < 0 || focusIndex_ >= imageIndex_.count()) return;

  auto viewer = std::make_unique<ViewerActivity>(
      renderer, mappedInput, imageIndex_, focusIndex_);
  startActivityForResult(std::move(viewer), [this](bool) {
    needsRedraw_ = true;
    requestUpdate();
  });
}

void GalleryActivity::openFileBrowser() {
  // TODO: push FileBrowserActivity when implemented
  LOG_DBG("GALLERY", "File browser requested");
}

void GalleryActivity::openSettings() {
  // TODO: push SettingsActivity when implemented
  LOG_DBG("GALLERY", "Settings requested");
}

// ── Thumbnail loading ───────────────────────────────────────────────────────

GridThumbInfo GalleryActivity::loadThumb(int globalIndex) {
  GridThumbInfo info = {};
  info.state = ThumbState::NotLoaded;

  if (globalIndex < 0 || globalIndex >= imageIndex_.count()) {
    return info;
  }

  const ImageEntry& entry = imageIndex_.at(globalIndex);
  char fullPath[256];
  imageIndex_.getFullPath(globalIndex, fullPath, sizeof(fullPath));

  // Check if cached thumbnail exists
  char cachePath[256];
  ThumbnailCache::getCachePath(fullPath, entry.fileSize, entry.modTime,
                               cachePath, sizeof(cachePath));

  if (!ThumbnailCache::exists(fullPath, entry.fileSize, entry.modTime)) {
    // Generate thumbnail synchronously (blocking but simple)
    info.state = ThumbState::Loading;
    if (!ThumbnailCache::generate(fullPath, entry.fileSize, entry.modTime)) {
      info.state = ThumbState::Failed;
      return info;
    }
  }

  // Read the BMP data from cache
  // Note: drawGridView handles rendering from BMP data
  // For now we report Loaded with null data — drawGridView will show placeholder
  // Full BMP loading requires reading into a buffer which we defer to
  // the grid renderer's built-in BMP file rendering
  info.state = ThumbState::Loaded;
  info.bmpData = nullptr;
  info.bmpWidth = ThumbnailCache::THUMB_WIDTH;
  info.bmpHeight = ThumbnailCache::THUMB_HEIGHT;

  return info;
}
