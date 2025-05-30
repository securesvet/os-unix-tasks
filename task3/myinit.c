#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <stdarg.h>

#define MAX_PROC 10
#define MAX_CONF_NAME 4096
#define LOG_PATH "/tmp/myinit.log"
#define PIDFILE_PATH "/tmp/myinit.pid"

typedef struct {
    int args_count;
    char **args;
    char *in;
    char *out;
} TaskInfo;

pid_t pids[MAX_PROC] = {0};
int pids_count = 0;
TaskInfo tasks[MAX_PROC];
FILE *log_file = NULL;
char conf_name[MAX_CONF_NAME];

void write_log(const char *format, ...) {
    if (!log_file) return;

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    fflush(log_file);
    va_end(args);
}

void open_log() {
    log_file = fopen(LOG_PATH, "w");
    if (!log_file) {
        perror("fopen log");
        exit(EXIT_FAILURE);
    }
    write_log("myinit started");
}

void write_pidfile() {
    FILE *pid_file = fopen(PIDFILE_PATH, "w");
    if (pid_file) {
        fprintf(pid_file, "%d\n", getpid());
        fclose(pid_file);
    }
}

void close_all_fds() {
    struct rlimit limit;
    getrlimit(RLIMIT_NOFILE, &limit);
    for (int fd = 0; fd < (int)limit.rlim_max; ++fd) {
        close(fd);
    }
}

void check_absolute_path(const char *path) {
    if (path[0] != '/') {
        write_log("Путь должен быть абсолютным: %s", path);
        exit(EXIT_FAILURE);
    }
}

TaskInfo parse_task(char *line) {
    TaskInfo task = {0};
    char *token;
    int count = 0;
    char **args = malloc(sizeof(char *) * 64);

    token = strtok(line, " ");
    while (token != NULL) {
        args[count] = strdup(token);
        count++;
        token = strtok(NULL, " ");
    }

    if (count < 3) {
        write_log("Недостаточно аргументов в конфигурации");
        exit(EXIT_FAILURE);
    }

    args[count - 1][strcspn(args[count - 1], "\n")] = '\0';

    task.in = strdup(args[count - 2]);
    task.out = strdup(args[count - 1]);
    check_absolute_path(task.in);
    check_absolute_path(task.out);

    args[count - 2] = NULL;
    args[count - 1] = NULL;
    task.args = args;
    task.args_count = count - 2;
    return task;
}

void redirect_io(const TaskInfo *task) {
    freopen(task->in, "r", stdin);
    freopen(task->out, "w", stdout);
}

void start_task(int index) {
    TaskInfo task = tasks[index];
    pid_t pid = fork();

    if (pid < 0) {
        write_log("Ошибка fork для задачи %d", index);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        redirect_io(&task);
        execvp(task.args[0], task.args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        pids[index] = pid;
        pids_count++;
        write_log("Задача %d запущена: %s [pid=%d]", index, task.args[0], pid);
    }
}

void load_config_and_start() {
    FILE *fp = fopen(conf_name, "r");
    if (!fp) {
        write_log("Не удалось открыть конфигурационный файл");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    int index = 0;

    while (getline(&line, &len, fp) != -1 && index < MAX_PROC) {
        tasks[index] = parse_task(line);
        start_task(index);
        index++;
    }

    free(line);
    fclose(fp);
}

void monitor_tasks() {
    pid_t pid;
    int status;

    while (pids_count > 0) {
        pid = wait(&status);
        for (int i = 0; i < MAX_PROC; ++i) {
            if (pids[i] == pid) {
                write_log("Задача %d завершена с кодом: %d", i, status);
                pids[i] = 0;
                pids_count--;
                start_task(i);
            }
        }
    }
}

void handle_sighup(int sig) {
    (void)sig;
    write_log("Получен сигнал SIGHUP, перезапуск");

    for (int i = 0; i < MAX_PROC; ++i) {
        if (pids[i] > 0) {
            kill(pids[i], SIGKILL);
            write_log("Задача %d остановлена", i);
        }
    }

    pids_count = 0;
    memset(pids, 0, sizeof(pids));
    load_config_and_start();
}

int main(int argc, char *argv[]) {
    int opt;
    bool config_provided = false;

    while ((opt = getopt(argc, argv, "c:")) != -1) {
        if (opt == 'c') {
            strncpy(conf_name, optarg, sizeof(conf_name) - 1);
            conf_name[sizeof(conf_name) - 1] = '\0';
            config_provided = true;
        } else {
            fprintf(stderr, "Использование: %s -c <config_file>\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!config_provided) {
        fprintf(stderr, "Использование: %s -c <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        setsid();
        chdir("/");
        close_all_fds();
        open_log();
        write_pidfile();

        struct sigaction sa = {0};
        sa.sa_handler = handle_sighup;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_NODEFER;
        sigaction(SIGHUP, &sa, NULL);

        load_config_and_start();
        monitor_tasks();
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

