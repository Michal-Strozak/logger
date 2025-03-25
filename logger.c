#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

// Plik logowania
static FILE *log_file = NULL;

// Mutexy
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dump_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t toggle_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t level_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

// Semafory do obsługi sygnałów
static sem_t dump_sem, log_sem, level_sem;

// Przechowywane dane sygnałów
static int log_toggle_state;
static int log_level;
static int is_initialized = 0;
static volatile sig_atomic_t signal_info;
static volatile sig_atomic_t stop_threads = 0;

// Wątki
static pthread_t dump_thread, toggle_thread, level_thread;

// Wskaźnik do funkcji zapisu stanu aplikacji do pliku dump
static void (*dump_callback)(FILE *) = NULL;

// Obsługa sygnałów
void signal_handler(int signo, siginfo_t *info, void *context) {
    signal_info = info->si_value.sival_int;
    if (signo == SIG_DUMP) {
        sem_post(&dump_sem);
    } else if (signo == SIG_LOG_TOGGLE) {
        sem_post(&log_sem);
    } else if (signo == SIG_LOG_LEVEL) {
        sem_post(&level_sem);
    }
}

// Funkcja do konfigurowania zawartości pliku dump
void set_dump_callback(void (*callback)(FILE *)) {
    pthread_mutex_lock(&dump_mutex);
    dump_callback = callback;
    pthread_mutex_unlock(&dump_mutex);
}

// Funkcja obsługująca sygnał dump
void *handle_dump(void *arg) {
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIG_DUMP);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while (stop_threads == 0) {
        sem_wait(&dump_sem);
        if (stop_threads == 1) {
            break;
        }

        pthread_mutex_lock(&dump_mutex);
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char filename[64];
        strftime(filename, sizeof(filename), "dump_%Y-%m-%d_%H:%M:%S.dump", tm_info);
        FILE *dump_file = fopen(filename, "w");
        if (dump_file) {
            if (dump_callback) {
                dump_callback(dump_file);
            } else {
                fprintf(dump_file, "Czas wykonania zrzutu: %s", asctime(tm_info));
                fprintf(dump_file, "PID: %d\n", getpid());
                fprintf(dump_file, "Dane sygnału: %d\n", signal_info);
                fprintf(dump_file, "log_level = %d\n", log_level);
                fprintf(dump_file, "log_toggle_state = %d\n", log_toggle_state);
            }
            fclose(dump_file);
        }
        pthread_mutex_unlock(&dump_mutex);
    }
    return NULL;
}

// Wątek obsługujący SIG_LOG_TOGGLE
void *handle_toggle(void *arg) {
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIG_LOG_TOGGLE);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while (stop_threads == 0) {
        sem_wait(&log_sem);
        if (stop_threads == 1) {
            break;
        }

        pthread_mutex_lock(&toggle_mutex);
        if (log_toggle_state == 0) {
            log_toggle_state = 1;
        } else {
            log_toggle_state = 0;
        }
        pthread_mutex_unlock(&toggle_mutex);
    }
    return NULL;
}

// Wątek obsługujący SIG_LOG_LEVEL
void *handle_level(void *arg) {
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIG_LOG_LEVEL);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while (stop_threads == 0) {
        sem_wait(&level_sem);
        if (stop_threads == 1) {
            break;
        }

        pthread_mutex_lock(&level_mutex);
        if (log_level == MIN) {
            log_level = STANDARD;
        } else if (log_level == STANDARD) {
            log_level = MAX;
        } else if (log_level == MAX) {
            log_level = MIN;
        }
        pthread_mutex_unlock(&level_mutex);
    }
    return NULL;
}

// Funkcja zapisująca logi
int write_log(int level, const char *fmt, ...) {
    pthread_mutex_lock(&init_mutex);
    if (is_initialized == 0) {
        pthread_mutex_unlock(&init_mutex);
        return 1;
    }
    pthread_mutex_unlock(&init_mutex);

    pthread_mutex_lock(&toggle_mutex);
    if (log_toggle_state == 0) {
        pthread_mutex_unlock(&toggle_mutex);
        return 2;
    }
    pthread_mutex_unlock(&toggle_mutex);

    pthread_mutex_lock(&level_mutex);
    if (level < 1 || level > 3 || level > log_level) {
        pthread_mutex_unlock(&level_mutex);
        return 3;
    }
    pthread_mutex_unlock(&level_mutex);

    pthread_mutex_lock(&log_mutex);
    log_file = fopen("app.log", "a");
    if (!log_file) {
        pthread_mutex_unlock(&log_mutex);
        return 3;
    }
    va_list args;
    va_start(args, fmt);
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    fprintf(log_file, "[%02d:%02d:%02d] ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");
    fclose(log_file);
    log_file = NULL;
    va_end(args);
    pthread_mutex_unlock(&log_mutex);
    return 0;
}

// Inicjalizacja loggera
int init_logger() {
    pthread_mutex_lock(&init_mutex);
    if (is_initialized == 1) {
        pthread_mutex_unlock(&init_mutex);
        return 1;
    }
    is_initialized = 1;
    log_toggle_state = 1;
    log_level = MIN;

    // Inicjalizacja semaforów
    sem_init(&dump_sem, 0, 0);
    sem_init(&log_sem, 0, 0);
    sem_init(&level_sem, 0, 0);

    // Tworzenie wątków
    pthread_create(&dump_thread, NULL, handle_dump, NULL);
    pthread_create(&toggle_thread, NULL, handle_toggle, NULL);
    pthread_create(&level_thread, NULL, handle_level, NULL);

    // Obsługa sygnałów
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigfillset(&sa.sa_mask);
    sigaction(SIG_DUMP, &sa, NULL);
    sigaction(SIG_LOG_TOGGLE, &sa, NULL);
    sigaction(SIG_LOG_LEVEL, &sa, NULL);

    pthread_mutex_unlock(&init_mutex);
    return 0;
}

// Zamknięcie loggera
int close_logger() {
    pthread_mutex_lock(&init_mutex);
    if (is_initialized == 0) {
        pthread_mutex_unlock(&init_mutex);
        return 1;
    }
    is_initialized = 0;

    // Naturalne zakończenie pracy wątków
    stop_threads = 1;
    sem_post(&dump_sem);
    sem_post(&log_sem);
    sem_post(&level_sem);
    pthread_join(dump_thread, NULL);
    pthread_join(toggle_thread, NULL);
    pthread_join(level_thread, NULL);

    // Usunięcie semaforów
    sem_destroy(&dump_sem);
    sem_destroy(&log_sem);
    sem_destroy(&level_sem);

    // Przywrócenie domyślnych sygnałów
    struct sigaction sa_default;
    sa_default.sa_handler = SIG_DFL;
    sigemptyset(&sa_default.sa_mask);
    sa_default.sa_flags = 0;
    sigaction(SIG_DUMP, &sa_default, NULL);
    sigaction(SIG_LOG_TOGGLE, &sa_default, NULL);
    sigaction(SIG_LOG_LEVEL, &sa_default, NULL);

    pthread_mutex_unlock(&init_mutex);
    return 0;
}
