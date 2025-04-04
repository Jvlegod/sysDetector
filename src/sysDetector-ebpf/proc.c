#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <mqueue.h>
#include <sys/stat.h>
#include "proc.h"
#include "proc.skel.h"

#define COMMAND_QUEUE  "/ebpf_command_queue"
#define RESPONSE_QUEUE "/ebpf_response_queue"
#define MAX_MSG_SIZE   1024
#define QUEUE_TIMEOUT  100  // milliseconds
#define LOG_DIR_PATH "/var/log/sysDetector"
#define LOG_FILE_NAME LOG_DIR_PATH"/sysDetector_proc.log"

#define PROC_START "start"
#define PROC_STOP "stop"

bool ebpf_exiting = false;
bool ebpf_running = false;
static mqd_t resp_mq;
static mqd_t cmd_mq;
FILE *log_file;

typedef enum {
    CMD_SUCCESS = 0,
    CMD_INVALID = -1,
    CMD_EBPF_ERR = -2
} ResponseCode;

static void sig_handler(int sig) {
    ebpf_exiting = true;
}

static void proc_event_exec(struct proc_event *e)
{
    fprintf(log_file, "PID:%d PPID:%d COMM:%-16s FILE:%s STACK_ID:0x%x\n", 
            e->pid, e->ppid, e->comm, e->filename, e->stack_id);
    fflush(log_file);
}

static void proc_event_exit(struct proc_event *e)
{
    fprintf(log_file, "PID:%d PPID:%d COMM:%-16s STACK_ID:0x%x\n", 
            e->pid, e->ppid, e->comm, e->stack_id);
    fflush(log_file);
}

static int handle_event(void *ctx, void *data, size_t sz) {
    if (ebpf_running) {
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
    }
    
handle_ret:
    return 0;
}

static void send_response(ResponseCode code) {
    char response[16];
    snprintf(response, sizeof(response), "%d", code);

    if (mq_send(resp_mq, response, strlen(response), 0) == -1) {
        fprintf(log_file, "mq_send response: %s\n", strerror(errno));
        fflush(log_file);
    }
}

static void handle_command(const char *command) {
    if (!command) {
        fprintf(log_file, "Invalid command format\n");
        fflush(log_file);
        send_response(CMD_INVALID);
        return;
    }

    if (strncmp(command, PROC_START, 5) == 0) {
        if (!ebpf_running) {
            fprintf(log_file, "Starting monitoring Proc\n");
            fflush(log_file);
            ebpf_running = true;
            send_response(CMD_SUCCESS);
        } else {
            fprintf(log_file, "eBPF service is already running\n");
            fflush(log_file);
            send_response(CMD_EBPF_ERR);
        }
        
    } else if (strncmp(command, PROC_STOP, 4) == 0) {
        if (ebpf_running) {
            fprintf(log_file, "Stopping monitoring Proc\n");
            fflush(log_file);
            ebpf_running = false;
            send_response(CMD_SUCCESS);
        } else {
            fprintf(log_file, "eBPF service is not running\n");
            fflush(log_file);
            send_response(CMD_EBPF_ERR);
        }
        
    } else {
        fprintf(log_file, "Unknown command: %s\n", command);
        fflush(log_file);
        send_response(CMD_INVALID);
    }
}

int main(int argc, char **argv) {
    struct proc_bpf *skel = NULL;
    struct ring_buffer *rb = NULL;
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MAX_MSG_SIZE,
        .mq_curmsgs = 0
    };

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (mkdir(LOG_DIR_PATH, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create log directory");
            return -1;
        }
    }

    log_file = fopen(LOG_FILE_NAME, "a");
    if (!log_file) {
        perror("Failed to open or create log file");
        return EXIT_FAILURE;
    }

    resp_mq = mq_open(RESPONSE_QUEUE, O_WRONLY | O_CREAT, 0666, &attr);
    if (resp_mq == (mqd_t)-1) {
        fprintf(log_file, "mq_open response: %s\n", strerror(errno));
        fflush(log_file);
        fclose(log_file);
        return EXIT_FAILURE;
    }

    cmd_mq = mq_open(COMMAND_QUEUE, O_RDONLY | O_CREAT | O_NONBLOCK, 0666, &attr);
    if (cmd_mq == (mqd_t)-1) {
        fprintf(log_file, "mq_open command: %s\n", strerror(errno));
        fflush(log_file);
        mq_close(resp_mq);
        fclose(log_file);
        return EXIT_FAILURE;
    }

    skel = proc_bpf__open();
    if (!skel || proc_bpf__load(skel) || proc_bpf__attach(skel)) {
        fprintf(log_file, "eBPF setup failed\n");
        fflush(log_file);
        goto cleanup;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.proc_events), handle_event, NULL, NULL);
    if (!rb) {
        fprintf(log_file, "Failed to create ring buffer\n");
        fflush(log_file);
        goto cleanup;
    }

    fprintf(log_file, "eBPF monitoring started\n");
    fflush(log_file);

    while (!ebpf_exiting) {
        char buffer[MAX_MSG_SIZE] = {0};
        ssize_t bytes_read;

        bytes_read = mq_receive(cmd_mq, buffer, MAX_MSG_SIZE, NULL);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            handle_command(buffer);
        } else if (errno != EAGAIN) {
            fprintf(log_file, "mq_receive failed with errno %d: %s\n", errno, strerror(errno));
            fflush(log_file);
        }

        ring_buffer__poll(rb, QUEUE_TIMEOUT);
    }

cleanup:
    ring_buffer__free(rb);
    proc_bpf__destroy(skel);
    mq_close(cmd_mq);
    mq_unlink(COMMAND_QUEUE);
    mq_close(resp_mq);
    mq_unlink(RESPONSE_QUEUE);
    fclose(log_file);
    return EXIT_SUCCESS;
}    