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
#define DAYS_EXPR "days="

enum parse_type {
    SUCCESS,
    ERROR_PARSING,
    ERROR_UNRECOGNIZED,
    START,
    STOP,
    DAYS,
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

enum parse_type parse_time(char *buf, int *out_buf) {
    unsigned num_read = strlen(buf);

    if (num_read != 5) {
        fprintf(stderr, "\"%s\" expected 24 hour format hh:mm\n", buf);
        return ERROR_PARSING;
    }

    if (buf[2] != ':') {
        fprintf(stderr, "\"%s\" expected 24 hour format hh:mm\n", buf);
        return ERROR_PARSING;
    }

    buf[2] = '\0';
    char *minute_buf = buf + 3;

    for (char *c = buf; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a number\n", buf);
            return ERROR_PARSING;
        }
    }

    for (char *c = minute_buf; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a number\n",
                    minute_buf);
            return ERROR_PARSING;
        }
    }

    *out_buf = atoi(buf) * 3600 + atoi(minute_buf) * 60;

    return SUCCESS;
}

enum parse_type get_type(char **buf) {
    char *buffer = *buf;
    unsigned len;
    enum parse_type pt;

    if (strncmp(buffer, START_EXPR, (len = strlen(START_EXPR))) == 0) {
        pt = START;
        *buf = buffer + len;
    } else if (strncmp(buffer, STOP_EXPR, (len = strlen(STOP_EXPR))) == 0) {
        pt = STOP;
        *buf = buffer + len;
    } else if (strncmp(buffer, DAYS_EXPR, (len = strlen(DAYS_EXPR))) == 0) {
        pt = DAYS;
        *buf = buffer + len;
    } else {
        fprintf(stderr, "Unrecognized thingie [%s]\n", buffer);
        return ERROR_UNRECOGNIZED;
    }

    return pt;
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
    char *temp_buf;
    struct times conf_times = { -1, -1, 0 };
    enum parse_type pt;
    int num_read = 0;
    int line_nr = 0;
    int out_buf = 0;

    if (config == 0) {
        fprintf(stderr, "could not find %s\n", LOCAL_CONF_NAME);
        exit(1);
    }

    while ((num_read = getline(&buf, &buf_size, config)) != EOF) {
        ++line_nr;
        // scrolling through comments and blank lines
        if ((num_read = strip_fluff(buf)) == 0)
            continue;

        temp_buf = buf;
        pt = get_type(&temp_buf);

        switch (pt) {
        case START:
            pt = parse_time(temp_buf, &out_buf);
            conf_times.start = out_buf;
            break;
        case STOP:
            pt = parse_time(temp_buf, &out_buf);
            conf_times.stop = out_buf;
            break;
        case DAYS:
            break;
        default:
            fprintf(stderr,
                    "Encountered an error parsing config file (%d)\n",
                    line_nr);
            exit(300);
        }

    }

    fclose(config);
    free(buf);

    if (conf_times.start == -1 || conf_times.stop == -1) {
        fprintf(stderr, "Missing mandatory items in config\n");
        exit(300);
    }

    return conf_times;
}
