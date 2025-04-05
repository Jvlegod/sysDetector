/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Author: Keke Ming
 * Date: 20250405
 */
#ifndef __PROC_H
#define __PROC_H
#include "common.h"

#define TASK_NAME_MAX 256
#define TASK_COMM_LEN 16

#define PERF_MAX_STACK_DEPTH 100

#define LOG_FILE_NAME LOG_DIR_PATH"/proc.log"
#define OUT_FILE_NAME LOG_DIR_PATH"/out.log"
#define PROC_CONFIG_DIR CONFIG_FILE_PATH"/proc"

#define PROC_START "start"
#define PROC_STOP "stop"
#define PROC_LIST "list"

#define MAX_MSG_SIZE   1024
#define QUEUE_TIMEOUT  100  // milliseconds

static bool ebpf_exiting = false;
static bool ebpf_running = false;
static mqd_t resp_mq;
static mqd_t cmd_mq;
static FILE *log_file;
static FILE *out_file;

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

struct process_config {
    char name[64];
    char user[32];
    char recover_cmd[256];
    char monitor_cmd[256];
    char stop_cmd[256];
    char alarm_cmd[256];
    int monitor_period;
    bool monitor_switch;
    ino_t config_id;
};

struct item_value_func {
    const char *key;
    int (*func)(const char *, void *);
};

static struct process_config *configs = NULL;
static int config_count = 0;

static volatile bool exiting = false;
void print_process_config(struct process_config *config);

#endif // __PROC_H