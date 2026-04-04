#include "AlbumTheme.h"

#include <Bitmap.h>
#include <HalGPIO.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "fontIds.h"

// ── Helpers ─────────────────────────────────────────────────────────────────

namespace {

// Battery icon: 16×10 body + 2px cap
void drawBatteryShape(const GfxRenderer& renderer, int x, int y, int bodyW, int bodyH, int capW,
                      int percent, bool charging) {
  // Outer rectangle (body)
  renderer.drawRect(x, y, bodyW, bodyH);
  // Cap (right side protrusion)
  renderer.fillRect(x + bodyW, y + 3, capW, bodyH - 6);

  // Fill level
  const int innerW = bodyW - 2;
  const int innerH = bodyH - 2;
  int fillW = (innerW * percent + 50) / 100;
  if (fillW < 0) fillW = 0;
  if (fillW > innerW) fillW = innerW;
  if (fillW > 0) {
    renderer.fillRect(x + 1, y + 1, fillW, innerH);
  }

  // Charging lightning bolt (simple 'Z' shape, white on black fill)
  if (charging && fillW >= 6) {
    const int bx = x + 4;
    const int by = y + 2;
    renderer.drawLine(bx + 3, by + 0, bx + 4, by + 0, false);
    renderer.drawLine(bx + 2, by + 1, bx + 3, by + 1, false);
    renderer.drawLine(bx + 1, by + 2, bx + 4, by + 2, false);
    renderer.drawLine(bx + 2, by + 3, bx + 3, by + 3, false);
    renderer.drawLine(bx + 1, by + 4, bx + 2, by + 4, false);
    renderer.drawLine(bx + 0, by + 5, bx + 1, by + 5, false);
  }
}

}  // namespace

// ── StatusBar ───────────────────────────────────────────────────────────────

void AlbumTheme::drawStatusBar(GfxRenderer& renderer, const StatusBarData& data) const {
  const int screenW = renderer.getScreenWidth();
  const int barH = metrics_.statusBarHeight;

  // Background: white fill then bottom separator line
  renderer.fillRect(0, 0, screenW, barH, false);
  renderer.drawLine(0, barH - 1, screenW - 1, barH - 1);

  // -- Left: folder icon placeholder (24×24 filled square) + folder name --
  const int iconX = 7;
  const int iconY = 0;
  // Simple folder icon: draw a small rectangle as placeholder
  renderer.drawRect(iconX, iconY + 4, 20, 16);
  renderer.drawLine(iconX, iconY + 4, iconX + 8, iconY + 4);
  renderer.drawLine(iconX, iconY + 2, iconX + 8, iconY + 2);
  renderer.drawLine(iconX, iconY + 2, iconX, iconY + 4);
  renderer.drawLine(iconX + 8, iconY + 2, iconX + 8, iconY + 4);

  if (data.folderName && data.folderName[0] != '\0') {
    const int maxFolderW = isLandscape(renderer) ? 200 : 120;
    auto truncated = renderer.truncatedText(UI_10_FONT_ID, data.folderName, maxFolderW);
    renderer.drawText(UI_10_FONT_ID, 35, 2, truncated.c_str());
  }

  // -- Center: image count "N/M 张" --
  if (data.totalCount > 0) {
    char countBuf[32];
    if (data.currentIndex > 0) {
      snprintf(countBuf, sizeof(countBuf), "%d/%d", data.currentIndex, data.totalCount);
    } else {
      snprintf(countBuf, sizeof(countBuf), "%d", data.totalCount);
    }
    renderer.drawCenteredText(UI_10_FONT_ID, 2, countBuf);
  }

  // -- Right: battery icon + percentage --
  const int battX = screenW - AlbumMetrics::VIEWABLE_MARGIN_RIGHT - 4 - metrics_.batteryWidth;
  const int battY = (barH - metrics_.batteryHeight) / 2;
  const bool charging = gpio.isUsbConnected();
  drawBatteryShape(renderer, battX, battY, metrics_.batteryWidth, metrics_.batteryHeight,
                   metrics_.batteryCapWidth, data.batteryPercent, charging);

  // Battery percentage text
  char percentBuf[8];
  snprintf(percentBuf, sizeof(percentBuf), "%d%%", data.batteryPercent);
  const int percentW = renderer.getTextWidth(SMALL_FONT_ID, percentBuf);
  renderer.drawText(SMALL_FONT_ID, battX - 4 - percentW, 4, percentBuf);

  // Time text (if enabled)
  if (data.showClock) {
    // For now use a placeholder; real time requires RTC which X4 may not have
    const char* timeStr = "--:--";
    const int timeW = renderer.getTextWidth(SMALL_FONT_ID, timeStr);
    renderer.drawText(SMALL_FONT_ID, battX - 4 - percentW - 8 - timeW, 4, timeStr);
  }
}

void AlbumTheme::drawBatteryIcon(const GfxRenderer& renderer, int x, int y, int percent) const {
  const bool charging = gpio.isUsbConnected();
  drawBatteryShape(renderer, x, y, metrics_.batteryWidth, metrics_.batteryHeight,
                   metrics_.batteryCapWidth, percent, charging);
}

// ── ButtonHints ─────────────────────────────────────────────────────────────

void AlbumTheme::drawButtonHints(GfxRenderer& renderer, const char* label1, const char* label2,
                                 const char* label3, const char* label4) const {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();
  const int barH = metrics_.buttonHintsHeight;
  const int barY = screenH - barH;

  // Separator line at top of button area
  renderer.drawLine(0, barY, screenW - 1, barY);

  const int btnW = metrics_.buttonWidth;
  const int btnSp = metrics_.buttonSpacing;
  const int totalW = 4 * btnW + 3 * btnSp;
  const int startX = (screenW - totalW) / 2;

  const char* labels[4] = {label1, label2, label3, label4};

  for (int i = 0; i < 4; i++) {
    const char* lbl = labels[i];
    if (!lbl || lbl[0] == '\0') continue;

    const int bx = startX + i * (btnW + btnSp);
    const int by = barY + 1;  // 1px below separator
    const int bh = barH - 1;

    // Draw button border
    renderer.drawRect(bx, by, btnW, bh);

    // Center text in button
    const int textW = renderer.getTextWidth(UI_10_FONT_ID, lbl);
    const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
    const int textX = bx + (btnW - textW) / 2;
    const int textY = by + (bh - lineH) / 2;
    renderer.drawText(UI_10_FONT_ID, textX, textY, lbl);
  }
}

// ── ProgressBar ─────────────────────────────────────────────────────────────

void AlbumTheme::drawProgressBar(GfxRenderer& renderer, int x, int y, int width, int height,
                                 int progress) const {
  if (progress < 0) progress = 0;
  if (progress > 100) progress = 100;

  // Outer frame
  renderer.drawRect(x, y, width, height);

  // Background (light gray dither for unfilled portion)
  renderer.fillRectDither(x + 1, y + 1, width - 2, height - 2, Color::LightGray);

  // Filled portion (black)
  const int fillW = ((width - 2) * progress + 50) / 100;
  if (fillW > 0) {
    renderer.fillRect(x + 1, y + 1, fillW, height - 2);
  }
}

// ── ListView ────────────────────────────────────────────────────────────────

void AlbumTheme::drawListView(GfxRenderer& renderer, const Rect& rect, const ListItem items[],
                              int itemCount, const ListViewConfig& config) const {
  const int rowH = metrics_.listRowHeight;
  const int visibleRows = rect.height / rowH;
  const int padX = metrics_.contentSidePadding;
  const int screenW = renderer.getScreenWidth();

  for (int i = 0; i < visibleRows && (config.scrollOffset + i) < itemCount; i++) {
    const int idx = config.scrollOffset + i;
    const ListItem& item = items[idx];
    const int rowY = rect.y + i * rowH;
    const bool selected = (idx == config.selectedIndex);

    // Selected row: inverted colors
    if (selected) {
      renderer.fillRect(0, rowY, screenW, rowH);
    }

    const bool textBlack = !selected;
    int textX = padX;

    // Icon (24×24)
    if (item.icon) {
      const int iconY = rowY + (rowH - metrics_.listIconSize) / 2;
      if (selected) {
        // For selected row, draw icon inverted (white on black)
        // drawIcon draws transparent-bg icons, works on both backgrounds
        renderer.drawIcon(item.icon, padX, iconY, metrics_.listIconSize, metrics_.listIconSize);
      } else {
        renderer.drawImage(item.icon, padX, iconY, metrics_.listIconSize, metrics_.listIconSize);
      }
      textX = padX + metrics_.listIconSize + 10;
    }

    // Right-side value
    int rightEdge = screenW - padX;
    if (item.value && item.value[0] != '\0') {
      const int valW = renderer.getTextWidth(SMALL_FONT_ID, item.value);
      const int valY = rowY + (rowH - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
      renderer.drawText(SMALL_FONT_ID, rightEdge - valW, valY, item.value, textBlack);

      // Arrow indicator "▸"
      renderer.drawText(SMALL_FONT_ID, rightEdge - valW - 12, valY, ">", textBlack);
      rightEdge = rightEdge - valW - 20;
    }

    // Title text
    if (item.title && item.title[0] != '\0') {
      const int maxTitleW = rightEdge - textX - 4;
      auto title = renderer.truncatedText(UI_10_FONT_ID, item.title, maxTitleW);
      const int titleY = rowY + (rowH - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
      renderer.drawText(UI_10_FONT_ID, textX, titleY, title.c_str(), textBlack);
    }

    // Subtitle (if present, item uses taller row)
    if (item.subtitle && item.subtitle[0] != '\0') {
      const int subY = rowY + rowH / 2 + 2;
      auto sub = renderer.truncatedText(SMALL_FONT_ID, item.subtitle, rightEdge - textX - 4);
      renderer.drawText(SMALL_FONT_ID, textX, subY, sub.c_str(), textBlack);
    }

    // Separator line
    if (config.showSeparators && i < visibleRows - 1 && (idx + 1) < itemCount) {
      if (!selected && (idx + 1) != config.selectedIndex) {
        renderer.drawLine(padX, rowY + rowH - 1, screenW - padX, rowY + rowH - 1);
      }
    }
  }

  // Scrollbar
  if (config.showScrollBar && itemCount > visibleRows) {
    drawScrollBar(renderer, rect, itemCount, visibleRows, config.scrollOffset);
  }
}

void AlbumTheme::drawScrollBar(const GfxRenderer& renderer, const Rect& rect, int totalItems,
                               int visibleItems, int scrollOffset) const {
  const int screenW = renderer.getScreenWidth();
  const int barX =
      screenW - AlbumMetrics::VIEWABLE_MARGIN_RIGHT - metrics_.listScrollBarOffset -
      metrics_.listScrollBarWidth;
  const int barY = rect.y;
  const int barH = rect.height;

  // Calculate thumb size and position
  int thumbH = (barH * visibleItems) / totalItems;
  if (thumbH < 20) thumbH = 20;

  const int maxScroll = totalItems - visibleItems;
  int thumbY = barY;
  if (maxScroll > 0) {
    thumbY = barY + ((barH - thumbH) * scrollOffset) / maxScroll;
  }

  // Draw thumb (dark gray dither)
  renderer.fillRectDither(barX, thumbY, metrics_.listScrollBarWidth, thumbH, Color::DarkGray);
}

// ── GridView ────────────────────────────────────────────────────────────────

void AlbumTheme::drawGridView(GfxRenderer& renderer, const GridViewConfig& config,
                              ThumbLoader loader) const {
  const bool landscape = isLandscape(renderer);
  const int cols = config.cols;
  const int rows = config.rows;
  const int tw = metrics_.thumbWidth;
  const int th = metrics_.thumbHeight;
  const int screenW = renderer.getScreenWidth();

  // Calculate grid origin and spacing based on orientation
  int spacingH, spacingV, marginLeft, marginTop;
  if (landscape) {
    spacingH = metrics_.gridSpacingH;
    spacingV = metrics_.gridSpacingV;
    marginLeft = metrics_.gridMarginLeft;
    marginTop = metrics_.gridMarginTop_landscape;
  } else {
    // Portrait: recalculate margins
    const int contentH =
        renderer.getScreenHeight() - metrics_.statusBarHeight - metrics_.buttonHintsHeight;
    spacingH = (screenW - AlbumMetrics::VIEWABLE_MARGIN_LEFT - AlbumMetrics::VIEWABLE_MARGIN_RIGHT -
                cols * tw) /
               (cols + 1);
    spacingV = (contentH - rows * th) / (rows + 1);
    marginLeft = AlbumMetrics::VIEWABLE_MARGIN_LEFT + spacingH;
    marginTop = metrics_.statusBarHeight + AlbumMetrics::VIEWABLE_MARGIN_TOP + spacingV;
  }

  const int pageSize = cols * rows;

  for (int i = 0; i < pageSize; i++) {
    const int globalIdx = config.pageOffset + i;
    if (globalIdx >= config.totalCount) break;

    const int col = i % cols;
    const int row = i / cols;
    const int cellX = marginLeft + col * (tw + spacingH);
    const int cellY = marginTop + row * (th + spacingV);
    const bool selected = (globalIdx == config.selectedIndex);

    // Border
    const int borderW = selected ? metrics_.thumbBorderSelected : metrics_.thumbBorderNormal;
    for (int b = 0; b < borderW; b++) {
      renderer.drawRect(cellX + b, cellY + b, tw - 2 * b, th - 2 * b);
    }

    // Thumbnail content
    GridThumbInfo info = loader(globalIdx);
    const int innerX = cellX + borderW;
    const int innerY = cellY + borderW;
    const int innerW = tw - 2 * borderW;
    const int innerH = th - 2 * borderW;

    switch (info.state) {
      case ThumbState::Loading:
        // Light gray fill as loading indicator
        renderer.fillRectDither(innerX, innerY, innerW, innerH, Color::LightGray);
        break;

      case ThumbState::Loaded: {
        if (info.cachePath[0] != '\0') {
          FsFile thumbFile;
          if (Storage.openFileForRead("THUMB_R", info.cachePath, thumbFile)) {
            Bitmap bmp(thumbFile, true);
            if (bmp.parseHeaders() == BmpReaderError::Ok) {
              renderer.drawBitmap(bmp, innerX, innerY, innerW, innerH);
            }
            thumbFile.close();
          }
        }
        break;
      }

      case ThumbState::Failed: {
        // Draw X mark in center
        const int cx = cellX + tw / 2;
        const int cy = cellY + th / 2;
        renderer.drawLine(cx - 8, cy - 8, cx + 8, cy + 8);
        renderer.drawLine(cx + 8, cy - 8, cx - 8, cy + 8);
        break;
      }

      case ThumbState::NotLoaded:
      default:
        // Empty frame only (border already drawn)
        break;
    }
  }

  // Page indicator (below grid, above button hints)
  if (config.totalCount > pageSize) {
    const int totalPages = (config.totalCount + pageSize - 1) / pageSize;
    const int currentPage = config.pageOffset / pageSize + 1;
    char pageBuf[16];
    snprintf(pageBuf, sizeof(pageBuf), "%d/%d", currentPage, totalPages);
    const int screenH = renderer.getScreenHeight();
    const int pageY = screenH - metrics_.buttonHintsHeight - renderer.getLineHeight(SMALL_FONT_ID) - 2;
    renderer.drawCenteredText(SMALL_FONT_ID, pageY, pageBuf);
  }
}

// ── Dialog ──────────────────────────────────────────────────────────────────

void AlbumTheme::drawDialog(GfxRenderer& renderer, const DialogConfig& config,
                            int focusedIndex) const {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();
  const int padX = metrics_.dialogPaddingX;
  const int padY = metrics_.dialogPaddingY;
  const int borderW = metrics_.dialogBorderWidth;

  // Calculate dialog dimensions
  int titleW = 0;
  if (config.title) {
    titleW = renderer.getTextWidth(UI_12_FONT_ID, config.title);
  }

  int bodyW = 0;
  int bodyLines = 0;
  if (config.message) {
    bodyW = renderer.getTextWidth(UI_10_FONT_ID, config.message);
    bodyLines = 1;
    // Count newlines
    for (const char* p = config.message; *p; p++) {
      if (*p == '\n') {
        bodyLines++;
      }
    }
  }

  // For Select type, calculate options width
  int optionsH = 0;
  int optionsW = 0;
  if (config.options && config.optionCount > 0) {
    optionsH = config.optionCount * 32;  // 32px per option row
    for (int i = 0; i < config.optionCount; i++) {
      int w = renderer.getTextWidth(UI_10_FONT_ID, config.options[i]);
      if (w > optionsW) optionsW = w;
    }
    optionsW += 40;  // "▶" marker + "✓" checkmark margins
  }

  int contentW = titleW;
  if (bodyW > contentW) contentW = bodyW;
  if (optionsW > contentW) contentW = optionsW;

  int dialogW = contentW + 2 * padX + 2 * borderW;
  if (dialogW < metrics_.dialogMinWidth) dialogW = metrics_.dialogMinWidth;
  const int maxW = isLandscape(renderer) ? metrics_.dialogMaxWidth : screenW - 40;
  if (dialogW > maxW) dialogW = maxW;

  // Height calculation
  const int titleH = config.title ? renderer.getLineHeight(UI_12_FONT_ID) + padY + 8 : 0;
  const int sepH = config.title ? 1 : 0;
  const int bodyH =
      (config.message && bodyLines > 0) ? bodyLines * renderer.getLineHeight(UI_10_FONT_ID) + 12 : 0;
  const int btnAreaH = metrics_.dialogButtonHeight + padY * 2;
  const int dialogH = padY + titleH + sepH + bodyH + optionsH + btnAreaH + padY;

  const int dx = (screenW - dialogW) / 2;
  const int dy = (screenH - dialogH) / 2;

  // Draw dialog background (white fill + border)
  renderer.fillRect(dx + borderW, dy + borderW, dialogW - 2 * borderW, dialogH - 2 * borderW,
                    false);
  for (int i = 0; i < borderW; i++) {
    renderer.drawRoundedRect(dx + i, dy + i, dialogW - 2 * i, dialogH - 2 * i, 1,
                             metrics_.dialogCornerRadius, true);
  }

  int curY = dy + padY;

  // Title
  if (config.title) {
    const int titleTextW = renderer.getTextWidth(UI_12_FONT_ID, config.title);
    renderer.drawText(UI_12_FONT_ID, dx + (dialogW - titleTextW) / 2, curY, config.title, true,
                      EpdFontFamily::BOLD);
    curY += renderer.getLineHeight(UI_12_FONT_ID) + 8;

    // Separator
    renderer.drawLine(dx + padX, curY, dx + dialogW - padX, curY);
    curY += 1 + 6;
  }

  // Message body
  if (config.message) {
    // Simple line-by-line rendering (split on \n)
    const char* start = config.message;
    while (*start) {
      const char* end = start;
      while (*end && *end != '\n') end++;
      char lineBuf[128];
      const int len = end - start;
      if (len > 0 && len < (int)sizeof(lineBuf)) {
        memcpy(lineBuf, start, len);
        lineBuf[len] = '\0';
        auto truncated = renderer.truncatedText(UI_10_FONT_ID, lineBuf, dialogW - 2 * padX);
        renderer.drawText(UI_10_FONT_ID, dx + padX, curY, truncated.c_str());
      }
      curY += renderer.getLineHeight(UI_10_FONT_ID);
      if (*end == '\n')
        start = end + 1;
      else
        break;
    }
    curY += 6;
  }

  // Select options
  if (config.options && config.optionCount > 0) {
    for (int i = 0; i < config.optionCount; i++) {
      const int optY = curY + i * 32;
      const bool optSelected = (i == focusedIndex);
      const bool isCurrent = (i == config.currentOption);

      if (optSelected) {
        renderer.fillRect(dx + padX, optY, dialogW - 2 * padX, 30);
      }

      // Selection marker
      if (optSelected) {
        renderer.drawText(UI_10_FONT_ID, dx + padX + 4, optY + 4, ">", !optSelected);
      }

      // Option text
      renderer.drawText(UI_10_FONT_ID, dx + padX + 20, optY + 4, config.options[i], !optSelected);

      // Current value checkmark
      if (isCurrent) {
        renderer.drawText(UI_10_FONT_ID, dx + dialogW - padX - 20, optY + 4, "*", !optSelected);
      }
    }
    curY += config.optionCount * 32;
    // Separator before buttons
    renderer.drawLine(dx + padX, curY, dx + dialogW - padX, curY);
    curY += 1;
  }

  // Buttons
  curY += padY;
  const int btnH = metrics_.dialogButtonHeight;
  const bool hasCancel = config.cancelLabel && config.cancelLabel[0] != '\0';

  if (hasCancel) {
    // Two buttons
    const int btnW = (dialogW - 2 * padX - metrics_.dialogButtonSpacing) / 2;
    const int btn1X = dx + padX;
    const int btn2X = btn1X + btnW + metrics_.dialogButtonSpacing;

    // Confirm button
    const char* confirmLbl = config.confirmLabel ? config.confirmLabel : "OK";
    if (focusedIndex == 0) {
      renderer.fillRect(btn1X, curY, btnW, btnH);
    }
    renderer.drawRect(btn1X, curY, btnW, btnH);
    int tw = renderer.getTextWidth(UI_10_FONT_ID, confirmLbl);
    renderer.drawText(UI_10_FONT_ID, btn1X + (btnW - tw) / 2,
                      curY + (btnH - renderer.getLineHeight(UI_10_FONT_ID)) / 2, confirmLbl,
                      focusedIndex != 0);

    // Cancel button
    if (focusedIndex == 1) {
      renderer.fillRect(btn2X, curY, btnW, btnH);
    }
    renderer.drawRect(btn2X, curY, btnW, btnH);
    tw = renderer.getTextWidth(UI_10_FONT_ID, config.cancelLabel);
    renderer.drawText(UI_10_FONT_ID, btn2X + (btnW - tw) / 2,
                      curY + (btnH - renderer.getLineHeight(UI_10_FONT_ID)) / 2,
                      config.cancelLabel, focusedIndex != 1);
  } else {
    // Single button centered
    const int btnW = 120;
    const int btnX = dx + (dialogW - btnW) / 2;
    const char* confirmLbl = config.confirmLabel ? config.confirmLabel : "OK";

    renderer.fillRect(btnX, curY, btnW, btnH);
    renderer.drawRect(btnX, curY, btnW, btnH);
    int tw = renderer.getTextWidth(UI_10_FONT_ID, confirmLbl);
    renderer.drawText(UI_10_FONT_ID, btnX + (btnW - tw) / 2,
                      curY + (btnH - renderer.getLineHeight(UI_10_FONT_ID)) / 2, confirmLbl,
                      false);
  }
}

// ── Toast ───────────────────────────────────────────────────────────────────

void AlbumTheme::drawToast(GfxRenderer& renderer, const char* message, ToastType type) const {
  if (!message || message[0] == '\0') return;

  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();
  const int toastH = metrics_.toastHeight;
  const int padX = metrics_.toastPaddingX;

  // Prefix icon character
  const char* prefix = "";
  switch (type) {
    case ToastType::Success:
      prefix = "OK ";
      break;
    case ToastType::Error:
      prefix = "X ";
      break;
    case ToastType::Info:
      prefix = "i ";
      break;
  }

  // Build full message
  char fullMsg[128];
  snprintf(fullMsg, sizeof(fullMsg), "%s%s", prefix, message);

  const int textW = renderer.getTextWidth(UI_10_FONT_ID, fullMsg);
  const int toastW = textW + 2 * padX;

  const int toastX = (screenW - toastW) / 2;
  const int toastY = screenH - metrics_.buttonHintsHeight - 8 - toastH;

  // Draw rounded black background
  renderer.fillRoundedRect(toastX, toastY, toastW, toastH, metrics_.toastCornerRadius,
                           Color::Black);

  // White text on black background
  const int textY = toastY + (toastH - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
  renderer.drawText(UI_10_FONT_ID, toastX + padX, textY, fullMsg, false);
}

// ── InfoOverlay ─────────────────────────────────────────────────────────────

void AlbumTheme::drawInfoOverlay(GfxRenderer& renderer, const OverlayData& data) const {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  // Top bar: 32px black background with filename and index
  const int topH = 32;
  renderer.fillRect(0, 0, screenW, topH);

  if (data.filename) {
    auto truncName = renderer.truncatedText(UI_10_FONT_ID, data.filename, screenW / 2);
    renderer.drawText(UI_10_FONT_ID, 12, 6, truncName.c_str(), false);
  }

  if (data.totalCount > 0) {
    char idxBuf[32];
    snprintf(idxBuf, sizeof(idxBuf), "%d/%d", data.currentIndex, data.totalCount);
    const int idxW = renderer.getTextWidth(UI_10_FONT_ID, idxBuf);
    renderer.drawText(UI_10_FONT_ID, screenW - 12 - idxW, 6, idxBuf, false);
  }

  // Bottom bar: 40px black background with button labels
  const int botY = screenH - metrics_.buttonHintsHeight;
  renderer.fillRect(0, botY, screenW, metrics_.buttonHintsHeight);

  // Draw button labels in white (same layout as ButtonHints but inverted colors)
  const int btnW = metrics_.buttonWidth;
  const int btnSp = metrics_.buttonSpacing;
  const int totalW = 4 * btnW + 3 * btnSp;
  const int startX = (screenW - totalW) / 2;

  for (int i = 0; i < 4; i++) {
    const char* lbl = data.buttonLabels[i];
    if (!lbl || lbl[0] == '\0') continue;

    const int bx = startX + i * (btnW + btnSp);
    const int textW = renderer.getTextWidth(UI_10_FONT_ID, lbl);
    const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
    const int textX = bx + (btnW - textW) / 2;
    const int textY = botY + (metrics_.buttonHintsHeight - lineH) / 2;
    renderer.drawText(UI_10_FONT_ID, textX, textY, lbl, false);
  }
}

// ── EmptyState ──────────────────────────────────────────────────────────────

void AlbumTheme::drawEmptyState(GfxRenderer& renderer, const EmptyStateConfig& config) const {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  // Determine content area
  const int contentTop = config.showStatusBar ? metrics_.statusBarHeight : 0;
  const int contentBot = config.showButtonHints ? screenH - metrics_.buttonHintsHeight : screenH;
  const int contentH = contentBot - contentTop;
  const int centerY = contentTop + contentH / 2;

  // Icon (32×32 centered)
  if (config.icon && config.iconSize > 0) {
    const int iconX = (screenW - config.iconSize) / 2;
    const int iconY = centerY - 50;
    renderer.drawImage(config.icon, iconX, iconY, config.iconSize, config.iconSize);
  }

  // Title
  if (config.title && config.title[0] != '\0') {
    renderer.drawCenteredText(UI_12_FONT_ID, centerY + 10, config.title);
  }

  // Subtitle (supports \n)
  if (config.subtitle && config.subtitle[0] != '\0') {
    int subY = centerY + 36;
    const char* start = config.subtitle;
    while (*start) {
      const char* end = start;
      while (*end && *end != '\n') end++;
      char lineBuf[128];
      const int len = end - start;
      if (len > 0 && len < (int)sizeof(lineBuf)) {
        memcpy(lineBuf, start, len);
        lineBuf[len] = '\0';
        renderer.drawCenteredText(UI_10_FONT_ID, subY, lineBuf);
      }
      subY += renderer.getLineHeight(UI_10_FONT_ID);
      if (*end == '\n')
        start = end + 1;
      else
        break;
    }
  }

  // Button hints
  if (config.showButtonHints) {
    drawButtonHints(renderer, config.buttonLabels[0], config.buttonLabels[1],
                    config.buttonLabels[2], config.buttonLabels[3]);
  }
}

// ── BootScreen ──────────────────────────────────────────────────────────────

void AlbumTheme::drawBootScreen(GfxRenderer& renderer, const char* statusText, int progress,
                                const char* version) const {
  const int screenW = renderer.getScreenWidth();

  // Product name
  renderer.drawCenteredText(UI_12_FONT_ID, 245, "X4 ALBUM", true, EpdFontFamily::BOLD);

  // Decorative line
  renderer.drawLine(360, 265, 440, 265);

  // Subtitle
  renderer.drawCenteredText(UI_10_FONT_ID, 278, "E-Ink Photo Album");

  // Status text
  if (statusText && statusText[0] != '\0') {
    renderer.drawCenteredText(UI_10_FONT_ID, 330, statusText);
  }

  // Progress bar
  if (progress >= 0) {
    const int barW = metrics_.progressBarWidth;
    const int barH = metrics_.progressBarHeight;
    const int barX = (screenW - barW) / 2;
    drawProgressBar(renderer, barX, 352, barW, barH, progress);
  }

  // Version
  if (version && version[0] != '\0') {
    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "v%s  Xteink", version);
    renderer.drawCenteredText(SMALL_FONT_ID, 448, verBuf);
  }
}

// ── ShutdownScreen ──────────────────────────────────────────────────────────

void AlbumTheme::drawShutdownScreen(GfxRenderer& renderer) const {
  renderer.clearScreen(0xFF);
  renderer.drawCenteredText(UI_12_FONT_ID, 210, "X4 ALBUM", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, 250, ":)");
}
