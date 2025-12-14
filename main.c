#include "netblocker.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define CHECK_RULE_4 "iptables -C OUTPUT -j REJECT 2>/dev/null"
#define ADD_RULE_4 "iptables -A OUTPUT -j REJECT"
#define DELETE_RULE_4 "iptables -D OUTPUT -j REJECT"
#define CHECK_RULE_6 "ip6tables -C OUTPUT -j REJECT 2>/dev/null"
#define ADD_RULE_6 "ip6tables -A OUTPUT -j REJECT"
#define DELETE_RULE_6 "ip6tables -D OUTPUT -j REJECT"

#define ERROR_FORMAT "command [ %s ] returned value %d\n"

#define DAY_SEC (24 * 60 * 60)

void add_rules() {
    int ret;

    ret = system(CHECK_RULE_4);
    if (ret != 0) {
        ret = system(ADD_RULE_4);
        if (ret != 0) {
            fprintf(stderr, ERROR_FORMAT, ADD_RULE_4, ret);
            exit(ret);
        }
    }

    ret = system(CHECK_RULE_6);
    if (ret != 0) {
        ret = system(ADD_RULE_6);
        if (ret != 0) {
            fprintf(stderr, ERROR_FORMAT, ADD_RULE_6, ret);
            exit(ret);
        }
    }
}

void delete_rules() {
    int ret;

    ret = system(CHECK_RULE_4);
    if (ret == 0) {
        ret = system(DELETE_RULE_4);
        if (ret != 0) {
            fprintf(stderr, ERROR_FORMAT, DELETE_RULE_4, ret);
            exit(ret);
        }
    }

    ret = system(CHECK_RULE_6);
    if (ret == 0) {
        ret = system(DELETE_RULE_6);
        if (ret != 0) {
            fprintf(stderr, ERROR_FORMAT, DELETE_RULE_6, ret);
            exit(ret);
        }
    }
}

void exit_handler(int sig) {
    char exit_msg[1024];
    sprintf(exit_msg, "\nRecieved sig %d, exiting...\n", sig);
    write(1, exit_msg, strlen(exit_msg));
    delete_rules();
    exit(0);
}

#ifdef DEBUG_BUILD
void log_sleep_time(int time_in_sec) {
    int hours = time_in_sec / 60 / 60;
    int minutes = (time_in_sec / 60) % 60;
    int seconds = ((time_in_sec % 60) % 60);

    printf("Sleep for: %02d:%02d:%02d\n", hours, minutes, seconds);
}
#endif

void *scheduler(void *time_ptr) {
    const struct times *target_times = time_ptr;
    int now;
    time_t now_epoch;
    unsigned int sleep_time;
    struct tm today = { 0 };

#ifdef DEBUG_BUILD
    printf("%d → %d\n", target_times->start, target_times->stop);
#endif

    if (target_times->start < target_times->stop) {
        // time span is within the day
        for (;;) {
            now_epoch = time(0);
            localtime_r(&now_epoch, &today);
            now = today.tm_hour * 3600 + today.tm_min * 60 + today.tm_sec;
            if (target_times->start <= now && target_times->stop > now) {
                // blocking should be on
                add_rules();
                sleep_time = target_times->stop - now;
            } else if (target_times->stop <= now) {
                // blocking should be off
                delete_rules();
                sleep_time = (DAY_SEC - now) + target_times->start;
            } else {            // now is before start
                // blocking should be off
                delete_rules();
                sleep_time = target_times->start - now;
            }
#ifdef DEBUG_BUILD
            log_sleep_time(sleep_time);
#endif
            sleep(sleep_time);
        }
    } else if (target_times->stop < target_times->start) {
        // time spans adjecent days
        for (;;) {
            now_epoch = time(0);
            localtime_r(&now_epoch, &today);
            now = today.tm_hour * 3600 + today.tm_min * 60 + today.tm_sec;
            if (target_times->start <= now) {
                // blocking should be on
                add_rules();
                sleep_time = (DAY_SEC - now) + target_times->stop;
            } else if (target_times->stop > now) {
                // blocking should be on
                add_rules();
                sleep_time = target_times->stop - now;
            } else {
                // blocking should be off
                delete_rules();
                sleep_time = target_times->start - now;
            }
#ifdef DEBUG_BUILD
            log_sleep_time(sleep_time);
#endif
            sleep(sleep_time);
        }
    } else {
        fprintf(stderr, "start time and stop time is the same\n");
        exit(100);
    }
}

int main() {
    if (geteuid() != 0) {
        fprintf(stderr, "Must run as root\n");
        return 200;
    }

    struct times target_times = parse_config();
    struct sigaction sa = { 0 };
    pthread_t thr;
    sa.sa_handler = exit_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);

    for (;;) {
        pthread_create(&thr, 0, scheduler, &target_times);

        int waitresult = wait_for_wakeup();
        if (waitresult != 0)
            return waitresult;
        pthread_cancel(thr);
    }

    return 0;
}
