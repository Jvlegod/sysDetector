#!/usr/bin/env python3
"""
System Detector Uninstall Script
"""

import os
import sys
import shutil
from pathlib import Path

# Installation paths to remove
INSTALL_PATHS = [
    Path("/usr/local/bin/sysDetector-cli"),
    Path("/usr/local/libexec/sysDetector/sysDetector-server"),
    Path("/usr/local/lib/systemd/system/sysDetector.service"),
]

# Potential empty directories to clean up
POTENTIAL_EMPTY_DIRS = [
    Path("/usr/local/libexec/sysDetector"),
    Path("/usr/local/lib/systemd/system") # Delete with caution
]

def check_root():
    """Verify script is run with root privileges"""
    if os.geteuid() != 0:
        print("Error: This script requires root privileges. Use sudo.")
        sys.exit(1)

def remove_installation_items():
    """Remove installed files and directories"""
    removed_items = 0
    
    for path in INSTALL_PATHS:
        if path.exists():
            try:
                if path.is_file() or path.is_symlink():
                    path.unlink()
                    print(f"Removed: {path}")
                    removed_items += 1
                elif path.is_dir():
                    shutil.rmtree(path)
                    print(f"Removed directory: {path}")
                    removed_items += 1
            except Exception as e:
                print(f"Error removing {path}: {str(e)}")
        else:
            print(f"Not found: {path} (skipping)")

    return removed_items

def clean_empty_directories():
    """Remove empty parent directories"""
    cleaned_dirs = 0
    
    for dir_path in POTENTIAL_EMPTY_DIRS:
        try:
            if dir_path.exists() and dir_path.is_dir():
                if not any(dir_path.iterdir()):
                    dir_path.rmdir()
                    print(f"Cleaned empty directory: {dir_path}")
                    cleaned_dirs += 1
        except Exception as e:
            print(f"Error cleaning directory {dir_path}: {str(e)}")

    return cleaned_dirs

def main():
    check_root()
    
    print("Uninstalling System Detector...\n")
    
    removed_count = remove_installation_items()
    cleaned_count = clean_empty_directories()
    
    print("\nUninstallation summary:")
    print(f"• Removed {removed_count} files/directories")
    print(f"• Cleaned {cleaned_count} empty directories")
    
    if removed_count < len(INSTALL_PATHS):
        print("\nWarning: Not all components were successfully removed")
        sys.exit(1)
        
    print("\nUninstallation completed successfully")

if __name__ == "__main__":
    main()