#!/usr/bin/env python3
# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import glob
import os
import shutil
import sys

def copy_files(src_dir, dst_dir, patterns):
    os.makedirs(dst_dir, exist_ok=True)
    for pattern in patterns:
        for src_path in glob.glob(os.path.join(src_dir, pattern)):
            if os.path.isfile(src_path):
                filename = os.path.basename(src_path)
                dst_path = os.path.join(dst_dir, filename)
                shutil.copy2(src_path, dst_path)
                print(f"Copied: {src_path} -> {dst_path}")

def main():
    src_root = sys.argv[1] if len(sys.argv) > 1 else 'protobuf'
    dst_root = sys.argv[2] if len(sys.argv) > 2 else '.'

    ext_src_dir = os.path.join(src_root, 'php', 'ext', 'google', 'protobuf')
    utf8_range_src_dir = os.path.join(src_root, 'third_party', 'utf8_range')

    if not os.path.exists(ext_src_dir):
        print(f"Error: Extension source directory not found: {ext_src_dir}")
        sys.exit(1)

    if not os.path.exists(utf8_range_src_dir):
        fallback_dir = os.path.join(ext_src_dir, 'third_party', 'utf8_range')
        if os.path.exists(fallback_dir):
            utf8_range_src_dir = fallback_dir
        else:
            print(f"Warning: utf8_range source directory not found at {utf8_range_src_dir}")

    print(f"Syncing PHP extension files from {src_root} to {dst_root}...")

    # Copy root extension files (.c, .h, .inc, config files, composer.json, README.md)
    copy_files(ext_src_dir, dst_root, [
        '*.c', '*.h', '*.inc', '*.m4', '*.w32',
        'composer.json', 'README.md', 'LICENSE'
    ])

    # Copy third_party/utf8_range dependencies
    if os.path.exists(utf8_range_src_dir):
        utf8_range_dst_dir = os.path.join(dst_root, 'third_party', 'utf8_range')
        copy_files(utf8_range_src_dir, utf8_range_dst_dir, [
            '*.c', '*.h', '*.inc', 'LICENSE', 'README.md'
        ])

    # Copy root LICENSE if not already copied from ext_src_dir
    root_license = os.path.join(src_root, 'LICENSE')
    if os.path.exists(root_license) and not os.path.exists(os.path.join(dst_root, 'LICENSE')):
        shutil.copy2(root_license, os.path.join(dst_root, 'LICENSE'))
        print(f"Copied: {root_license} -> {os.path.join(dst_root, 'LICENSE')}")

    print("Sync completed successfully.")

if __name__ == '__main__':
    main()
