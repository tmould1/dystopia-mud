#!/usr/bin/env python3
"""
fetch_audio.py - Download CC0 sound files for the MCMP starter audio pack.

Reads audio_manifest.json, downloads ZIP packs from Kenney.nl and OpenGameArt,
extracts the specified files, converts them to MP3, and places them in the
audio/ directory tree.

All sources are CC0 (public domain). No attribution required.

Requirements: Python 3, ffmpeg (must be on PATH or in standard install locations).
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
AUDIO_DIR = os.path.join(REPO_ROOT, "audio")
MANIFEST_PATH = os.path.join(SCRIPT_DIR, "audio_manifest.json")


def find_ffmpeg():
    """Locate ffmpeg executable on PATH or in common install locations."""
    # Try PATH first
    try:
        subprocess.run(["ffmpeg", "-version"], capture_output=True, check=True)
        return "ffmpeg"
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass

    # Check common Windows install locations
    candidates = [
        os.path.expandvars(r"%LOCALAPPDATA%\Microsoft\WinGet\Links\ffmpeg.exe"),
        r"C:\ffmpeg\bin\ffmpeg.exe",
        r"C:\tools\ffmpeg\bin\ffmpeg.exe",
    ]
    # Also check winget package directories
    winget_dir = os.path.expandvars(r"%LOCALAPPDATA%\Microsoft\WinGet\Packages")
    if os.path.isdir(winget_dir):
        for entry in os.listdir(winget_dir):
            if "FFmpeg" in entry:
                for root, dirs, files in os.walk(os.path.join(winget_dir, entry)):
                    if "ffmpeg.exe" in files:
                        candidates.append(os.path.join(root, "ffmpeg.exe"))

    for path in candidates:
        if os.path.isfile(path):
            return path

    return None


def convert_ogg_to_mp3(ffmpeg, ogg_path, mp3_path):
    """Convert an OGG file to MP3 using ffmpeg. Returns True on success."""
    try:
        result = subprocess.run(
            [ffmpeg, "-y", "-i", ogg_path, "-codec:a", "libmp3lame", "-qscale:a", "4", mp3_path],
            capture_output=True, text=True
        )
        if result.returncode == 0 and os.path.isfile(mp3_path):
            os.remove(ogg_path)
            return True
        return False
    except Exception:
        return False


def load_manifest():
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def download_pack(url, dest_path):
    """Download a file from url to dest_path, with a simple progress indicator."""
    print(f"  Downloading {os.path.basename(dest_path)}...", end=" ", flush=True)
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "fetch_audio/1.0"})
        with urllib.request.urlopen(req, timeout=60) as resp:
            with open(dest_path, "wb") as out:
                shutil.copyfileobj(resp, out)
        size_kb = os.path.getsize(dest_path) // 1024
        print(f"OK ({size_kb} KB)")
        return True
    except Exception as e:
        print(f"FAILED: {e}")
        return False


def extract_file(zip_path, source_file, dest_path):
    """Extract a single file from a ZIP archive to dest_path."""
    try:
        with zipfile.ZipFile(zip_path, "r") as zf:
            names = zf.namelist()
            # Try exact match first
            if source_file in names:
                data = zf.read(source_file)
            else:
                # Try case-insensitive match
                match = None
                for n in names:
                    if n.lower() == source_file.lower():
                        match = n
                        break
                if match is None:
                    # Try matching just the filename part
                    basename = os.path.basename(source_file)
                    for n in names:
                        if os.path.basename(n).lower() == basename.lower():
                            match = n
                            break
                if match is None:
                    print(f"  WARNING: '{source_file}' not found in {os.path.basename(zip_path)}")
                    return False
                data = zf.read(match)

        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        with open(dest_path, "wb") as f:
            f.write(data)
        return True
    except Exception as e:
        print(f"  ERROR extracting '{source_file}': {e}")
        return False


def main():
    print(f"MCMP Starter Audio Pack - Fetch Script")
    print(f"Audio directory: {AUDIO_DIR}")
    print(f"Manifest: {MANIFEST_PATH}")
    print()

    # Find ffmpeg for OGG -> MP3 conversion
    ffmpeg = find_ffmpeg()
    if ffmpeg is None:
        print("ERROR: ffmpeg not found. Install ffmpeg and ensure it is on PATH.")
        print("  Windows: winget install Gyan.FFmpeg")
        print("  Linux:   sudo apt install ffmpeg")
        return 1
    print(f"Using ffmpeg: {ffmpeg}")
    print()

    manifest = load_manifest()
    packs = manifest["_packs"]
    sounds = manifest["sounds"]

    # Determine which packs we need
    needed_packs = set()
    for info in sounds.values():
        needed_packs.add(info["pack"])

    # Download packs to temp directory
    tmp_dir = tempfile.mkdtemp(prefix="mcmp_audio_")
    print(f"Temp directory: {tmp_dir}")
    print()

    pack_zips = {}
    print("=== Downloading packs ===")
    for pack_name in sorted(needed_packs):
        pack_info = packs[pack_name]
        zip_path = os.path.join(tmp_dir, f"{pack_name}.zip")
        if download_pack(pack_info["url"], zip_path):
            pack_zips[pack_name] = zip_path
        else:
            print(f"  SKIPPING pack '{pack_name}' -- download failed")
    print()

    # Extract sounds (OGG from ZIPs) and convert to MP3
    print("=== Extracting and converting sounds ===")
    ok_count = 0
    fail_count = 0
    for target_path, info in sorted(sounds.items()):
        pack_name = info["pack"]
        source_file = info["source_file"]
        dest = os.path.join(AUDIO_DIR, target_path.replace("/", os.sep))

        if pack_name not in pack_zips:
            print(f"  SKIP {target_path} (pack '{pack_name}' not available)")
            fail_count += 1
            continue

        # Extract OGG to a temp location, then convert to MP3
        ogg_tmp = dest.rsplit(".mp3", 1)[0] + ".ogg.tmp"
        if not extract_file(pack_zips[pack_name], source_file, ogg_tmp):
            fail_count += 1
            continue

        os.makedirs(os.path.dirname(dest), exist_ok=True)
        if convert_ogg_to_mp3(ffmpeg, ogg_tmp, dest):
            size_kb = os.path.getsize(dest) / 1024
            print(f"  OK  {target_path} ({size_kb:.1f} KB)")
            ok_count += 1
        else:
            # Clean up failed temp file
            if os.path.isfile(ogg_tmp):
                os.remove(ogg_tmp)
            print(f"  FAILED converting {target_path}")
            fail_count += 1

    print()

    # Clean up temp directory
    shutil.rmtree(tmp_dir, ignore_errors=True)

    # Summary
    total = len(sounds)
    print(f"=== Summary ===")
    print(f"  {ok_count}/{total} sounds extracted successfully")
    if fail_count > 0:
        print(f"  {fail_count} failures")
    print()

    # Verify all expected files exist
    print("=== Verification ===")
    missing = []
    for target_path in sorted(sounds.keys()):
        full_path = os.path.join(AUDIO_DIR, target_path.replace("/", os.sep))
        if not os.path.isfile(full_path):
            missing.append(target_path)
            print(f"  MISSING: {target_path}")

    if not missing:
        print(f"  All {total} audio files present.")
    else:
        print(f"  {len(missing)} files missing!")

    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
