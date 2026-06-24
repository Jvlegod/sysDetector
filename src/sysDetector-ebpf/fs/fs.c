#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define FS_CONFIG_DIR "/etc/sysDetector/fs"
#define LOG_DIR_PATH "/var/log/sysDetector"
#define FS_LOG_FILE LOG_DIR_PATH "/fs.log"
#define OUT_FILE_NAME LOG_DIR_PATH "/out.log"
#define COMMAND_QUEUE "/fs_command_queue"
#define RESPONSE_QUEUE "/fs_response_queue"
#define MAX_CONFIGS 128
#define MAX_EVENTS 128
#define MAX_MSG_SIZE 1024
#define MAX_FIELD_LEN 256
#define EVENT_BUF_LEN (32 * (sizeof(struct inotify_event) + MAX_FIELD_LEN))

struct fs_config {
    int id;
    bool monitor_switch;
    char name[MAX_FIELD_LEN];
    char path[MAX_FIELD_LEN];
    char operations[MAX_FIELD_LEN];
    char alarm_cmd[MAX_FIELD_LEN];
};

struct fs_event_record {
    time_t ts;
    int config_id;
    char name[MAX_FIELD_LEN];
    char op[64];
    char path[MAX_FIELD_LEN];
};

static struct fs_config configs[MAX_CONFIGS];
static int config_count;
static struct fs_event_record events[MAX_EVENTS];
static int event_count;
static int event_cursor;

static void ensure_dirs(void) {
    mkdir(LOG_DIR_PATH, 0755);
    mkdir(FS_CONFIG_DIR, 0755);
}

static void trim(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) end--;
    *end = '\0';
}

static void log_line(const char *fmt, ...) {
    FILE *fp = fopen(FS_LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    char ts[64];
    strftime(ts, sizeof(ts), "%a %b %d %H:%M:%S %Y", localtime(&now));
    fprintf(fp, "[%s] ", ts);

    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fprintf(fp, "\n");
    fclose(fp);
}

static void send_response(int code) {
    mqd_t mq = mq_open(RESPONSE_QUEUE, O_WRONLY);
    if (mq == (mqd_t)-1) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", code);
    mq_send(mq, buf, strlen(buf), 0);
    mq_close(mq);
}

static void parse_config_file(const char *path, int id) {
    if (config_count >= MAX_CONFIGS) return;
    FILE *fp = fopen(path, "r");
    if (!fp) return;

    struct fs_config *cfg = &configs[config_count];
    memset(cfg, 0, sizeof(*cfg));
    cfg->id = id;
    snprintf(cfg->operations, sizeof(cfg->operations), "write,delete,rename,chmod");

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (!line[0] || line[0] == '#') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        trim(key);
        trim(value);

        if (strcmp(key, "NAME") == 0) snprintf(cfg->name, sizeof(cfg->name), "%s", value);
        else if (strcmp(key, "PATH") == 0) snprintf(cfg->path, sizeof(cfg->path), "%s", value);
        else if (strcmp(key, "OPERATIONS") == 0) snprintf(cfg->operations, sizeof(cfg->operations), "%s", value);
        else if (strcmp(key, "ALARM_COMMAND") == 0) snprintf(cfg->alarm_cmd, sizeof(cfg->alarm_cmd), "%s", value);
        else if (strcmp(key, "MONITOR_SWITCH") == 0) cfg->monitor_switch = strcasecmp(value, "on") == 0;
    }

    fclose(fp);
    if (cfg->name[0] && cfg->path[0]) config_count++;
}

static void load_configs(void) {
    config_count = 0;
    DIR *dir = opendir(FS_CONFIG_DIR);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", FS_CONFIG_DIR, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) parse_config_file(path, (int)st.st_ino);
    }
    closedir(dir);
}

static struct fs_config *find_config(int id) {
    for (int i = 0; i < config_count; i++) if (configs[i].id == id) return &configs[i];
    return NULL;
}

static void write_config_file(struct fs_config *cfg) {
    DIR *dir = opendir(FS_CONFIG_DIR);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", FS_CONFIG_DIR, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && (int)st.st_ino == cfg->id) {
            FILE *fp = fopen(path, "w");
            if (fp) {
                fprintf(fp, "MONITOR_SWITCH=%s\nNAME=%s\nPATH=%s\nOPERATIONS=%s\nALARM_COMMAND=%s\n",
                        cfg->monitor_switch ? "on" : "off", cfg->name, cfg->path,
                        cfg->operations, cfg->alarm_cmd[0] ? cfg->alarm_cmd : "true");
                fclose(fp);
            }
            break;
        }
    }
    closedir(dir);
}

static void list_configs(void) {
    FILE *fp = fopen(OUT_FILE_NAME, "w");
    if (!fp) return;
    fprintf(fp, "Num\tID\tName\tPath\tOperations\tSwitch\n");
    for (int i = 0; i < config_count; i++) {
        fprintf(fp, "[%d]\t%d\t%s\t%s\t%s\t%s\n", i, configs[i].id, configs[i].name,
                configs[i].path, configs[i].operations, configs[i].monitor_switch ? "On" : "Off");
    }
    fclose(fp);
}

static void list_events(void) {
    FILE *fp = fopen(OUT_FILE_NAME, "w");
    if (!fp) return;
    fprintf(fp, "Time\tID\tName\tOperation\tPath\n");
    for (int i = 0; i < event_count; i++) {
        int idx = (event_cursor + i) % MAX_EVENTS;
        char ts[64];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&events[idx].ts));
        fprintf(fp, "%s\t%d\t%s\t%s\t%s\n", ts, events[idx].config_id, events[idx].name,
                events[idx].op, events[idx].path);
    }
    fclose(fp);
}

static void record_event(struct fs_config *cfg, const char *op, const char *path) {
    int idx = (event_cursor + event_count) % MAX_EVENTS;
    if (event_count == MAX_EVENTS) {
        idx = event_cursor;
        event_cursor = (event_cursor + 1) % MAX_EVENTS;
    } else {
        event_count++;
    }

    events[idx].ts = time(NULL);
    events[idx].config_id = cfg->id;
    snprintf(events[idx].name, sizeof(events[idx].name), "%s", cfg->name);
    snprintf(events[idx].op, sizeof(events[idx].op), "%s", op);
    snprintf(events[idx].path, sizeof(events[idx].path), "%s", path);
    log_line("FS EVENT ID:%d NAME:%s OP:%s PATH:%s", cfg->id, cfg->name, op, path);

    if (cfg->alarm_cmd[0] && strcmp(cfg->alarm_cmd, "true") != 0) system(cfg->alarm_cmd);
}

static const char *mask_to_op(uint32_t mask) {
    if (mask & (IN_CREATE | IN_CLOSE_WRITE | IN_MODIFY)) return "write";
    if (mask & (IN_DELETE | IN_DELETE_SELF)) return "delete";
    if (mask & (IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF)) return "rename";
    if (mask & IN_ATTRIB) return "chmod";
    return "event";
}

static void monitor_once(void) {
    int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd < 0) return;

    int watch_config[MAX_CONFIGS];
    int watch_count = 0;
    for (int i = 0; i < config_count; i++) {
        if (!configs[i].monitor_switch) continue;
        int wd = inotify_add_watch(fd, configs[i].path,
            IN_CREATE | IN_CLOSE_WRITE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF |
            IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF | IN_ATTRIB);
        if (wd >= 0 && watch_count < MAX_CONFIGS) watch_config[watch_count++] = (wd << 16) | i;
    }

    char buf[EVENT_BUF_LEN];
    ssize_t len = read(fd, buf, sizeof(buf));
    if (len > 0) {
        for (char *ptr = buf; ptr < buf + len;) {
            struct inotify_event *ev = (struct inotify_event *)ptr;
            for (int i = 0; i < watch_count; i++) {
                if ((watch_config[i] >> 16) == ev->wd) {
                    struct fs_config *cfg = &configs[watch_config[i] & 0xffff];
                    char path[512];
                    snprintf(path, sizeof(path), "%s%s%s", cfg->path, ev->len ? "/" : "", ev->len ? ev->name : "");
                    record_event(cfg, mask_to_op(ev->mask), path);
                    break;
                }
            }
            ptr += sizeof(struct inotify_event) + ev->len;
        }
    }
    close(fd);
}

static void handle_command(char *cmd) {
    char *parts[3] = {0};
    int count = 0;
    for (char *tok = strtok(cmd, " "); tok && count < 3; tok = strtok(NULL, " ")) parts[count++] = tok;
    if (count < 1) { send_response(1); return; }

    load_configs();
    if (strcmp(parts[0], "start") == 0 && count >= 2) {
        struct fs_config *cfg = find_config(atoi(parts[1]));
        if (!cfg) { send_response(1); return; }
        cfg->monitor_switch = true;
        write_config_file(cfg);
        log_line("Started fs monitoring for config ID:%d", cfg->id);
        send_response(0);
    } else if (strcmp(parts[0], "stop") == 0 && count >= 2) {
        struct fs_config *cfg = find_config(atoi(parts[1]));
        if (!cfg) { send_response(1); return; }
        cfg->monitor_switch = false;
        write_config_file(cfg);
        log_line("Stopped fs monitoring for config ID:%d", cfg->id);
        send_response(0);
    } else if (strcmp(parts[0], "list") == 0) {
        if (count < 2 || strcmp(parts[1], "config") == 0) list_configs();
        else if (strcmp(parts[1], "events") == 0) list_events();
        else { send_response(1); return; }
        send_response(0);
    } else {
        send_response(1);
    }
}

int main(void) {
    ensure_dirs();
    load_configs();
    log_line("Starting monitoring FS");

    struct mq_attr attr = {0, 10, MAX_MSG_SIZE, 0};
    mqd_t cmdq = mq_open(COMMAND_QUEUE, O_CREAT | O_RDONLY | O_NONBLOCK, 0600, &attr);
    if (cmdq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    while (1) {
        char msg[MAX_MSG_SIZE] = {0};
        ssize_t len = mq_receive(cmdq, msg, sizeof(msg), NULL);
        if (len > 0) handle_command(msg);
        load_configs();
        monitor_once();
        usleep(200000);
    }
}
