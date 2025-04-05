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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "proc.h"
#include "proc.skel.h"

static void send_response(ResponseCode code) {
    char response[16];
    snprintf(response, sizeof(response), "%d", code);

    if (mq_send(resp_mq, response, strlen(response), 0) == -1) {
        fprintf(log_file, "mq_send response: %s\n", strerror(errno));
        fflush(log_file);
    }
}
static int parse_monitor_period(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    char *endptr;
    long period = strtol(value, &endptr, 10);
    if (*endptr != '\0' || period < 1) {
        return -EINVAL;
    }
    config->monitor_period = (int)period;
    return 0;
}

static int parse_monitor_switch(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    if (strcasecmp(value, "on") == 0) {
        config->monitor_switch = true;
    } else if (strcasecmp(value, "off") == 0) {
        config->monitor_switch = false;
    } else {
        return -EINVAL;
    }
    return 0;
}

static int parse_user(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    snprintf(config->user, sizeof(config->user), "%s", value);
    return 0;
}

static int parse_name(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    snprintf(config->name, sizeof(config->name), "%s", value);
    return 0;
}

static int parse_recover_command(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    snprintf(config->recover_cmd, sizeof(config->recover_cmd), "%s", value);
    return 0;
}

static int parse_monitor_command(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    snprintf(config->monitor_cmd, sizeof(config->monitor_cmd), "%s", value);
    return 0;
}

static int parse_stop_command(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    snprintf(config->stop_cmd, sizeof(config->stop_cmd), "%s", value);
    return 0;
}

static int parse_alarm_command(const char *value, void *data) {
    struct process_config *config = (struct process_config *)data;
    snprintf(config->alarm_cmd, sizeof(config->alarm_cmd), "%s", value);
    return 0;
}

static struct item_value_func g_ps_opt_array[] = {
    { "MONITOR_PERIOD", parse_monitor_period },
    { "MONITOR_SWITCH", parse_monitor_switch },
    { "USER", parse_user },
    { "NAME", parse_name },
    { "RECOVER_COMMAND", parse_recover_command },
    { "MONITOR_COMMAND", parse_monitor_command },
    { "STOP_COMMAND", parse_stop_command },
    { "ALARM_COMMAND", parse_alarm_command },
    { NULL, NULL }
};

static void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

static int parse_config_file(const char *filename, ino_t config_id) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open config file: %s\n", filename);
        return -1;
    }

    struct process_config config = {
        .monitor_period = 3,
        .monitor_switch = false,
        .config_id = config_id
    };

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char *eq_pos = strchr(line, '=');
        if (!eq_pos) continue;

        *eq_pos = '\0';
        char *key = line;
        char *value = eq_pos + 1;

        trim_whitespace(key);
        trim_whitespace(value);

        if (*key == '#' || *key == '\0') continue;

        int i;
        for (i = 0; g_ps_opt_array[i].key; i++) {
            if (strcasecmp(key, g_ps_opt_array[i].key) == 0) {
                if (g_ps_opt_array[i].func(value, &config) != 0) {
                    fprintf(stderr, "Invalid value for %s in %s\n", key, filename);
                    fclose(file);
                    return -1;
                }
                break;
            }
        }
        if (!g_ps_opt_array[i].key) {
            fprintf(stderr, "Unknown config key: %s in %s\n", key, filename);
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    if (strlen(config.name) == 0) {
        fprintf(stderr, "Missing NAME in %s\n", filename);
        return -1;
    }

    if (strlen(config.monitor_cmd) == 0) {
        snprintf(config.monitor_cmd, sizeof(config.monitor_cmd),
                 "pgrep -f $(which %s)", config.name);
    }

    configs = realloc(configs, (config_count + 1) * sizeof(struct process_config));
    if (!configs) {
        perror("realloc");
        return -1;
    }
    configs[config_count++] = config;
    return 0;
}

static int proc_parse_config() {
    if (!opendir(CONFIG_FILE_PATH)) {
        if (mkdir(CONFIG_FILE_PATH, 0755) == -1) {
            perror("mkdir failed");
            return -1;
        }
    }

    DIR *dir = opendir(PROC_CONFIG_DIR);
    if (!dir) {
        if (errno == ENOENT) {
            if (mkdir(PROC_CONFIG_DIR, 0755) == -1) {
                perror("mkdir failed");
                return -1;
            }

            dir = opendir(PROC_CONFIG_DIR);
            if (!dir) {
                perror("opendir after mkdir failed");
                return -1;
            }
        } else {
            perror("opendir failed");
            return -1;
        }
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue; // Jvle: why vscode report error?

        size_t len = strlen(entry->d_name);
        if (len < 5 || strcmp(entry->d_name + len - 5, ".conf") != 0) {
            continue;
        }

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", PROC_CONFIG_DIR, entry->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            perror("stat");
            continue;
        }

        ino_t config_id = statbuf.st_ino;

        if (parse_config_file(path, config_id) != 0) {
            fprintf(stderr, "Error parsing %s\n", path);
        }
    }

    closedir(dir);
    return 0;
}

static void handle_start_command(const char *config_id_str) {
    ino_t config_id = strtoul(config_id_str, NULL, 10);
    for (int i = 0; i < config_count; i++) {
        if (configs[i].config_id == config_id) {
            configs[i].monitor_switch = true;
            fprintf(log_file, "Started monitoring for config ID: %lu\n", (unsigned long)config_id);
            fflush(log_file);
            send_response(CMD_SUCCESS);
            return;
        }
    }
    fprintf(log_file, "Config ID not found: %s\n", config_id_str);
    fflush(log_file);
    send_response(CMD_INVALID);
}

static void handle_list_printf(struct process_config *config, int id) {
    fprintf(out_file, "[%d]\t%lu\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%s\n", 
            id, (unsigned long)config->config_id, config->name,
            config->user, config->recover_cmd, config->monitor_cmd,
            config->stop_cmd, config->alarm_cmd, config->monitor_period,
            config->monitor_switch ? "On" : "Off");
}
static void handle_list_command() {
    out_file = fopen(OUT_FILE_NAME, "w");
    fprintf(out_file, "Num\tID\tName\tUser\tRecover\tMonitor\tStop\tAlarm\tPeriod\tSwitch\t\n");
    for (int i = 0; i < config_count; i++) {
        handle_list_printf(&configs[i], i);
    }
    fclose(out_file);
}

void print_process_config(struct process_config *config) {
    printf("Config ID: %lu\n", (unsigned long)config->config_id);
    printf("Name: %s\n", config->name);
    printf("User: %s\n", config->user);
    printf("Recover Command: %s\n", config->recover_cmd);
    printf("Monitor Command: %s\n", config->monitor_cmd);
    printf("Stop Command: %s\n", config->stop_cmd);
    printf("Alarm Command: %s\n", config->alarm_cmd);
    printf("Monitor Period: %d\n", config->monitor_period);
    printf("Monitor Switch: %s\n", config->monitor_switch ? "On" : "Off");
    printf("\n");
    fflush(stdout);
}
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

static void handle_command(const char *command) {
    if (!command) {
        fprintf(log_file, "Invalid command format\n");
        fflush(log_file);
        send_response(CMD_INVALID);
        return;
    }


    if (strncmp(command, PROC_LIST, 4) == 0) {
        handle_list_command();
        send_response(CMD_SUCCESS);
    } else if (strncmp(command, PROC_START, 5) == 0) {
        if (!ebpf_running) {
            fprintf(log_file, "Starting monitoring Proc\n");
            fflush(log_file);
            // handle_start_command(command + 6);
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

static int proc_init_log_and_out() {
    if (mkdir(LOG_DIR_PATH, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create log directory");
            return EXIT_FAILURE;
        }
    }

    out_file = fopen(OUT_FILE_NAME, "a");
    if (!out_file) {
        perror("Failed to open or create proc out file");
        return EXIT_FAILURE;
    }
    fclose(out_file);

    log_file = fopen(LOG_FILE_NAME, "a");
    if (!log_file) {
        perror("Failed to open or create proc log file");
        return EXIT_FAILURE;
    }
    return 0;
}

static int proc_init_mq() {
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MAX_MSG_SIZE,
        .mq_curmsgs = 0
    };

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

    return 0;
}

int main(int argc, char **argv) {
    struct proc_bpf *skel = NULL;
    struct ring_buffer *rb = NULL;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (proc_init_log_and_out() != 0) {
        return EXIT_FAILURE;
    }

    if (proc_init_mq() != 0 ) {
        return EXIT_FAILURE;
    }

    if (proc_parse_config()  != 0) {
        return EXIT_FAILURE;
    }

#ifdef DEBUG_PROC
        for (int i = 0; i < config_count; i++) {
            print_process_config(&configs[i]);
        }
#endif

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