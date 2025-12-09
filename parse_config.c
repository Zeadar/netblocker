#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netblocker.h"

#define LOCAL_CONF_NAME "block.conf"
#define ETC_CONF_NAME "/etc/netblocker/block.conf"

#define START_EXPR "start="
#define STOP_EXPR "stop="

enum time_type {
    ERROR_PARSING, ERROR_END_OF_FILE, ERROR_NEITHER, START, STOP,
};

ptrdiff_t strip_fluff(char *line) {
    char *read = line, *write = line;

    while (*read != '\0') {
        if (*read == ' ' || *read == '\t' || *read == '\n') {
            ++read;
            continue;
        }

        if (*read == '#') {
            break;
        }

        *write = *read;

        ++write;
        ++read;
    }

    *write = '\0';
    return write - line;
}

enum time_type extract_time(FILE *config, char *buf,
                            size_t *buf_size, int *time) {
    ssize_t num_read;
    enum time_type tt;
    char *rvalue;
    unsigned line = 1;

    // scrolling through comments and blank lines
    while ((num_read = getline(&buf, buf_size, config)) != EOF) {
        line++;
        if ((num_read = strip_fluff(buf)) != 0) {
            break;
        }
    }

    if (num_read == EOF)
        return ERROR_END_OF_FILE;

    if (strncmp(buf, START_EXPR, strlen(START_EXPR)) == 0) {
        tt = START;
        rvalue = buf + strlen(START_EXPR);
        num_read -= strlen(START_EXPR);
    } else if (strncmp(buf, STOP_EXPR, strlen(STOP_EXPR)) == 0) {
        tt = STOP;
        rvalue = buf + strlen(STOP_EXPR);
        num_read -= strlen(STOP_EXPR);
    } else {
        fprintf(stderr, "Could not interpret [%s] on line %u\n", buf,
                line);
        return ERROR_NEITHER;
    }

    if (num_read != 5) {
        fprintf(stderr,
                "\"%s\" expected 24 hour format hh:mm on line (%u)\n",
                rvalue, line);
        return ERROR_PARSING;
    }

    if (rvalue[2] != ':') {
        fprintf(stderr,
                "\"%s\" expected 24 hour format hh:mm on line (%u)\n",
                rvalue, line);
        return ERROR_PARSING;
    }

    rvalue[2] = '\0';
    char *minute_rvalue = rvalue + 3;

    for (char *c = rvalue; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a numbers\n",
                    rvalue);
            return ERROR_PARSING;
        }
    }

    for (char *c = minute_rvalue; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a number\n",
                    minute_rvalue);
            return ERROR_PARSING;
        }
    }

    *time = atoi(rvalue) * 3600 + atoi(minute_rvalue) * 60;
    printf("parsed time %d\n", *time);

    return tt;
}

struct times parse_config() {
    FILE *config = fopen(LOCAL_CONF_NAME, "r");
    if (config == 0) {
        config = fopen(ETC_CONF_NAME, "r");
    }
    if (config == 0) {
        fprintf(stderr, "Error opening config file\n");
        exit(300);
    }
    size_t buf_size = 4096;
    char *buf = malloc(buf_size);
    struct times conf_times = { -1, -1 };
    enum time_type tt;
    int recieved_time;

    if (config == 0) {
        fprintf(stderr, "could not find %s\n", LOCAL_CONF_NAME);
        exit(1);
    }

    while (conf_times.start == -1 || conf_times.stop == -1) {
        tt = extract_time(config, buf, &buf_size, &recieved_time);

        switch (tt) {
        case START:
            conf_times.start = recieved_time;
            break;
        case STOP:
            conf_times.stop = recieved_time;
            break;
        default:
            fprintf(stderr, "Encountered an error parsing config file\n");
            exit(300);
        }
    }

    fclose(config);
    free(buf);

    return conf_times;
}
