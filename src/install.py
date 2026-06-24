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

import os
import shutil
import subprocess
import sys
from pathlib import Path


def run_command(command, cwd, env=None):
    print(f"Running: {' '.join(command)}")
    subprocess.run(command, cwd=cwd, env=env, check=True)


def find_clang():
    configured_clang = os.environ.get("CLANG")
    if configured_clang:
        return configured_clang

    for candidate in ["clang", "clang-18", "clang-17", "clang-16"]:
        clang = shutil.which(candidate)
        if clang:
            return clang

    return None


def build_environment():
    env = os.environ.copy()
    clang = find_clang()
    if clang:
        env["CLANG"] = clang
    return env


def main():
    src_directory = Path(__file__).resolve().parent
    build_directory = src_directory / "tmp"
    build_directory.mkdir(exist_ok=True)

    # The script is commonly run as `sudo python3 install.py`; avoid nesting sudo.
    use_sudo = os.geteuid() != 0
    install_command = ["cmake", "--install", "."]
    if use_sudo:
        install_command.insert(0, "sudo")

    build_env = build_environment()

    run_command(["cmake", ".."], build_directory, env=build_env)
    run_command(["cmake", "--build", "."], build_directory, env=build_env)
    run_command(install_command, build_directory, env=build_env)

    print("Installation completed successfully.")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as err:
        print(f"An error occurred during installation: {err}", file=sys.stderr)
        sys.exit(err.returncode)
    except OSError as err:
        print(f"An error occurred while preparing installation: {err}", file=sys.stderr)
        sys.exit(1)
