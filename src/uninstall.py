#!/usr/bin/env python3
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 
# Author: Keke Ming
# Date: 20250405
"""
System Detector Uninstall Script
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path

# Installation paths to remove
INSTALL_PATHS = [
    Path("/usr/local/bin/sysDetector-cli"),
    Path("/usr/local/libexec/sysDetector"),
    Path("/etc/sysDetector"),
    Path("/usr/local/lib/systemd/system/sysDetector.service"),
    Path("/usr/local/lib/systemd/system/sysDetector-proc.service"),
    Path("/var/run/sysDetector.sock")
]

# Potential empty directories to clean up
POTENTIAL_EMPTY_DIRS = [
    Path("/usr/local/libexec/sysDetector"),
    Path("/usr/local/lib/systemd/system"),  # Delete with caution
    Path("/etc/sysDetector")
]

# service list
SERVICES_TO_STOP = [
    "sysDetector.service",
    "sysDetector-proc.service"
]


def check_root():
    """Verify script is run with root privileges"""
    if os.geteuid() != 0:
        print("Error: This script requires root privileges. Use sudo.")
        sys.exit(1)


def stop_services():
    """Stop all relevant services using systemctl"""
    for service in SERVICES_TO_STOP:
        try:
            subprocess.run(["systemctl", "stop", service], check=True)
            print(f"Successfully stopped {service}")
        except subprocess.CalledProcessError as e:
            print(f"Error stopping {service}: {e}")


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

    print("Stopping System Detector services...")
    stop_services()

    print("\nUninstalling System Detector...\n")

    removed_count = remove_installation_items()
    cleaned_count = clean_empty_directories()

    print("\nUninstallation summary:")
    print(f"• Removed {removed_count} files directories")
    print(f"• Cleaned {cleaned_count} empty directories")

    if removed_count < len(INSTALL_PATHS):
        print("\nWarning: Not all components were successfully removed")
        sys.exit(1)

    print("\nUninstallation completed successfully")


if __name__ == "__main__":
    main()
    