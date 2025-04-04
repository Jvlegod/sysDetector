#ifndef __PROC_H
#define __PROC_H

typedef unsigned int __u32;

typedef long long unsigned int __u64;

#define TASK_NAME_MAX 256
#define TASK_COMM_LEN 16

#define PERF_MAX_STACK_DEPTH 100

enum event_type {
    EVENT_EXEC,
    EVENT_EXIT,
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