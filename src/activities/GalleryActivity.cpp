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

  firstRender_ = true;
  needsRedraw_ = true;
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

    bool ok = ThumbnailCache::generate(fullPath, entry.fileSize, entry.modTime);
    if (ok) {
      generated++;
    } else {
      LOG_ERR("GALLERY", "Thumb failed: %s (fmt=%d, size=%u)",
              entry.filename, static_cast<int>(entry.format), entry.fileSize);
    }

    // Update progress every 3 thumbnails
    if ((generated + alreadyCached) % 3 == 0 || i == total - 1) {
      int done = alreadyCached + generated;
      int progress = (done * 100) / total;
      renderer.clearScreen(0xFF);
      snprintf(msg, sizeof(msg), "%s (%d/%d)", tr(STR_LOADING), done, total);
      renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2 - 20, msg);
      THEME.drawProgressBar(renderer, (renderer.getScreenWidth() - 400) / 2,
                            renderer.getScreenHeight() / 2 + 10, 400, 16, progress);
      renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    }

    vTaskDelay(1);
  }

  LOG_INF("GALLERY", "Thumbnails: %d new, %d cached, %d total", generated, alreadyCached, total);
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
  if (!needsRedraw_) return;
  needsRedraw_ = false;

  renderer.clearScreen(0xFF);

  if (imageIndex_.count() == 0) {
    EmptyStateConfig cfg = {};
    cfg.title = tr(STR_NO_IMAGES);
    cfg.subtitle = tr(STR_NO_IMAGES_SUBTITLE);
    cfg.showStatusBar = true;
    cfg.showButtonHints = true;
    cfg.buttonLabels[0] = tr(STR_BROWSE);
    THEME.drawEmptyState(renderer, cfg);
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
    return;
  }

  // Status bar — show app name instead of raw "/" path
  StatusBarData status = {};
  const char* displayName = initialDir_;
  if (strcmp(initialDir_, "/") == 0) {
    displayName = tr(STR_APP_NAME);
  }
  status.folderName = displayName;
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

  // First render uses FULL_REFRESH to clear ghosting, subsequent uses FAST_REFRESH
  renderer.displayBuffer(firstRender_ ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
  firstRender_ = false;
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
  focusIndex_ = pageOffset_;

  needsRedraw_ = true;
  requestUpdate();
}

void GalleryActivity::openSelectedImage() {
  if (focusIndex_ < 0 || focusIndex_ >= imageIndex_.count()) return;

  auto viewer = std::make_unique<ViewerActivity>(
      renderer, mappedInput, imageIndex_, focusIndex_);
  startActivityForResult(std::move(viewer), [this](bool) {
    needsRedraw_ = true;
    firstRender_ = true;  // Full refresh when returning from viewer
    requestUpdate();
  });
}

void GalleryActivity::openFileBrowser() {
  LOG_DBG("GALLERY", "File browser requested");
}

void GalleryActivity::openSettings() {
  LOG_DBG("GALLERY", "Settings requested");
}

// ── Thumbnail loading ───────────────────────────────────────────────────────

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
    info.state = ThumbState::Failed;
  }

  info.bmpWidth = ThumbnailCache::THUMB_WIDTH;
  info.bmpHeight = ThumbnailCache::THUMB_HEIGHT;
  return info;
}
