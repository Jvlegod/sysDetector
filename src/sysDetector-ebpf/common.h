#ifndef __COMMON_H
#define __COMMON_H

typedef unsigned int __u32;
typedef long long unsigned int __u64;
typedef unsigned char __u8;

#define COMMAND_QUEUE  "/ebpf_command_queue"
#define RESPONSE_QUEUE "/ebpf_response_queue"
#define LOG_DIR_PATH "/var/log/sysDetector"
#define CONFIG_FILE_PATH "/etc/sysDetector"
#define MAX_LINE_LENGTH 1024
#define MAX_PATH_LEN 100
#define ITEM_LEN 100

// #define DEBUG
#ifdef DEBUG
#define DEBUG_PROC
#endif

#endif // __COMMON_H