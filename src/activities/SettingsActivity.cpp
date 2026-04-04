#include "SettingsActivity.h"

#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include "fontIds.h"
#include "settings/AlbumSettings.h"

// ── Lifecycle ──────────────────────────────────────────────────────────────

void SettingsActivity::onEnter() {
  Activity::onEnter();
  LOG_INF("SETTINGS", "Opening settings");
  selectedIndex_ = 0;
  scrollOffset_ = 0;
  dialogActive_ = false;
  confirmActive_ = false;
  needsRedraw_ = true;
  requestUpdate();
}

void SettingsActivity::onExit() {
  SETTINGS.saveToFile();
  Activity::onExit();
}

// ── Input handling ─────────────────────────────────────────────────────────

void SettingsActivity::loop() {
  using Button = MappedInputManager::Button;

  // ── Confirm dialog active ──
  if (confirmActive_) {
    if (mappedInput.wasPressed(Button::Left) || mappedInput.wasPressed(Button::Up)) {
      if (confirmFocus_ > 0) { confirmFocus_--; needsRedraw_ = true; requestUpdate(); }
      return;
    }
    if (mappedInput.wasPressed(Button::Right) || mappedInput.wasPressed(Button::Down)) {
      if (confirmFocus_ < 1) { confirmFocus_++; needsRedraw_ = true; requestUpdate(); }
      return;
    }
    if (mappedInput.wasPressed(Button::Confirm)) {
      if (confirmFocus_ == 0) executeAction(confirmItem_);
      confirmActive_ = false;
      needsRedraw_ = true;
      requestUpdate();
      return;
    }
    if (mappedInput.wasPressed(Button::Back)) {
      confirmActive_ = false;
      needsRedraw_ = true;
      requestUpdate();
      return;
    }
    return;
  }

  // ── Select dialog active ──
  if (dialogActive_) {
    int optCount = getOptionCount(dialogItem_);
    if (mappedInput.wasPressed(Button::Up)) {
      if (dialogFocus_ > 0) { dialogFocus_--; needsRedraw_ = true; requestUpdate(); }
      return;
    }
    if (mappedInput.wasPressed(Button::Down)) {
      if (dialogFocus_ < optCount - 1) { dialogFocus_++; needsRedraw_ = true; requestUpdate(); }
      return;
    }
    if (mappedInput.wasPressed(Button::Confirm)) {
      applySelectChoice(dialogItem_, dialogFocus_);
      dialogActive_ = false;
      needsRedraw_ = true;
      requestUpdate();
      return;
    }
    if (mappedInput.wasPressed(Button::Back)) {
      dialogActive_ = false;
      needsRedraw_ = true;
      requestUpdate();
      return;
    }
    return;
  }

  // ── Main settings list ──
  if (mappedInput.wasPressed(Button::Up)) {
    if (selectedIndex_ > 0) {
      selectedIndex_--;
      adjustScrollForSelection();
      needsRedraw_ = true;
      requestUpdate();
    }
    return;
  }
  if (mappedInput.wasPressed(Button::Down)) {
    if (selectedIndex_ < ITEM_COUNT - 1) {
      selectedIndex_++;
      adjustScrollForSelection();
      needsRedraw_ = true;
      requestUpdate();
    }
    return;
  }
  if (mappedInput.wasPressed(Button::Confirm)) {
    auto id = static_cast<ItemId>(selectedIndex_);
    if (id == ItemId::ClearCache || id == ItemId::ResetDefaults) {
      openConfirmDialog(id);
    } else if (id == ItemId::About) {
      // About is info-only, no action
    } else {
      openSelectDialog(id);
    }
    return;
  }
  if (mappedInput.wasPressed(Button::Back)) {
    finish();
    return;
  }

  // Left/Right quick-adjust current value
  if (mappedInput.wasPressed(Button::Left)) {
    auto id = static_cast<ItemId>(selectedIndex_);
    if (id != ItemId::ClearCache && id != ItemId::ResetDefaults && id != ItemId::About) {
      int cur = getCurrentOption(id);
      if (cur > 0) {
        applySelectChoice(id, cur - 1);
        needsRedraw_ = true;
        requestUpdate();
      }
    }
    return;
  }
  if (mappedInput.wasPressed(Button::Right)) {
    auto id = static_cast<ItemId>(selectedIndex_);
    if (id != ItemId::ClearCache && id != ItemId::ResetDefaults && id != ItemId::About) {
      int cur = getCurrentOption(id);
      if (cur < getOptionCount(id) - 1) {
        applySelectChoice(id, cur + 1);
        needsRedraw_ = true;
        requestUpdate();
      }
    }
    return;
  }
}

// ── Rendering ──────────────────────────────────────────────────────────────

void SettingsActivity::render(RenderLock&&) {
  if (!needsRedraw_) return;
  needsRedraw_ = false;

  const auto& m = THEME.getMetrics();
  int screenW = renderer.getScreenWidth();
  int screenH = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);

  // Status bar
  StatusBarData sb{};
  sb.folderName = nullptr;
  sb.currentIndex = 0;
  sb.totalCount = 0;
  sb.batteryPercent = powerManager.getBatteryPercentage();
  sb.showClock = true;
  THEME.drawStatusBar(renderer, sb);

  // Title in status bar area
  renderer.drawText(UI_10_FONT_ID, 7, 5, tr(STR_SETTINGS));

  // Build list items
  char valueBuffers[ITEM_COUNT][24];
  ListItem items[ITEM_COUNT];
  buildListItems(items, valueBuffers);

  // List area
  Rect listRect;
  listRect.x = 0;
  listRect.y = m.statusBarHeight;
  listRect.width = screenW;
  listRect.height = screenH - m.statusBarHeight - m.buttonHintsHeight;

  ListViewConfig listCfg{};
  listCfg.selectedIndex = selectedIndex_;
  listCfg.scrollOffset = scrollOffset_;
  listCfg.showScrollBar = (ITEM_COUNT > listRect.height / m.listRowHeight);
  listCfg.showSeparators = true;

  THEME.drawListView(renderer, listRect, items, ITEM_COUNT, listCfg);

  // Button hints
  auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), nullptr, nullptr);
  THEME.drawButtonHints(renderer, labels.btn1, labels.btn2, nullptr, nullptr);

  // Draw dialog overlay if active
  if (dialogActive_) {
    int optCount = getOptionCount(dialogItem_);
    const char* optLabels[8];
    for (int i = 0; i < optCount && i < 8; i++) {
      optLabels[i] = getOptionLabel(dialogItem_, i);
    }

    DialogConfig dlgCfg{};
    dlgCfg.title = items[static_cast<int>(dialogItem_)].title;
    dlgCfg.message = nullptr;
    dlgCfg.confirmLabel = tr(STR_CONFIRM);
    dlgCfg.cancelLabel = tr(STR_CANCEL);
    dlgCfg.options = optLabels;
    dlgCfg.optionCount = optCount;
    dlgCfg.currentOption = getCurrentOption(dialogItem_);

    THEME.drawDialog(renderer, dlgCfg, dialogFocus_);
  }

  if (confirmActive_) {
    const char* msg = (confirmItem_ == ItemId::ClearCache)
                          ? tr(STR_CLEAR_CACHE_CONFIRM)
                          : tr(STR_RESET_CONFIRM);
    DialogConfig dlgCfg{};
    dlgCfg.title = (confirmItem_ == ItemId::ClearCache) ? tr(STR_CLEAR_CACHE) : tr(STR_RESET_DEFAULTS);
    dlgCfg.message = msg;
    dlgCfg.confirmLabel = tr(STR_CONFIRM);
    dlgCfg.cancelLabel = tr(STR_CANCEL);
    dlgCfg.options = nullptr;
    dlgCfg.optionCount = 0;
    dlgCfg.currentOption = -1;

    THEME.drawDialog(renderer, dlgCfg, confirmFocus_);
  }

  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}

// ── List item building ─────────────────────────────────────────────────────

// Map ItemId to StrId for titles
static StrId ITEM_TITLE_KEYS[] = {
    StrId::STR_SLIDESHOW_INTERVAL,
    StrId::STR_SLIDESHOW_ORDER,
    StrId::STR_SCALE_MODE,
    StrId::STR_ORIENTATION,
    StrId::STR_RENDER_MODE,
    StrId::STR_AUTO_SLEEP,
    StrId::STR_LOOP_BROWSING,
    StrId::STR_SIDE_BUTTONS,
    StrId::STR_LANGUAGE,
    StrId::STR_CLEAR_CACHE,
    StrId::STR_RESET_DEFAULTS,
    StrId::STR_ABOUT,
};

void SettingsActivity::buildListItems(ListItem* items, char valueBuffers[][24]) const {
  for (int i = 0; i < ITEM_COUNT; i++) {
    items[i].title = I18N.get(ITEM_TITLE_KEYS[i]);
    items[i].subtitle = nullptr;
    items[i].icon = nullptr;
    items[i].enabled = true;

    auto id = static_cast<ItemId>(i);
    if (id == ItemId::ClearCache || id == ItemId::ResetDefaults) {
      items[i].value = nullptr;
    } else if (id == ItemId::About) {
      snprintf(valueBuffers[i], 24, "v1.0.0");
      items[i].value = valueBuffers[i];
    } else {
      const char* label = getValueLabel(id);
      snprintf(valueBuffers[i], 24, "%s", label);
      items[i].value = valueBuffers[i];
    }
  }
}

// ── Option configuration (all using tr() for i18n) ─────────────────────────

static const uint16_t INTERVAL_VALUES[] = {5, 10, 30, 60, 300};
static const uint16_t SLEEP_VALUES[] = {0, 5, 10, 30, 60};

// StrId arrays for option labels
static const StrId INTERVAL_KEYS[] = {
    StrId::STR_5_SEC, StrId::STR_10_SEC, StrId::STR_30_SEC,
    StrId::STR_1_MIN, StrId::STR_5_MIN,
};
static const StrId ORDER_KEYS[] = {StrId::STR_ORDER_SEQUENTIAL, StrId::STR_ORDER_RANDOM};
static const StrId SCALE_KEYS[] = {StrId::STR_SCALE_FIT, StrId::STR_SCALE_FILL};
static const StrId ORIENT_KEYS[] = {
    StrId::STR_ORIENTATION_LANDSCAPE, StrId::STR_ORIENTATION_PORTRAIT,
    StrId::STR_ORIENTATION_LANDSCAPE_INV, StrId::STR_ORIENTATION_PORTRAIT_INV,
};
static const StrId RENDER_KEYS[] = {StrId::STR_RENDER_GRAYSCALE, StrId::STR_RENDER_BW};
static const StrId SLEEP_KEYS[] = {
    StrId::STR_NEVER, StrId::STR_5_MIN, StrId::STR_10_MIN,
    StrId::STR_30_MIN, StrId::STR_1_HOUR,
};
static const StrId TOGGLE_KEYS[] = {StrId::STR_OFF, StrId::STR_ON};

int SettingsActivity::getOptionCount(ItemId id) const {
  switch (id) {
    case ItemId::SlideshowInterval: return 5;
    case ItemId::SlideshowOrder:    return 2;
    case ItemId::ScaleMode:         return 2;
    case ItemId::Orientation:       return 4;
    case ItemId::RenderMode:        return 2;
    case ItemId::AutoSleep:         return 5;
    case ItemId::LoopBrowsing:      return 2;
    case ItemId::SideButtonReversed: return 2;
    case ItemId::Language:          return static_cast<int>(Language::_COUNT);
    default: return 0;
  }
}

const char* SettingsActivity::getOptionLabel(ItemId id, int option) const {
  switch (id) {
    case ItemId::SlideshowInterval: return I18N.get(INTERVAL_KEYS[option]);
    case ItemId::SlideshowOrder:    return I18N.get(ORDER_KEYS[option]);
    case ItemId::ScaleMode:         return I18N.get(SCALE_KEYS[option]);
    case ItemId::Orientation:       return I18N.get(ORIENT_KEYS[option]);
    case ItemId::RenderMode:        return I18N.get(RENDER_KEYS[option]);
    case ItemId::AutoSleep:         return I18N.get(SLEEP_KEYS[option]);
    case ItemId::LoopBrowsing:      return I18N.get(TOGGLE_KEYS[option]);
    case ItemId::SideButtonReversed: return I18N.get(TOGGLE_KEYS[option]);
    case ItemId::Language:          return I18N.getLanguageName(static_cast<Language>(option));
    default: return "";
  }
}

int SettingsActivity::getCurrentOption(ItemId id) const {
  const auto& d = SETTINGS.data;
  switch (id) {
    case ItemId::SlideshowInterval:
      for (int i = 0; i < 5; i++) {
        if (d.slideshowInterval == INTERVAL_VALUES[i]) return i;
      }
      return 1;  // default 10s
    case ItemId::SlideshowOrder:    return d.slideshowOrder;
    case ItemId::ScaleMode:         return d.scaleMode;
    case ItemId::Orientation:       return d.orientation;
    case ItemId::RenderMode:        return d.renderMode;
    case ItemId::AutoSleep:
      for (int i = 0; i < 5; i++) {
        if (d.autoSleepMinutes == SLEEP_VALUES[i]) return i;
      }
      return 3;  // default 30min
    case ItemId::LoopBrowsing:      return d.loopBrowsing;
    case ItemId::SideButtonReversed: return d.sideButtonReversed;
    case ItemId::Language:          return static_cast<int>(I18N.getLanguage());
    default: return 0;
  }
}

const char* SettingsActivity::getValueLabel(ItemId id) const {
  int cur = getCurrentOption(id);
  return getOptionLabel(id, cur);
}

// ── Dialog management ──────────────────────────────────────────────────────

void SettingsActivity::openSelectDialog(ItemId id) {
  dialogItem_ = id;
  dialogFocus_ = getCurrentOption(id);
  dialogActive_ = true;
  needsRedraw_ = true;
  requestUpdate();
}

void SettingsActivity::openConfirmDialog(ItemId id) {
  confirmItem_ = id;
  confirmFocus_ = 1;  // default to Cancel for safety
  confirmActive_ = true;
  needsRedraw_ = true;
  requestUpdate();
}

void SettingsActivity::applySelectChoice(ItemId id, int choice) {
  auto& d = SETTINGS.data;
  switch (id) {
    case ItemId::SlideshowInterval:
      d.slideshowInterval = INTERVAL_VALUES[choice];
      break;
    case ItemId::SlideshowOrder:
      d.slideshowOrder = static_cast<uint8_t>(choice);
      break;
    case ItemId::ScaleMode:
      d.scaleMode = static_cast<uint8_t>(choice);
      break;
    case ItemId::Orientation:
      d.orientation = static_cast<uint8_t>(choice);
      break;
    case ItemId::RenderMode:
      d.renderMode = static_cast<uint8_t>(choice);
      break;
    case ItemId::AutoSleep:
      d.autoSleepMinutes = SLEEP_VALUES[choice];
      break;
    case ItemId::LoopBrowsing:
      d.loopBrowsing = static_cast<uint8_t>(choice);
      break;
    case ItemId::SideButtonReversed:
      d.sideButtonReversed = static_cast<uint8_t>(choice);
      break;
    case ItemId::Language:
      I18N.setLanguage(static_cast<Language>(choice));
      d.language = static_cast<uint8_t>(choice);
      break;
    default:
      break;
  }
  LOG_INF("SETTINGS", "Changed setting %d to option %d", static_cast<int>(id), choice);
}

void SettingsActivity::executeAction(ItemId id) {
  if (id == ItemId::ClearCache) {
    LOG_INF("SETTINGS", "Clearing thumbnail cache");
    Storage.removeDir("/.y-x4-album/thumbs");
    Storage.ensureDirectoryExists("/.y-x4-album/thumbs");
    THEME.drawToast(renderer, tr(STR_CACHE_CLEARED), ToastType::Success);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  } else if (id == ItemId::ResetDefaults) {
    LOG_INF("SETTINGS", "Resetting to defaults");
    SETTINGS.resetToDefaults();
    THEME.drawToast(renderer, tr(STR_DEFAULTS_RESTORED), ToastType::Success);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  }
}

void SettingsActivity::adjustScrollForSelection() {
  int visibleRows = THEME.getListVisibleRows(renderer);
  if (selectedIndex_ < scrollOffset_) {
    scrollOffset_ = selectedIndex_;
  } else if (selectedIndex_ >= scrollOffset_ + visibleRows) {
    scrollOffset_ = selectedIndex_ - visibleRows + 1;
  }
}
