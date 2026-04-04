#!/usr/bin/env python3
"""
Generate I18n C++ files for Y-X4 Album.

Wrapper that reuses crosspoint-reader's gen_i18n.py with album-specific paths.
Called as PlatformIO pre-build script.
"""

import sys
import os

def main():
    # Determine project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    translations_dir = os.path.join(project_root, "translations")
    # Output to shared I18n library directory
    output_dir = os.path.normpath(os.path.join(
        project_root, "..", "..", "lib", "crosspoint-reader", "lib", "I18n"
    ))

    if not os.path.isdir(translations_dir):
        print(f"[gen_i18n] No translations directory found at {translations_dir}, skipping")
        return

    if not os.path.isdir(output_dir):
        print(f"[gen_i18n] Output directory not found: {output_dir}, skipping")
        return

    # Import and run the shared gen_i18n.py
    reader_scripts = os.path.normpath(os.path.join(
        project_root, "..", "..", "lib", "crosspoint-reader", "scripts"
    ))

    if os.path.isdir(reader_scripts):
        sys.path.insert(0, reader_scripts)
        try:
            import gen_i18n
            print(f"[gen_i18n] Generating album i18n files...")
            print(f"  Translations: {translations_dir}")
            print(f"  Output: {output_dir}")
            gen_i18n.main(translations_dir, output_dir)
            return
        except ImportError:
            print(f"[gen_i18n] Could not import gen_i18n from {reader_scripts}")
        except Exception as e:
            print(f"[gen_i18n] Error: {e}")
            sys.exit(1)

    print("[gen_i18n] crosspoint-reader gen_i18n.py not found, skipping")


if __name__ == "__main__":
    main()
else:
    # Called by PlatformIO as extra_script
    try:
        Import("env")
        print("[gen_i18n] Running album i18n generation from PlatformIO...")
        main()
    except NameError:
        pass
