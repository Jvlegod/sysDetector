#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include "proc.h"
#include "proc.skel.h"
static void sig_handler(int sig) {
    exiting = true;
}

static void proc_event_exec(struct proc_event *e)
{
    printf("PID:%d PPID:%d COMM:%-16s FILE:%s STACK_ID:%u\n", 
           e->pid, e->ppid, e->comm, e->filename, e->stack_id);
}

static void proc_event_exit(struct proc_event *e)
{
    printf("PID:%d PPID:%d COMM:%-16s STACK_ID:%u\n", 
           e->pid, e->ppid, e->comm, e->stack_id);
}
static int handle_event(void *ctx, void *data, size_t sz) {
    struct proc_event *e = data;
    switch (e->type)
    {
    case EVENT_EXEC:
        proc_event_exec(e);
        goto handle_ret;
    case EVENT_EXIT:
        proc_event_exit(e);
        goto handle_ret;
    default:
        return -1;
    }
    
handle_ret:
    return 0;
}

int main(int argc, char **argv) {
    struct proc_bpf *skel = NULL;
    struct ring_buffer *rb = NULL;
    int err;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    skel = proc_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    err = proc_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF skeleton\n");
        goto cleanup;
    }

    err = proc_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton\n");
        goto cleanup;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.proc_events), handle_event, NULL, NULL);
    if (!rb) {
        err = -errno;
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }

    printf("Monitoring process events...\n");
    while (!exiting) {
        /* timeout(ms) */
        ring_buffer__poll(rb, 100);
    }

cleanup:
    ring_buffer__free(rb);
    proc_bpf__destroy(skel);
    return -err;
}