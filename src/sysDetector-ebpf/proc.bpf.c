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
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "proc.h"

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * sizeof(struct proc_event));
} proc_events SEC(".maps");

struct exec_args {
    char argv[ARGV_MAX_ARGS][ARGV_ARG_LEN];
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u32);
    __type(value, struct exec_args);
} exec_args_cache SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_STACK_TRACE);
    __uint(key_size, sizeof(u32));
    __uint(value_size, PERF_MAX_STACK_DEPTH * sizeof(u64));
    __uint(max_entries, 1024);
} stack_traces SEC(".maps");

static void get_exec_info(struct trace_event_raw_sched_process_exec *ctx,
                         struct proc_event *e)
{
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    __u32 pid = bpf_get_current_pid_tgid() >> 32;

    e->type = EVENT_EXEC;
    e->pid = pid;
    e->ppid = BPF_CORE_READ(task, real_parent, tgid);
    bpf_get_current_comm(e->comm, sizeof(e->comm));

    u16 filename_off = ctx->__data_loc_filename & 0xFFFF;
    u16 filename_len = ctx->__data_loc_filename >> 16;

    long ret = bpf_probe_read_kernel_str(e->filename, sizeof(e->filename), 
                                    (void *)ctx + filename_off);
    
    if (ret < 0 || ret > filename_len) {
        __builtin_memset(e->filename, 0, sizeof(e->filename));
        bpf_printk("Filename read error: %d", ret);
    }

    __builtin_memset(e->argv, 0, sizeof(e->argv));
    struct exec_args *args = bpf_map_lookup_elem(&exec_args_cache, &pid);
    if (args) {
        __builtin_memcpy(e->argv, args->argv, sizeof(e->argv));
        bpf_map_delete_elem(&exec_args_cache, &pid);
    }

    e->stack_id = bpf_get_stackid(ctx, &stack_traces, BPF_F_USER_STACK);
}

static void get_exit_info(struct trace_event_raw_sched_process_exit *ctx,
                         struct proc_event *e)
{
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();

    e->type = EVENT_EXIT;
    e->pid = bpf_get_current_pid_tgid() >> 32;
    e->ppid = BPF_CORE_READ(task, real_parent, tgid);
    e->exit_code = BPF_CORE_READ(task, exit_code);
    bpf_get_current_comm(e->comm, sizeof(e->comm));

    __builtin_memset(e->filename, 0, sizeof(e->filename));
    __builtin_memset(e->argv, 0, sizeof(e->argv));

    e->stack_id = bpf_get_stackid(ctx, &stack_traces, BPF_F_USER_STACK);
}

SEC("tp/syscalls/sys_enter_execve")
int handle_execve(struct trace_event_raw_sys_enter *ctx)
{
    __u32 pid = bpf_get_current_pid_tgid() >> 32;
    const char **argv = (const char **)ctx->args[1];
    struct exec_args args = {};

#pragma unroll
    for (int i = 0; i < ARGV_MAX_ARGS; i++) {
        const char *argp = NULL;
        bpf_probe_read_user(&argp, sizeof(argp), &argv[i]);
        if (argp) {
            bpf_probe_read_user_str(args.argv[i], sizeof(args.argv[i]), argp);
        }
    }

    bpf_map_update_elem(&exec_args_cache, &pid, &args, BPF_ANY);
    return 0;
}

SEC("tp/sched/sched_process_exec")
int handle_exec(struct trace_event_raw_sched_process_exec *ctx)
{
    struct proc_event *e = bpf_ringbuf_reserve(&proc_events, sizeof(*e), 0);
    if (!e) return 0;

    get_exec_info(ctx, e);
    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("tp/sched/sched_process_exit")
int handle_exit(struct trace_event_raw_sched_process_exit *ctx)
{
    struct proc_event *e = bpf_ringbuf_reserve(&proc_events, sizeof(*e), 0);
    if (!e) return 0;

    get_exit_info(ctx, e);
    bpf_ringbuf_submit(e, 0);
    return 0;
}

char _license[] SEC("license") = "GPL";