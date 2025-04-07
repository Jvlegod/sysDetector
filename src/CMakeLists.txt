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

cmake_minimum_required(VERSION 3.14)

project(sysDetector)
set(CMAKE_C_STANDARD 11)

# include
include(GNUInstallDirs)

# Build rust flags
# list(APPEND PROJECT_RUST_FLAGS
#     --cfg unsound_local_offset
#     -C relocation_model=pic
#     -D warnings
#     -C link-arg=-s
#     -C overflow_checks
#     -W rust_2021_incompatible_closure_captures
# )

# Build rust executables
set(RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/rust")
set(RUST_OUTPUT_DIR "${RUST_TARGET_DIR}/release")
foreach(FLAG IN LISTS PROJECT_RUST_FLAGS)
     set(RUST_FLAGS "${RUST_FLAGS} ${FLAG}")
endforeach()

add_custom_target(build_sysDetector_server ALL
    COMMENT "Building sysDetector-server executables..."
    COMMAND cargo build
        --release --target-dir "${RUST_TARGET_DIR}"
        --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-server/Cargo.toml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-server
)
add_custom_target(build_sysDetector_cli ALL
    COMMENT "Building sysDetector-cli executables..."
    COMMAND cargo build
        --release --target-dir "${RUST_TARGET_DIR}"
        --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-cli/Cargo.toml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-cli
)

# Install dir
set(SYSDETECTOR_ETC_DIR "/etc/sysDetector")
set(SYSDETECTOR_BINARY_DIR  "${CMAKE_INSTALL_FULL_BINDIR}")
set(SYSDETECTOR_LIBEXEC_DIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/sysDetector")
set(SYSDETECTOR_SERVICE_DIR "${CMAKE_INSTALL_PREFIX}/lib/systemd/system")

message("----------------------------------------------------")
message("
 ▗▄▄▖▗▖  ▗▖▗▄▄▖▗▄▄▄ ▗▄▄▄▖▗▄▄▄▖▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖▗▄▖ ▗▄▄▖ 
▐▌    ▝▚▞▘▐▌   ▐▌  █▐▌     █  ▐▌   ▐▌     █ ▐▌ ▐▌▐▌ ▐▌
 ▝▀▚▖  ▐▌  ▝▀▚▖▐▌  █▐▛▀▀▘  █  ▐▛▀▀▘▐▌     █ ▐▌ ▐▌▐▛▀▚▖
▗▄▄▞▘  ▐▌ ▗▄▄▞▘▐▙▄▄▀▐▙▄▄▖  █  ▐▙▄▄▖▝▚▄▄▖  █ ▝▚▄▞▘▐▌ ▐▌                                                    
")
message("----------------------------------------------------")
# message("-- Rust flags: ${PROJECT_RUST_FLAGS}")
message("-- Binary  directory: ${SYSDETECTOR_BINARY_DIR}")
message("-- Libexec directory: ${SYSDETECTOR_LIBEXEC_DIR}")
message("-- Service directory: ${SYSDETECTOR_SERVICE_DIR}")
message("----------------------------------------------------")

install(
    PROGRAMS
        ${RUST_OUTPUT_DIR}/sysDetector-cli
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    DESTINATION
    ${SYSDETECTOR_BINARY_DIR}
)

install (
    PROGRAMS
        ${RUST_OUTPUT_DIR}/sysDetector-server
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    DESTINATION
    ${SYSDETECTOR_LIBEXEC_DIR}
)

# TODO: change eBPF program from makefile to cmake
# compile and install eBPF program
add_custom_target(build_sysDetector_ebpf ALL
    COMMENT "Building sysDetector-ebpf using Makefile..."
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-ebpf
    DEPENDS build_sysDetector_server build_sysDetector_cli
)

add_custom_command(TARGET build_sysDetector_ebpf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSDETECTOR_LIBEXEC_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSDETECTOR_ETC_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSDETECTOR_ETC_DIR}/proc
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSDETECTOR_ETC_DIR}/fs
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-ebpf/proc ${SYSDETECTOR_LIBEXEC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/sysDetector-ebpf/configs ${SYSDETECTOR_ETC_DIR}
    COMMENT "Copying sysDetector-ebpf executables to ${SYSDETECTOR_LIBEXEC_DIR}"
)

# add sub dir
# add_subdirectory(sysDetector-ebpf)
add_subdirectory(misc)