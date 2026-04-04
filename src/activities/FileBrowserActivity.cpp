#include "FileBrowserActivity.h"

#include <HalPowerManager.h>
#include <I18n.h>
#include <Logging.h>

#include <cstdlib>
#include <cstring>

#include "ActivityManager.h"
#include "fontIds.h"

void FileBrowserActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("BROWSER", "Opening file browser at: %s", currentDir_);
  scanCurrentDir();
  needsRedraw_ = true;
  requestUpdate();
}

void FileBrowserActivity::onExit() {
  freeEntries();
  Activity::onExit();
}

void FileBrowserActivity::loop() {
  // Up/Down navigation
  nav_.onPressAndContinuous({MappedInputManager::Button::Down}, [this]() {
    if (entryCount_ == 0) return;
    selectedIndex_ = ButtonNavigator::nextIndex(selectedIndex_, entryCount_);
    int vis = getVisibleRows();
    if (selectedIndex_ >= scrollOffset_ + vis) scrollOffset_ = selectedIndex_ - vis + 1;
    if (selectedIndex_ < scrollOffset_) scrollOffset_ = selectedIndex_;
    needsRedraw_ = true;
    requestUpdate();
  });

  nav_.onPressAndContinuous({MappedInputManager::Button::Up}, [this]() {
    if (entryCount_ == 0) return;
    selectedIndex_ = ButtonNavigator::previousIndex(selectedIndex_, entryCount_);
    if (selectedIndex_ < scrollOffset_) scrollOffset_ = selectedIndex_;
    int vis = getVisibleRows();
    if (selectedIndex_ >= scrollOffset_ + vis) scrollOffset_ = selectedIndex_ - vis + 1;
    needsRedraw_ = true;
    requestUpdate();
  });

  // Confirm: enter directory or open gallery at current dir
  nav_.onPress({MappedInputManager::Button::Confirm}, [this]() { enterSelected(); });

  // Right: also enters selected
  nav_.onPress({MappedInputManager::Button::Right}, [this]() { enterSelected(); });

  // Back / Left: go up
  nav_.onPress({MappedInputManager::Button::Back}, [this]() { navigateUp(); });
  nav_.onPress({MappedInputManager::Button::Left}, [this]() { navigateUp(); });
}

void FileBrowserActivity::render(RenderLock&& lock) {
  if (!needsRedraw_) return;
  needsRedraw_ = false;

  renderer.clearScreen(0xFF);

  // Status bar
  StatusBarData status = {};
  status.folderName = currentDir_;
  status.currentIndex = entryCount_ > 0 ? selectedIndex_ + 1 : 0;
  status.totalCount = entryCount_;
  status.batteryPercent = powerManager.getBatteryPercentage();
  THEME.drawStatusBar(renderer, status);

  const auto& m = THEME.getMetrics();

  if (entryCount_ == 0) {
    EmptyStateConfig cfg = {};
    cfg.title = tr(STR_EMPTY_FOLDER);
    cfg.subtitle = tr(STR_NO_FILES_FOUND);
    cfg.showStatusBar = false;
    cfg.showButtonHints = true;
    cfg.buttonLabels[0] = tr(STR_BACK);
    THEME.drawEmptyState(renderer, cfg);
  } else {
    // Build ListItem array for visible items
    int vis = getVisibleRows();
    int drawCount = (entryCount_ - scrollOffset_ < vis) ? entryCount_ - scrollOffset_ : vis;

    // Stack-allocate ListItems for visible rows (each ~24 bytes, max ~10 rows = 240 bytes)
    ListItem items[drawCount];
    for (int i = 0; i < drawCount; i++) {
      const BrowserEntry& e = entries_[scrollOffset_ + i];
      items[i].title = e.name;
      items[i].subtitle = nullptr;
      items[i].icon = nullptr;
      items[i].enabled = true;

      if (e.isDir) {
        items[i].value = ">";
      } else {
        items[i].value = nullptr;
      }
    }

    Rect listRect = {0, m.statusBarHeight, renderer.getScreenWidth(),
                     THEME.getContentHeight(renderer)};

    ListViewConfig listCfg = {};
    listCfg.selectedIndex = selectedIndex_ - scrollOffset_;
    listCfg.scrollOffset = 0;  // We handle scrolling ourselves
    listCfg.showScrollBar = (entryCount_ > vis);
    listCfg.showSeparators = true;

    THEME.drawListView(renderer, listRect, items, drawCount, listCfg);
  }

  // Button hints
  THEME.drawButtonHints(renderer, tr(STR_BACK), tr(STR_OPEN), nullptr, nullptr);

  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}

// ── Directory scanning ──────────────────────────────────────────────────────

bool FileBrowserActivity::scanCurrentDir() {
  freeEntries();

  FsFile dir = Storage.open(currentDir_);
  if (!dir || !dir.isDirectory()) {
    LOG_ERR("BROWSER", "Cannot open directory: %s", currentDir_);
    return false;
  }

  entries_ = static_cast<BrowserEntry*>(malloc(sizeof(BrowserEntry) * MAX_ENTRIES));
  if (!entries_) {
    LOG_ERR("BROWSER", "Failed to allocate browser entries");
    dir.close();
    return false;
  }

  char name[128];
  int count = 0;

  for (FsFile file = dir.openNextFile(); file && count < MAX_ENTRIES;
       file = dir.openNextFile()) {
    file.getName(name, sizeof(name));

    // Skip hidden files/dirs
    if (name[0] == '.') {
      file.close();
      continue;
    }

    bool isDirectory = file.isDirectory();

    // Include directories and supported image files
    if (!isDirectory && !ImageScanner::isSupportedImage(name)) {
      file.close();
      continue;
    }

    strncpy(entries_[count].name, name, sizeof(entries_[count].name) - 1);
    entries_[count].name[sizeof(entries_[count].name) - 1] = '\0';
    entries_[count].isDir = isDirectory;
    entries_[count].fileSize = isDirectory ? 0 : static_cast<uint32_t>(file.fileSize());
    count++;

    file.close();
  }

  dir.close();
  entryCount_ = count;

  // Sort: directories first, then alphabetical
  for (int i = 1; i < entryCount_; i++) {
    BrowserEntry tmp;
    memcpy(&tmp, &entries_[i], sizeof(BrowserEntry));
    int j = i - 1;
    while (j >= 0) {
      bool swap = false;
      if (tmp.isDir && !entries_[j].isDir) {
        swap = true;
      } else if (tmp.isDir == entries_[j].isDir &&
                 strcasecmp(tmp.name, entries_[j].name) < 0) {
        swap = true;
      }
      if (!swap) break;
      memcpy(&entries_[j + 1], &entries_[j], sizeof(BrowserEntry));
      j--;
    }
    memcpy(&entries_[j + 1], &tmp, sizeof(BrowserEntry));
  }

  selectedIndex_ = 0;
  scrollOffset_ = 0;

  LOG_INF("BROWSER", "Found %d items in %s", entryCount_, currentDir_);
  return true;
}

void FileBrowserActivity::freeEntries() {
  if (entries_) {
    free(entries_);
    entries_ = nullptr;
  }
  entryCount_ = 0;
}

void FileBrowserActivity::navigateUp() {
  if (strcmp(currentDir_, "/") == 0) {
    // At root — go back to gallery
    finish();
    return;
  }

  // Remove last path component
  char* lastSlash = strrchr(currentDir_, '/');
  if (lastSlash && lastSlash != currentDir_) {
    *lastSlash = '\0';
  } else {
    strcpy(currentDir_, "/");
  }

  scanCurrentDir();
  needsRedraw_ = true;
  requestUpdate();
}

void FileBrowserActivity::enterSelected() {
  if (entryCount_ == 0 || selectedIndex_ >= entryCount_) return;

  const BrowserEntry& e = entries_[selectedIndex_];

  if (e.isDir) {
    // Navigate into directory
    size_t len = strlen(currentDir_);
    if (len == 1 && currentDir_[0] == '/') {
      snprintf(currentDir_, sizeof(currentDir_), "/%s", e.name);
    } else {
      snprintf(currentDir_ + len, sizeof(currentDir_) - len, "/%s", e.name);
    }
    scanCurrentDir();
    needsRedraw_ = true;
    requestUpdate();
  } else {
    // Open gallery at current directory
    openGallery(currentDir_);
  }
}

void FileBrowserActivity::openGallery(const char* dirPath) {
  // Return to gallery with new directory
  activityManager.goToGallery(dirPath);
}

int FileBrowserActivity::getVisibleRows() const {
  return THEME.getListVisibleRows(renderer);
}
