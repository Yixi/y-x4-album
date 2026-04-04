# Y-X4-Album UI 组件规格文档

> 版本：1.0  
> 日期：2026-04-04  
> 配套文档：[UX_DESIGN.md](./UX_DESIGN.md)  
> 目标读者：hw-developer（实现 AlbumTheme + 组件库）

---

## 目录

1. [全局常量与主题系统](#1-全局常量与主题系统)
2. [字体规格](#2-字体规格)
3. [图标规格](#3-图标规格)
4. [StatusBar 状态栏](#4-statusbar-状态栏)
5. [ButtonHints 按钮提示](#5-buttonhints-按钮提示)
6. [ProgressBar 进度条](#6-progressbar-进度条)
7. [ListView 列表视图](#7-listview-列表视图)
8. [GridView 缩略图网格](#8-gridview-缩略图网格)
9. [Dialog 对话框](#9-dialog-对话框)
10. [Toast 短暂提示](#10-toast-短暂提示)
11. [InfoOverlay 信息叠加层](#11-infooverlay-信息叠加层)
12. [启动画面与关机画面](#12-启动画面与关机画面)
13. [空状态与错误状态](#13-空状态与错误状态)
14. [屏幕方向适配规则](#14-屏幕方向适配规则)

---

## 1. 全局常量与主题系统

### 1.1 AlbumMetrics 结构体

所有布局数值集中定义在 `AlbumMetrics` 中，组件通过 `AlbumTheme::getMetrics()` 获取。

```cpp
struct AlbumMetrics {
    // ── 屏幕 ──
    static constexpr int SCREEN_W_LANDSCAPE = 800;
    static constexpr int SCREEN_H_LANDSCAPE = 480;
    static constexpr int SCREEN_W_PORTRAIT  = 480;
    static constexpr int SCREEN_H_PORTRAIT  = 800;

    // ── 可视边距（E-Ink 面板物理遮挡区域）──
    static constexpr int VIEWABLE_MARGIN_TOP    = 9;
    static constexpr int VIEWABLE_MARGIN_RIGHT  = 3;
    static constexpr int VIEWABLE_MARGIN_BOTTOM = 3;
    static constexpr int VIEWABLE_MARGIN_LEFT   = 3;

    // ── StatusBar ──
    int statusBarHeight      = 24;   // px

    // ── ButtonHints ──
    int buttonHintsHeight    = 40;   // px
    int buttonHintsCount     = 4;    // 前置按钮数量
    int buttonWidth          = 106;  // 每个按钮宽度
    int buttonSpacing        = 12;   // 按钮间距

    // ── 内容区域 ──
    int contentSidePadding   = 20;   // 左右内边距
    int contentTopPadding    = 8;    // 内容区顶部间距

    // ── 列表 ──
    int listRowHeight        = 50;   // 标准行高
    int listRowWithSubHeight = 70;   // 带副标题的行高
    int listIconSize         = 24;   // 列表图标尺寸
    int listScrollBarWidth   = 4;    // 滚动条宽度
    int listScrollBarOffset  = 5;    // 距右边距

    // ── 网格（Gallery）──
    int gridCols_landscape   = 5;
    int gridRows_landscape   = 3;
    int gridCols_portrait    = 3;
    int gridRows_portrait    = 5;
    int thumbWidth           = 140;  // 缩略图宽
    int thumbHeight          = 120;  // 缩略图高
    int thumbBorderNormal    = 1;    // 未选中边框
    int thumbBorderSelected  = 3;    // 选中边框
    int gridSpacingH         = 16;   // 水平间距（横屏）
    int gridSpacingV         = 11;   // 垂直间距（横屏）
    int gridMarginLeft       = 19;   // 网格左起始（横屏）
    int gridMarginTop_landscape = 44; // 网格顶起始（横屏，24状态栏+9边距+11间距）

    // ── 弹窗 ──
    int dialogMinWidth       = 280;  // 对话框最小宽度
    int dialogMaxWidth       = 500;  // 对话框最大宽度
    int dialogPaddingX       = 20;   // 水平内边距
    int dialogPaddingY       = 16;   // 垂直内边距
    int dialogCornerRadius   = 6;    // 圆角半径
    int dialogBorderWidth    = 2;    // 边框宽度
    int dialogButtonHeight   = 36;   // 对话框按钮高度
    int dialogButtonSpacing  = 16;   // 对话框按钮间距

    // ── Toast ──
    int toastHeight          = 36;   // Toast 高度
    int toastPaddingX        = 16;   // 水平内边距
    int toastCornerRadius    = 4;    // 圆角

    // ── 进度条 ──
    int progressBarHeight    = 16;   // 进度条高度
    int progressBarWidth     = 400;  // 进度条宽度（启动画面）

    // ── 电池图标 ──
    int batteryWidth         = 16;
    int batteryHeight        = 10;
    int batteryCapWidth      = 2;    // 凸出端宽度
};
```

### 1.2 颜色常量

E-Ink 显示仅支持有限颜色，UI 组件使用以下颜色：

```cpp
// 来自 GfxRenderer::Color
constexpr uint8_t COLOR_WHITE      = 0x01;  // 白色背景
constexpr uint8_t COLOR_LIGHT_GRAY = 0x05;  // 浅灰（抖动）
constexpr uint8_t COLOR_DARK_GRAY  = 0x0A;  // 深灰（抖动）
constexpr uint8_t COLOR_BLACK      = 0x10;  // 黑色前景
```

**UI 组件颜色用法**：

| 元素 | 颜色 | 说明 |
|------|------|------|
| 背景 | WHITE | 所有页面默认白色背景 |
| 主文字 | BLACK | 标题、正文、标签 |
| 次要文字 | DARK_GRAY | 副标题、说明文字、禁用项 |
| 选中项背景 | BLACK | 反色高亮（黑底白字） |
| 选中项文字 | WHITE | 反色高亮 |
| 边框 | BLACK | 缩略图边框、分隔线 |
| 缩略图占位 | LIGHT_GRAY | 加载中的占位纹理 |
| 进度条填充 | BLACK | 已完成部分 |
| 进度条背景 | LIGHT_GRAY | 未完成部分 |
| 叠加层背景 | BLACK（抖动） | 半透明效果 |

---

## 2. 字体规格

### 2.1 可用字体

基于 crosspoint-reader 的 EpdFont 系统，相册项目使用以下字体：

| 用途标识 | 字体 ID 常量 | 字体 | 大小 | 行高(advanceY) | 上升(ascender) |
|----------|-------------|------|------|----------------|---------------|
| 页面标题 | `UI_12_FONT_ID` | Ubuntu 12 | 12px | ~29px | 29px |
| 正文/标签 | `UI_10_FONT_ID` | Ubuntu 10 | 10px | ~24px | 24px |
| 小字/辅助 | `SMALL_FONT_ID` | 小号字体 | ~8px | ~18px | — |

### 2.2 字体用途分配

```
┌─────────────────────────────────────────────────────────────────────┐
│ 📁 我的照片           3/126 张                  10:30  ██▌ 85% │ ← UI_10 (文件夹名、计数)
│                                                          ↑         │   SMALL (时间、百分比)
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│              （内容区域 - 各组件自行选择字体）                        │
│                                                                     │
│  列表标题 ← UI_10                                                   │
│  列表副标题 ← SMALL                                                 │
│  弹窗标题 ← UI_12                                                   │
│  弹窗正文 ← UI_10                                                   │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│  [打开]         [幻灯片]        [文件夹]        [设置]              │ ← UI_10 (按钮标签)
└─────────────────────────────────────────────────────────────────────┘
```

### 2.3 文字绘制规则

1. **基线对齐**：`drawText(fontId, x, y, text)` 中 y 是文本区域的**顶部**，不是基线
2. **截断策略**：超长文本使用 `renderer.truncatedText(fontId, text, maxWidth)` 截断，自动添加省略号 "..."
3. **居中文本**：使用 `renderer.drawCenteredText(fontId, y, text)` 自动水平居中
4. **宽度测量**：绘制前用 `renderer.getTextWidth(fontId, text)` 预测量

---

## 3. 图标规格

### 3.1 图标清单

相册项目需要以下图标（24×24 像素，1-bit 单色位图）：

| 图标名 | 尺寸 | 用途 | 来源 |
|--------|------|------|------|
| `FolderIcon24` | 24×24 | 文件浏览器列表项、状态栏 | crosspoint-reader |
| `ImageIcon24` | 24×24 | 图片文件标识 | crosspoint-reader |
| `SettingsIcon24` | 24×24 | 设置页面标题 | 需新建或从32px缩放 |
| `SlideshowIcon24` | 24×24 | 幻灯片标识 | **新建** |
| `InfoIcon24` | 24×24 | 图片信息页标题 | **新建** |
| `ErrorIcon32` | 32×32 | 错误/空状态居中图标 | **新建** |
| `NoCardIcon32` | 32×32 | 无 TF 卡状态图标 | **新建** |
| `NoImageIcon32` | 32×32 | 无图片状态图标 | **新建** |
| `PauseIcon32` | 32×32 | 幻灯片暂停图标 | **新建** |
| `LogoIcon` | 200×80 | 启动/关机画面品牌 Logo | **新建** |

### 3.2 图标绘制规则

```cpp
// 使用 drawImage 绘制（透明背景）
renderer.drawImage(icon24_data, x, y, 24, 24);

// 使用 drawIcon 绘制（同 drawImage）
renderer.drawIcon(icon24_data, x, y, 24, 24);
```

### 3.3 电池图标精确规格

```
电池图标（16×10 像素）:

 ┌──────────────┐
 │██████████    │─┐  ← 凸出端 2px 宽 × 4px 高
 │██████████    │─┘
 └──────────────┘
  ↑ 填充宽度 = (16 - 2 - 2) × percentage / 100
  ↑ 左边距 1px，右侧留 2px 给凸出端

外框: drawRect(x, y, 14, 10)
凸出: fillRect(x+14, y+3, 2, 4)
填充: fillRect(x+1, y+1, fillWidth, 8)
```

**电池等级视觉区分**：

| 电量范围 | 填充格数 | 特殊标记 |
|----------|---------|---------|
| 76-100% | 4 格满 | — |
| 51-75% | 3 格 | — |
| 26-50% | 2 格 | — |
| 11-25% | 1 格 | — |
| 0-10% | 闪烁空框 | 低电量警告 |

---

## 4. StatusBar 状态栏

### 4.1 布局总览

```
横屏 800×480：
 0                                                                              800
 ├──3px──┬───────────────────────────────────────────────────────────────┬──3px──┤
 │       │                     StatusBar (24px 高)                      │       │
 │       ├──────────────────────────────────────────────────────────────┤       │
 │       │                     Content Area                             │       │
 9px     └──────────────────────────────────────────────────────────────┘       3px
(top)                                                                          (bottom)
```

### 4.2 StatusBar 内部布局

```
 x=3                                                                        x=797
 ├─4─┬──24──┬─6─┬───≤200px───┬──────── flex ─────────┬──time──┬─6─┬─batt─┬─4─┤
 │   │ icon │   │ folderName │       "N/M 张"         │ 10:30  │   │██▌85%│   │
 │   │24×24 │   │ UI_10      │     居中 UI_10          │ SMALL  │   │16×10 │   │
 └───┴──────┴───┴────────────┴────────────────────────┴────────┴───┴──────┴───┘
 ↕ 24px 高
```

**精确坐标（横屏）**：

| 元素 | X 坐标 | Y 坐标 | 宽度 | 高度 | 字体 |
|------|--------|--------|------|------|------|
| 背景矩形 | 0 | 0 | 800 | 24 | — |
| 文件夹图标 | 7 | 0 | 24 | 24 | — |
| 文件夹名 | 35 | 2 | ≤200 | 20 | UI_10 |
| 图片计数 | 居中计算 | 2 | auto | 20 | UI_10 |
| 时间文字 | 右侧计算 | 4 | auto | 16 | SMALL |
| 电池图标 | 800-3-4-16=777 | 7 | 16 | 10 | — |
| 电池百分比 | 777-4-textW | 4 | auto | 16 | SMALL |

**图片计数居中公式**：
```
countText = "3/126 张"
countWidth = getTextWidth(UI_10, countText)
countX = (800 - countWidth) / 2
```

**时间文字右对齐公式**：
```
timeText = "10:30"
timeWidth = getTextWidth(SMALL, timeText)
timeX = batteryPercentX - 8 - timeWidth
```

### 4.3 API 设计

```cpp
struct StatusBarData {
    const char* folderName;    // 当前文件夹名（可为 nullptr）
    int currentIndex;          // 当前图片索引（从 1 开始）
    int totalCount;            // 图片总数
    int batteryPercent;        // 电池百分比 0-100
    bool showClock;            // 是否显示时间
};

void drawStatusBar(GfxRenderer& renderer, const Rect& rect, const StatusBarData& data);
```

### 4.4 状态栏显示规则

| 界面 | 是否显示 | 特殊行为 |
|------|---------|---------|
| 启动画面 | 否 | — |
| 图库 | 是 | folderName + 选中图片索引/总数 |
| 全屏浏览 | 否 | 信息叠加层替代 |
| 幻灯片 | 否 | — |
| 文件浏览器 | 是 | folderName="选择文件夹"，无计数 |
| 设置 | 是 | folderName="设置"，无计数 |
| 图片信息 | 是 | folderName="图片信息"，当前图片索引/总数 |

---

## 5. ButtonHints 按钮提示

### 5.1 布局

底部固定 40px 高的区域，显示 4 个前置按钮的当前功能标签。

```
横屏 800×480，按钮提示区域 y=440：

 0                                                                              800
 ├─────────────────────────────────────────────────────────────────────────────────┤
 │                            分隔线（1px 黑色）                                    │ y=439
 ├───────┬──────────┬────┬──────────┬────┬──────────┬────┬──────────┬───────────┤
 │  边距  │ [按钮 1]  │间距│ [按钮 2]  │间距│ [按钮 3]  │间距│ [按钮 4]  │   边距    │ y=440
 │       │ 106×40   │    │ 106×40   │    │ 106×40   │    │ 106×40   │           │
 └───────┴──────────┴────┴──────────┴────┴──────────┴────┴──────────┴───────────┘ y=480
```

### 5.2 精确坐标计算

```
总按钮宽度 = 4 × 106 = 424px
总间距宽度 = 3 × 12 = 36px
总占用 = 424 + 36 = 460px
左边距 = (800 - 460) / 2 = 170px

按钮 1 (Confirm): x=170, y=440, w=106, h=40
按钮 2 (Back):    x=288, y=440, w=106, h=40
按钮 3 (Left):    x=406, y=440, w=106, h=40
按钮 4 (Right):   x=524, y=440, w=106, h=40
```

> 注意：按钮顺序对应物理前置按钮从左到右的排列。具体映射在 MappedInputManager 中配置，这里按默认顺序 Confirm → Back → Left → Right。如果实际硬件布局不同，需调整顺序。

### 5.3 按钮绘制细节

```
每个按钮（106×40px）：

 ┌────────────────────────────────┐
 │                                │  ← 1px 黑色边框
 │          [标签文字]             │  ← UI_10 字体，居中
 │                                │
 └────────────────────────────────┘

文字居中公式：
textWidth = getTextWidth(UI_10, label)
textX = buttonX + (106 - textWidth) / 2
textY = buttonY + (40 - lineHeight) / 2
```

**状态变化**：

| 状态 | 边框 | 背景 | 文字 |
|------|------|------|------|
| 正常（有功能） | 1px BLACK | WHITE | BLACK |
| 无功能（空标签） | 无 | WHITE | — |
| 按下反馈 | 2px BLACK | BLACK | WHITE（反色） |

### 5.4 API 设计

```cpp
// labels 数组长度为 4，空字符串 "" 或 nullptr 表示该位置不显示按钮
void drawButtonHints(GfxRenderer& renderer,
                     const char* label1,   // Confirm 按钮标签
                     const char* label2,   // Back 按钮标签
                     const char* label3,   // Left 按钮标签
                     const char* label4);  // Right 按钮标签
```

---

## 6. ProgressBar 进度条

### 6.1 标准进度条

```
宽 400px × 高 16px（启动画面居中使用）：

 ┌──────────────────────────────────────────────────────────────────────────────┐
 │████████████████████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
 └──────────────────────────────────────────────────────────────────────────────┘
  ↑ 1px 黑色边框
  ↑ 填充区域 = (width - 2) × progress / 100
  ↑ 未填充区域 = LIGHT_GRAY 抖动填充
```

**绘制步骤**：
```
1. drawRect(x, y, width, height)                          // 外框
2. fillRectDither(x+1, y+1, width-2, height-2, LIGHT_GRAY) // 背景
3. fillRect(x+1, y+1, fillWidth, height-2)                // 已完成（黑色）
```

### 6.2 弹窗内进度条（紧凑版）

```
宽度 = dialogWidth - 40px（左右各 20px 边距）
高度 = 8px
无边框，直接填充
```

### 6.3 API 设计

```cpp
void drawProgressBar(GfxRenderer& renderer, const Rect& rect, int progress);
// progress: 0-100
// rect: 进度条位置和尺寸
```

---

## 7. ListView 列表视图

### 7.1 标准列表（文件浏览器、设置）

```
完整列表布局（横屏 800×480）：

 x=0                                                                          x=800
 ├── 20px ──┬── 24 ──┬─ 10 ─┬─────────── flex ──────────────┬── value ──┬─ 20 ─┤
 │  padding │  icon  │ gap  │        主标题 (UI_10)          │ 右侧值    │ pad  │
 │          │ 24×24  │      │                                │ (SMALL)   │      │
 ├──────────┴────────┴──────┴────────────────────────────────┴───────────┴──────┤
 │                           分隔线（1px LIGHT_GRAY）                            │
 ├──────────┬────────┬──────┬────────────────────────────────┬───────────┬──────┤
 │          │  icon  │      │        主标题                   │ 右侧值    │      │
 │          │        │      │                                │           │      │
 └──────────┴────────┴──────┴────────────────────────────────┴───────────┴──────┘
 ↕ 每行 50px
```

### 7.2 行内细节

**标准行（50px 高）**：
```
 ┌────────────────────────────────────────────────────────────────────────────────┐
 │  ┌────┐                                                                       │
 │  │icon│  主标题文字                                              ▸ 当前值     │
 │  │24px│                                                                       │
 │  └────┘                                                                       │
 ├────────────────────────────────────────────────────────────────────────────────┤ ← 分隔线
```

| 元素 | X 坐标 | Y 偏移(相对行顶) | 字体 |
|------|--------|-----------------|------|
| 图标 | 20 | (50-24)/2 = 13 | — |
| 主标题 | 20+24+10 = 54（有图标）或 20（无图标） | (50-lineHeight)/2 | UI_10 |
| 右侧值 | 800-20-valueWidth | (50-lineHeight)/2 | SMALL |
| 箭头 "▸" | 右侧值左侧 8px | 同上 | SMALL |
| 分隔线 | 20 | 49 | 宽 800-40，1px |

**选中行（反色高亮）**：
```
fillRect(0, rowY, 800, 50, true)          // 黑色背景
drawText(fontId, x, y, text, false)       // 白色文字（black=false）
drawImage(invertedIcon, x, y, 24, 24)     // 反色图标
```

### 7.3 可见行数

| 屏幕方向 | 可用高度 | 可见行数 |
|----------|---------|---------|
| 横屏 | 480 - 24 - 40 = 416px | 416 / 50 = 8 行 |
| 竖屏 | 800 - 24 - 40 = 736px | 736 / 50 = 14 行 |

### 7.4 滚动条

当列表项超过一页时显示：

```
位置：x = 800 - 3 - 5 - 4 = 788（横屏）
宽度：4px
高度：contentHeight（可用内容区高度）
滑块高度 = max(20, contentHeight × visibleCount / totalCount)
滑块 Y = contentTop + (contentHeight - thumbH) × scrollOffset / maxScroll

颜色：DARK_GRAY 抖动填充
```

### 7.5 翻页指示器

当有更多内容时，在列表顶部/底部显示箭头：

```
上方箭头（有更多上方内容时）：
  ▲ 居中，y = contentTop + 2，字体 SMALL

下方箭头（有更多下方内容时）：
  ▼ 居中，y = contentBottom - lineHeight - 2，字体 SMALL
```

### 7.6 API 设计

```cpp
struct ListItem {
    const char* title;          // 主标题
    const char* subtitle;       // 副标题（可选，nullptr 表示无）
    const char* value;          // 右侧值（可选）
    const uint8_t* icon;        // 24×24 图标位图（可选）
    bool enabled;               // 是否可选择
};

struct ListViewConfig {
    int selectedIndex;          // 当前选中索引
    int scrollOffset;           // 滚动偏移（第一个可见项索引）
    bool showScrollBar;         // 是否显示滚动条
    bool showSeparators;        // 是否显示分隔线
};

void drawListView(GfxRenderer& renderer,
                  const Rect& rect,
                  const ListItem items[],
                  int itemCount,
                  const ListViewConfig& config);
```

---

## 8. GridView 缩略图网格

### 8.1 横屏布局（5×3）

```
 可用区域: 800 × 416px (y=24 到 y=440)
 实际网格区域: 考虑可视边距后

 ┌── 19px ──┬──140──┬─16─┬──140──┬─16─┬──140──┬─16─┬──140──┬─16─┬──140──┬── 19px ──┐
 │          │      │    │      │    │      │    │      │    │      │          │ y=44
 │          │ 120  │    │      │    │ [选中] │    │      │    │      │          │
 │          │      │    │      │    │      │    │      │    │      │          │ y=164
 │          ├──────┤    ├──────┤    ├━━━━━━┤    ├──────┤    ├──────┤          │
 │          │      │    │      │    │      │    │      │    │      │          │ y=175
 │          │      │    │      │    │      │    │      │    │      │          │
 │          │      │    │      │    │      │    │      │    │      │          │ y=295
 │          ├──────┤    ├──────┤    ├──────┤    ├──────┤    ├──────┤          │
 │          │      │    │      │    │      │    │      │    │      │          │ y=306
 │          │      │    │      │    │      │    │      │    │      │          │
 │          │      │    │      │    │      │    │      │    │      │          │ y=426
 └──────────┴──────┴────┴──────┴────┴──────┴────┴──────┴────┴──────┴──────────┘
```

**精确坐标表（横屏 5×3）**：

| 位置 | 列 0 | 列 1 | 列 2 | 列 3 | 列 4 |
|------|------|------|------|------|------|
| 行 0 | (19,44) | (175,44) | (331,44) | (487,44) | (643,44) |
| 行 1 | (19,175) | (175,175) | (331,175) | (487,175) | (643,175) |
| 行 2 | (19,306) | (175,306) | (331,306) | (487,306) | (643,306) |

**坐标公式**：
```
cellX = gridMarginLeft + col × (thumbWidth + gridSpacingH)
cellY = gridMarginTop + row × (thumbHeight + gridSpacingV)

// 横屏：
cellX = 19 + col × 156
cellY = 44 + row × 131
```

### 8.2 竖屏布局（3×5）

```
内容区域: 480 × 736px

gridSpacingH_portrait = (480 - 6 - 3×140) / 4 = 14px
gridSpacingV_portrait = (736 - 5×120) / 6 = 22px (取整 20px)
gridMarginLeft_portrait = (480 - 3×140 - 2×14) / 2 = 26px (取整)
gridMarginTop_portrait = 24 + 9 + 20 = 53px
```

| 位置 | 列 0 | 列 1 | 列 2 |
|------|------|------|------|
| 行 0 | (26,53) | (180,53) | (334,53) |
| 行 1 | (26,193) | (180,193) | (334,193) |
| 行 2 | (26,333) | (180,333) | (334,333) |
| 行 3 | (26,473) | (180,473) | (334,473) |
| 行 4 | (26,613) | (180,613) | (334,613) |

### 8.3 缩略图单元格绘制

```
单个缩略图单元格（140×120px）：

未选中状态：
┌────────────────────────────────────────┐
│ 1px 黑色边框                           │
│ ┌────────────────────────────────────┐ │
│ │                                    │ │
│ │       缩略图 BMP 数据              │ │
│ │       (138 × 118 可用区域)         │ │
│ │                                    │ │
│ └────────────────────────────────────┘ │
└────────────────────────────────────────┘

选中状态：
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ 3px 黑色粗边框                          ┃
┃ ┌────────────────────────────────────┐ ┃
┃ │                                    │ ┃
┃ │       缩略图 BMP 数据              │ ┃
┃ │       (134 × 114 可用区域)         │ ┃
┃ │                                    │ ┃
┃ └────────────────────────────────────┘ ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**绘制步骤**：
```cpp
// 1. 绘制边框
int borderW = isSelected ? 3 : 1;
for (int i = 0; i < borderW; i++) {
    renderer.drawRect(cellX + i, cellY + i,
                      thumbWidth - 2*i, thumbHeight - 2*i);
}

// 2. 绘制缩略图（从 SD 卡缓存加载的 1-bit BMP）
renderer.drawBitmap1Bit(thumbBmp,
                        cellX + borderW, cellY + borderW,
                        thumbWidth - 2*borderW,
                        thumbHeight - 2*borderW);
```

### 8.4 缩略图加载状态

| 状态 | 视觉 | 绘制方式 |
|------|------|---------|
| 未加载 | 空白框 + 1px 边框 | `drawRect()` |
| 加载中 | 浅灰斜线纹理 | `fillRectDither(LIGHT_GRAY)` |
| 已加载 | 实际缩略图 | `drawBitmap1Bit()` |
| 加载失败 | 居中 × 图标 | `drawRect()` + `drawText("×")` |

### 8.5 页码指示

网格底部中心区域（按钮提示上方 4px）显示当前页码：

```
文字: "1/9" (当前页/总页数)
字体: SMALL
位置: 居中，y = buttonHintsY - lineHeight - 4

仅在总页数 > 1 时显示。
```

### 8.6 API 设计

```cpp
struct GridViewConfig {
    int selectedIndex;       // 当前选中图片在全局列表中的索引
    int pageOffset;          // 当前页的起始索引
    int totalCount;          // 图片总数
    int cols;                // 列数（由方向决定）
    int rows;                // 行数
};

// thumbLoader 回调：给定全局索引，返回缩略图 BMP 指针（或 nullptr 表示未加载）
using ThumbLoader = const uint8_t* (*)(int globalIndex);

void drawGridView(GfxRenderer& renderer,
                  const Rect& rect,
                  const GridViewConfig& config,
                  ThumbLoader loader);
```

---

## 9. Dialog 对话框

### 9.1 确认对话框

屏幕中央显示的模态弹窗，背景保持不变（不清屏）。

```
对话框布局：

┌───────────────────────────────────────────────────┐
│ 2px 黑色边框，6px 圆角                             │
│                                                   │
│    对话框标题 (UI_12, 居中, 加粗)                   │  ← y + 16
│                                                   │
│    ─────────────────────────────────────           │  ← 分隔线 1px
│                                                   │
│    正文消息行 1 (UI_10)                             │
│    正文消息行 2 (UI_10)                             │
│                                                   │
│    ┌──────────┐     ┌──────────┐                  │  ← 按钮区域
│    │  [确定]   │     │  [取消]   │                  │
│    └──────────┘     └──────────┘                  │
│                                                   │
└───────────────────────────────────────────────────┘
```

### 9.2 精确尺寸计算

```
标题高度 = lineHeight(UI_12) + 16(上边距) + 8(下边距)
分隔线 = 1px
正文高度 = lineCount × lineHeight(UI_10) + 12(上下边距)
按钮区域 = 36(按钮高) + 16(上边距) + 16(下边距) = 68px

对话框总高 = 标题高度 + 1 + 正文高度 + 按钮区域

对话框宽度 = max(280, min(500, max(titleWidth, bodyWidth) + 40))

居中坐标：
dialogX = (screenWidth - dialogWidth) / 2
dialogY = (screenHeight - dialogHeight) / 2
```

### 9.3 对话框按钮布局

```
双按钮模式：
按钮宽度 = (dialogWidth - 40 - 16) / 2   （左右边距 20，间距 16）
按钮 1 X = dialogX + 20
按钮 2 X = dialogX + 20 + btn1Width + 16

单按钮模式：
按钮宽度 = 120
按钮 X = dialogX + (dialogWidth - 120) / 2

按钮样式：
  默认（非焦点）: 1px 黑色边框，白色背景，黑色文字
  焦点: 黑色填充，白色文字
```

### 9.4 选择列表弹窗

设置项的选项选择使用特殊的列表弹窗：

```
┌───────────────────────────────────────┐
│  标题 (UI_12, 居中)                    │
│  ─────────────────────────────────    │
│    选项 1                              │
│  ▶ 选项 2  ✓                          │ ← 选中+当前值
│    选项 3                              │
│    选项 4                              │
│  ─────────────────────────────────    │
│  [确定]              [取消]            │
│                                       │
└───────────────────────────────────────┘

选项行高 = 32px
选中标记 "▶" : x = dialogX + 12
勾选标记 "✓" : x = dialogX + dialogWidth - 30
选项文字 : x = dialogX + 28
```

### 9.5 API 设计

```cpp
enum class DialogResult { Confirm, Cancel };
enum class DialogType { Confirm, Alert, Select };

struct DialogConfig {
    const char* title;
    const char* message;           // 正文（Confirm/Alert 类型）
    const char* confirmLabel;      // 确认按钮文字，默认 "确定"
    const char* cancelLabel;       // 取消按钮文字，默认 "取消"（nullptr=不显示）
    // Select 类型专用
    const char** options;          // 选项数组
    int optionCount;
    int selectedOption;            // 当前选中的选项索引
};

// 绘制对话框（不清屏，覆盖在当前内容上）
void drawDialog(GfxRenderer& renderer, const DialogConfig& config,
                int focusedButton);  // 0=确定, 1=取消 (Confirm 类型)
                                     // 0..N-1 (Select 类型)
```

---

## 10. Toast 短暂提示

### 10.1 布局

屏幕底部居中显示的短暂消息条，持续 1.5 秒后自动消失。

```
位置：屏幕底部上方 60px 处（在按钮提示上方）
横屏坐标：y = 480 - 40 - 8 - 36 = 396

┌──────────────────────────────────────────┐
│  4px 圆角，黑色背景                        │ 36px 高
│     ✓ 操作成功提示文字  (白色 UI_10)       │
└──────────────────────────────────────────┘
宽度 = textWidth + 16×2 + iconWidth + 8
居中: x = (800 - width) / 2
```

### 10.2 图标类型

| 类型 | 前缀图标 | 用途 |
|------|---------|------|
| 成功 | ✓ | 操作完成确认 |
| 错误 | ✗ | 操作失败提示 |
| 信息 | ℹ | 一般信息提示 |

### 10.3 API 设计

```cpp
enum class ToastType { Success, Error, Info };

void drawToast(GfxRenderer& renderer, const char* message, ToastType type);

// Toast 管理器（处理定时消失）
class ToastManager {
    void show(const char* message, ToastType type, int durationMs = 1500);
    void dismiss();
    bool isVisible() const;
    void render(GfxRenderer& renderer);  // 在 Activity::render 中调用
};
```

---

## 11. InfoOverlay 信息叠加层

### 11.1 布局

全屏浏览时叠加在图片上方的信息条，进入浏览器后显示 3 秒自动隐藏。

```
顶部信息条（32px 高，半透明黑色背景）：

 ┌────────────────────────────────────────────────────────────────────────────────────┐
 │ ██████████████████████████████████████████████████████████████████████████████████ │ y=0
 │ █  IMG_0042.jpg                                                 12/126         █ │
 │ ██████████████████████████████████████████████████████████████████████████████████ │ y=32
 └────────────────────────────────────────────────────────────────────────────────────┘

底部按钮条（40px 高，半透明黑色背景）：

 ┌────────────────────────────────────────────────────────────────────────────────────┐
 │ ██████████████████████████████████████████████████████████████████████████████████ │ y=440
 │ █  [信息]         [幻灯片]        [旋转]          [返回]                        █ │
 │ ██████████████████████████████████████████████████████████████████████████████████ │ y=480
 └────────────────────────────────────────────────────────────────────────────────────┘
```

### 11.2 精确规格

**顶部信息条**：
| 元素 | X | Y | 字体 | 颜色 |
|------|---|---|------|------|
| 背景 | 0 | 0 | — | 黑色填充，全宽×32px |
| 文件名 | 12 | 6 | UI_10 | 白色 (black=false) |
| 索引 | 800-12-textWidth | 6 | UI_10 | 白色 |

**底部按钮条**：
与 ButtonHints 相同布局，但背景为黑色填充，文字为白色。

### 11.3 "半透明"实现

E-Ink 不支持真正的透明度。使用以下方案模拟：

```
方案 A（推荐）：纯黑色背景条
  fillRect(0, 0, 800, 32, true)    // 黑色背景
  drawText(font, x, y, text, false) // 白色文字

方案 B（抖动灰度）：
  fillRectDither(0, 0, 800, 32, DARK_GRAY) // 深灰抖动
  drawText(font, x, y, text, false)         // 白色文字
  注意：抖动在 E-Ink 上可能产生颗粒感，不推荐用于文字背景
```

### 11.4 自动隐藏逻辑

```
显示时机：进入 ViewerActivity 后的第一次 render
持续时间：3000ms
隐藏方式：快刷新（仅重绘顶部和底部条的区域）
再次显示：用户按 Confirm 键
```

### 11.5 API 设计

```cpp
struct OverlayData {
    const char* filename;
    int currentIndex;
    int totalCount;
    const char* buttonLabels[4];  // 底部按钮标签
};

class InfoOverlay {
    void show(const OverlayData& data);
    void hide();
    bool isVisible() const;
    void updateAutoHide(uint32_t elapsedMs);  // 在 loop() 中调用
    void render(GfxRenderer& renderer);
};
```

---

## 12. 启动画面与关机画面

### 12.1 启动画面（BootScreen）

```
横屏 800×480 全屏，白色背景，居中布局：

 ┌────────────────────────────────────────────────────────────────────────────────────┐
 │                                                                                    │
 │                                                                                    │
 │                                                                                    │
 │                           ┌────────────────────────┐                               │
 │                           │      LOGO 图片          │                               │ y=140
 │                           │      200 × 80px        │                               │ 
 │                           └────────────────────────┘                               │ y=220
 │                                                                                    │
 │                              X 4   A L B U M                                       │ y=245, UI_12 居中
 │                             ─ ─ ─ ─ ─ ─ ─ ─                                       │ y=260, 装饰线 80px
 │                              电  子  相  册                                         │ y=275, UI_10 居中
 │                                                                                    │
 │                                                                                    │
 │                          正在扫描图片...  56%                                       │ y=330, UI_10 居中
 │                       ┌━━━━━━━━━━━━━━━░░░░░░░░░┐                                   │ y=352
 │                       └────────────────────────┘                                   │ y=368
 │                                                                                    │
 │                                                                                    │
 │                              v1.0.0  Xteink                                        │ y=448, SMALL 居中
 │                                                                                    │
 └────────────────────────────────────────────────────────────────────────────────────┘
```

**精确坐标**：

| 元素 | X | Y | 宽 | 高 | 字体 | 对齐 |
|------|---|---|----|----|------|------|
| Logo 图片 | 300 | 140 | 200 | 80 | — | 居中 |
| 产品名 "X4 ALBUM" | 居中 | 245 | auto | — | UI_12 | 居中 |
| 装饰线 | 360 | 265 | 80 | 1 | — | 居中 |
| 副标题 "电子相册" | 居中 | 278 | auto | — | UI_10 | 居中 |
| 状态文字 | 居中 | 330 | auto | — | UI_10 | 居中 |
| 进度条 | 200 | 352 | 400 | 16 | — | 居中 |
| 版本号 | 居中 | 448 | auto | — | SMALL | 居中 |

**状态文字变化**：

| 阶段 | 文字 |
|------|------|
| 初始化 | "正在初始化..." |
| TF 卡挂载 | "正在读取存储卡..." |
| 图片扫描 | "正在扫描图片... N%" |
| 缩略图生成 | "正在生成缩略图... N%" |
| 完成 | "就绪" → 自动跳转 |
| TF 卡失败 | → 切换到错误状态页面 |

### 12.2 关机画面（ShutdownScreen）

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                              X 4   A L B U M                                       │ y=210, UI_12 居中
│                                                                                    │
│                                再见 :)                                              │ y=250, UI_10 居中
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
│                                                                                    │
└────────────────────────────────────────────────────────────────────────────────────┘
```

**精确坐标**：

| 元素 | X | Y | 字体 |
|------|---|---|------|
| 产品名 | 居中 | 210 | UI_12 |
| 告别语 | 居中 | 250 | UI_10 |

> 设计极简。E-Ink 断电后画面保持，此画面将一直显示到下次开机。
> 全刷新绘制，确保画面干净无残影。

---

## 13. 空状态与错误状态

### 13.1 通用空状态布局

所有空状态/错误状态使用统一的居中布局：

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│ [StatusBar - 可选]                                                                 │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│                                                                                    │
│                              ┌──────────┐                                          │
│                              │  32×32   │                                          │ 区域中心 - 50px
│                              │  图标    │                                          │
│                              └──────────┘                                          │
│                                                                                    │
│                           主消息文字 (UI_12)                                        │ 区域中心 + 10px
│                                                                                    │
│                        辅助说明文字 (UI_10)                                          │ 区域中心 + 36px
│                        辅助说明第 2 行 (UI_10)                                       │ 区域中心 + 60px
│                                                                                    │
│                                                                                    │
├────────────────────────────────────────────────────────────────────────────────────┤
│ [ButtonHints - 可选]                                                               │
└────────────────────────────────────────────────────────────────────────────────────┘
```

**居中计算**：
```
contentArea = 有 StatusBar ? (24, 440) : (0, 480)  // 有 ButtonHints 则减 40
centerY = contentArea.y + contentArea.height / 2

iconX = (screenWidth - 32) / 2
iconY = centerY - 50
titleY = centerY + 10
subtitleY = centerY + 36
```

### 13.2 各错误状态配置

| 状态 | 图标 | 主消息 | 辅助消息 | StatusBar | ButtonHints |
|------|------|--------|---------|-----------|-------------|
| 无 TF 卡 | NoCardIcon32 | "未检测到存储卡" | "请插入 TF 卡后重新启动" | 否 | 否 |
| 无图片 | NoImageIcon32 | "未找到支持的图片" | "支持格式: JPEG, PNG, BMP\n请将图片复制到 TF 卡中" | 是 | 是: [—][—][文件夹][设置] |
| 图片加载失败 | ErrorIcon32 | "图片加载失败" | "{filename}\n文件可能已损坏" | 否(全屏) | 是: [—][—][—][跳过] |
| TF 卡读取错误 | ErrorIcon32 | "存储卡读取错误" | "请检查 TF 卡是否松动\n或尝试重新插入" | — (弹窗) | 弹窗按钮: [确定] |

### 13.3 API 设计

```cpp
struct EmptyStateConfig {
    const uint8_t* icon;         // 32×32 图标（nullptr 表示无图标）
    const char* title;           // 主消息
    const char* subtitle;        // 辅助消息（支持 \n 换行）
    bool showStatusBar;
    bool showButtonHints;
    const char* buttonLabels[4]; // 仅 showButtonHints=true 时有效
};

void drawEmptyState(GfxRenderer& renderer, const EmptyStateConfig& config);
```

---

## 14. 屏幕方向适配规则

### 14.1 四个方向

| 方向 | 逻辑宽 | 逻辑高 | 默认 |
|------|--------|--------|------|
| Landscape (横屏) | 800 | 480 | ✓ 默认 |
| Portrait (竖屏) | 480 | 800 | |
| LandscapeInverted (横屏倒置) | 800 | 480 | |
| PortraitInverted (竖屏倒置) | 480 | 800 | |

### 14.2 组件适配策略

所有组件必须基于 `renderer.getScreenWidth()` / `renderer.getScreenHeight()` 动态计算，不能硬编码 800/480。

**关键适配点**：

| 组件 | 横屏 (800×480) | 竖屏 (480×800) |
|------|---------------|---------------|
| StatusBar | 宽 800，高 24 | 宽 480，高 24。文件夹名最大宽度缩至 120px |
| ButtonHints | 4 按钮居中排列 | 4 按钮居中排列（间距不变，左右边距减小） |
| GridView | 5列 × 3行 = 15 张/页 | 3列 × 5行 = 15 张/页 |
| ListView | 8 可见行 | 14 可见行 |
| Dialog | 宽 280-500px，居中 | 宽 280-440px，居中 |
| Toast | 居中 | 居中 |
| ProgressBar | 宽 400px | 宽 300px |
| InfoOverlay | 顶部 32px + 底部 40px | 同上 |

### 14.3 方向切换处理

当用户在设置中更改屏幕方向时：
1. 保存新方向到设置
2. 调用 `renderer.setOrientation(newOrientation)`
3. 全刷新清屏
4. 重绘当前界面（所有组件自动使用新尺寸）
5. GridView 重新计算焦点位置（保持当前选中图片不变）

### 14.4 竖屏 ButtonHints 适配

```
竖屏 480px 宽：

总按钮宽度 = 4 × 106 = 424px
总间距宽度 = 3 × 12 = 36px
总占用 = 460px
左边距 = (480 - 460) / 2 = 10px

按钮 1: x=10,  y=760, w=106, h=40
按钮 2: x=128, y=760, w=106, h=40
按钮 3: x=246, y=760, w=106, h=40
按钮 4: x=364, y=760, w=106, h=40
```

---

## 附录 A：组件绘制顺序

每个 Activity 的 `render()` 方法中，组件按以下顺序绘制：

```
1. renderer.clearScreen()               // 清屏（白色）
2. drawStatusBar(...)                    // 状态栏（如有）
3. drawContent(...)                      // 主内容（GridView/ListView/图片等）
4. drawButtonHints(...)                  // 底部按钮提示（如有）
5. drawToast(...)                        // Toast（如有，覆盖在上方）
6. drawDialog(...)                       // 对话框（如有，覆盖在上方）
7. renderer.displayBuffer(refreshMode)   // 刷新屏幕
```

## 附录 B：内存预算参考

| 组件 | 内存占用 | 说明 |
|------|---------|------|
| 帧缓冲区 | 48,000 B | 800×480÷8，系统固定分配 |
| 灰度 LSB 缓冲 | 48,000 B | 仅图片渲染时使用 |
| 单个缩略图加载缓冲 | 2,100 B | 140×120÷8，逐个绘制到帧缓冲 |
| Dialog 绘制 | 0 B | 直接绘制到帧缓冲，无额外缓冲 |
| Toast 绘制 | 0 B | 同上 |
| StatusBar 绘制 | 0 B | 同上 |
| 文字缓冲 | <256 B | snprintf 栈缓冲 |

> 所有 UI 组件均直接操作帧缓冲区，不分配独立的绘制缓冲。
> 唯一需要额外缓冲的是缩略图加载（~2KB）和图片解码（由图片管线管理）。

## 附录 C：快速参考 - 常用坐标

### 横屏 800×480

```
StatusBar:    (0, 0, 800, 24)
Content:      (0, 24, 800, 416)
ButtonHints:  (0, 440, 800, 40)

Grid cell[0][0]: (19, 44, 140, 120)
Grid cell[r][c]: (19 + c*156, 44 + r*131, 140, 120)

Button[0]: (170, 440, 106, 40)
Button[1]: (288, 440, 106, 40)
Button[2]: (406, 440, 106, 40)
Button[3]: (524, 440, 106, 40)
```

### 竖屏 480×800

```
StatusBar:    (0, 0, 480, 24)
Content:      (0, 24, 480, 736)
ButtonHints:  (0, 760, 480, 40)

Grid cell[0][0]: (26, 53, 140, 120)
Grid cell[r][c]: (26 + c*154, 53 + r*140, 140, 120)

Button[0]: (10, 760, 106, 40)
Button[1]: (128, 760, 106, 40)
Button[2]: (246, 760, 106, 40)
Button[3]: (364, 760, 106, 40)
```
