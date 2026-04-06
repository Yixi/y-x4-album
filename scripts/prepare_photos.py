#!/usr/bin/env python3
"""
X4 电子相册照片批量处理脚本

将照片裁剪为适合阅星瞳 X4 的分辨率，采用居中 cover 裁剪。
横向照片也会被裁剪为竖向。

用法:
  python3 scripts/prepare_photos.py ~/Photos/原图 ~/Photos/输出

选项:
  -w, --width     目标宽度 (默认 480)
  -h, --height    目标高度 (默认 800)
  -q, --quality   JPEG 质量 1-100 (默认 90)
  -g, --grayscale 转为灰度 (E-Ink 只显示灰度，可减小文件体积)
  -f, --format    输出格式 jpg/png (默认 jpg)
  --landscape     横屏模式，目标改为 800x480
  --dry-run       只打印计划，不实际处理
"""

import argparse
import os
import sys
from pathlib import Path

try:
    from PIL import Image, ImageOps
except ImportError:
    print("需要安装 Pillow: pip3 install Pillow")
    sys.exit(1)

SUPPORTED_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tiff", ".tif"}


def center_cover_crop(img: Image.Image, target_w: int, target_h: int) -> Image.Image:
    """
    居中 cover 裁剪：缩放图片使其完全覆盖目标区域，然后居中裁剪。
    等价于 CSS 的 object-fit: cover。
    """
    src_w, src_h = img.size
    target_ratio = target_w / target_h
    src_ratio = src_w / src_h

    if src_ratio > target_ratio:
        # 图片更宽 — 按高度缩放，裁剪左右
        new_h = target_h
        new_w = round(src_w * (target_h / src_h))
    else:
        # 图片更高 — 按宽度缩放，裁剪上下
        new_w = target_w
        new_h = round(src_h * (target_w / src_w))

    img = img.resize((new_w, new_h), Image.LANCZOS)

    # 居中裁剪
    left = (new_w - target_w) // 2
    top = (new_h - target_h) // 2
    img = img.crop((left, top, left + target_w, top + target_h))

    return img


def process_image(src_path: Path, dst_path: Path, target_w: int, target_h: int,
                  quality: int, grayscale: bool, fmt: str) -> bool:
    try:
        img = Image.open(src_path)

        # 处理 EXIF 旋转
        img = ImageOps.exif_transpose(img)

        # 转为 RGB（处理 RGBA、P 模式等）
        if img.mode not in ("RGB", "L"):
            img = img.convert("RGB")

        # 居中 cover 裁剪到目标尺寸
        img = center_cover_crop(img, target_w, target_h)

        # 可选灰度转换
        if grayscale:
            img = img.convert("L")

        # 保存
        dst_path.parent.mkdir(parents=True, exist_ok=True)

        save_kwargs = {}
        if fmt == "jpg":
            save_kwargs = {"quality": quality, "optimize": True}
            if dst_path.suffix.lower() not in (".jpg", ".jpeg"):
                dst_path = dst_path.with_suffix(".jpg")
        elif fmt == "png":
            save_kwargs = {"optimize": True}
            if dst_path.suffix.lower() != ".png":
                dst_path = dst_path.with_suffix(".png")

        img.save(dst_path, **save_kwargs)
        return True

    except Exception as e:
        print(f"  ✗ 失败: {src_path.name} — {e}")
        return False


def collect_images(src_dir: Path) -> list[Path]:
    images = []
    for f in sorted(src_dir.rglob("*")):
        if f.is_file() and f.suffix.lower() in SUPPORTED_EXTS:
            images.append(f)
    return images


def main():
    parser = argparse.ArgumentParser(
        description="X4 电子相册照片批量处理 — 居中 cover 裁剪",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 基本用法（竖屏 480x800，灰度 JPEG）
  python3 prepare_photos.py ~/原图 ~/输出 -g

  # 横屏模式
  python3 prepare_photos.py ~/原图 ~/输出 --landscape

  # 自定义尺寸和质量
  python3 prepare_photos.py ~/原图 ~/输出 -w 800 -h 480 -q 95
        """,
    )
    parser.add_argument("input", help="输入目录（支持子目录递归）")
    parser.add_argument("output", help="输出目录")
    parser.add_argument("-W", "--width", type=int, default=480, help="目标宽度 (默认 480)")
    parser.add_argument("-H", "--height", type=int, default=800, help="目标高度 (默认 800)")
    parser.add_argument("-q", "--quality", type=int, default=90, help="JPEG 质量 (默认 90)")
    parser.add_argument("-g", "--grayscale", action="store_true", help="转为灰度")
    parser.add_argument("-f", "--format", choices=["jpg", "png"], default="jpg", help="输出格式")
    parser.add_argument("--landscape", action="store_true", help="横屏模式 (800x480)")
    parser.add_argument("--dry-run", action="store_true", help="只打印计划")

    args = parser.parse_args()

    if args.landscape:
        args.width, args.height = 800, 480

    src_dir = Path(args.input).expanduser().resolve()
    dst_dir = Path(args.output).expanduser().resolve()

    if not src_dir.is_dir():
        print(f"错误: 输入目录不存在: {src_dir}")
        sys.exit(1)

    images = collect_images(src_dir)
    if not images:
        print(f"未找到图片文件 ({', '.join(SUPPORTED_EXTS)})")
        sys.exit(0)

    print(f"X4 照片处理")
    print(f"  输入: {src_dir} ({len(images)} 张)")
    print(f"  输出: {dst_dir}")
    print(f"  目标: {args.width}×{args.height} {'灰度' if args.grayscale else '彩色'} {args.format.upper()}")
    print(f"  裁剪: 居中 cover")
    print()

    if args.dry_run:
        for img_path in images:
            rel = img_path.relative_to(src_dir)
            print(f"  {rel}")
        print(f"\n共 {len(images)} 张，使用 --dry-run 模式，未实际处理")
        return

    success = 0
    failed = 0

    for i, img_path in enumerate(images, 1):
        rel = img_path.relative_to(src_dir)
        dst_path = dst_dir / rel

        # 强制输出扩展名
        if args.format == "jpg":
            dst_path = dst_path.with_suffix(".jpg")
        elif args.format == "png":
            dst_path = dst_path.with_suffix(".png")

        pct = i * 100 // len(images)
        print(f"  [{pct:3d}%] {rel}", end="", flush=True)

        if process_image(img_path, dst_path, args.width, args.height,
                         args.quality, args.grayscale, args.format):
            # 显示文件大小
            size_kb = dst_path.stat().st_size / 1024
            print(f" → {size_kb:.0f}KB")
            success += 1
        else:
            failed += 1

    print(f"\n完成: {success} 张成功, {failed} 张失败")
    if success > 0:
        total_mb = sum(f.stat().st_size for f in dst_dir.rglob("*") if f.is_file()) / 1024 / 1024
        print(f"输出目录总大小: {total_mb:.1f}MB")


if __name__ == "__main__":
    main()
