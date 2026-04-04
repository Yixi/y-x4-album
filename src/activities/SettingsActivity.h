#pragma once

#include "Activity.h"
#include "components/AlbumTheme.h"

class SettingsActivity final : public Activity {
 public:
  explicit SettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Settings", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  // ── Menu items ──
  enum class ItemId : uint8_t {
    SlideshowInterval,
    SlideshowOrder,
    ScaleMode,
    Orientation,
    RenderMode,
    AutoSleep,
    LoopBrowsing,
    SideButtonReversed,
    Language,
    ClearCache,
    ResetDefaults,
    About,
    COUNT
  };
  static constexpr int ITEM_COUNT = static_cast<int>(ItemId::COUNT);

  // ── State ──
  int selectedIndex_ = 0;
  int scrollOffset_ = 0;
  bool needsRedraw_ = true;

  // Dialog state
  bool dialogActive_ = false;
  ItemId dialogItem_ = ItemId::SlideshowInterval;
  int dialogFocus_ = 0;

  // Confirm dialog state (for ClearCache / ResetDefaults)
  bool confirmActive_ = false;
  ItemId confirmItem_ = ItemId::ClearCache;
  int confirmFocus_ = 0;  // 0=confirm, 1=cancel

  // ── Helpers ──
  void buildListItems(ListItem* items, char valueBuffers[][24]) const;
  const char* getValueLabel(ItemId id) const;
  void openSelectDialog(ItemId id);
  void openConfirmDialog(ItemId id);
  void applySelectChoice(ItemId id, int choice);
  void executeAction(ItemId id);
  int getCurrentOption(ItemId id) const;
  int getOptionCount(ItemId id) const;
  const char* getOptionLabel(ItemId id, int option) const;
  void adjustScrollForSelection();
};
