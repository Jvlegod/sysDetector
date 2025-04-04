#ifndef __PROC_H
#define __PROC_H
#include "common.h"

#define TASK_NAME_MAX 256
#define TASK_COMM_LEN 16

#define PERF_MAX_STACK_DEPTH 100

#define LOG_FILE_NAME LOG_DIR_PATH"/sysDetector_proc.log"

#define PROC_START "start"
#define PROC_STOP "stop"

#define MAX_MSG_SIZE   1024
#define QUEUE_TIMEOUT  100  // milliseconds

static bool ebpf_exiting = false;
static bool ebpf_running = false;
static mqd_t resp_mq;
static mqd_t cmd_mq;
static FILE *log_file;

typedef enum {
    CMD_SUCCESS = 0,
    CMD_INVALID = -1,
    CMD_EBPF_ERR = -2
} ResponseCode;

enum event_type {
    EVENT_EXEC,
    EVENT_EXIT,
};

struct task_parse {
    __u8 monitor_switch; // 1: on, 0: off
    __u8 user;
};

struct proc_event {
    enum event_type type;
    __u32 pid;
    __u32 ppid;
    char comm[TASK_COMM_LEN];
    char filename[TASK_NAME_MAX];
    __u64 exit_code;
    __u32 stack_id;
};

static volatile bool exiting = false;

#endif // __PROC_H