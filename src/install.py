import os
import subprocess

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
src_directory = os.getcwd()

try:
    tmp_directory = os.path.join(src_directory, 'tmp')
    os.makedirs(tmp_directory, exist_ok=True)
    os.chdir(tmp_directory)

    print("Running cmake...")
    subprocess.run(['sudo', 'cmake', '..'], check=True)

    print("Running make...")
    subprocess.run(['sudo', 'make'], check=True)

    print("Running make install...")
    subprocess.run(['sudo', 'make', 'install'], check=True)

    print("Installation completed successfully.")
except subprocess.CalledProcessError as e:
    print(f"An error occurred during installation: {e}")
except OSError as e:
    print(f"An error occurred while creating or changing directory: {e}")    