#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "netblocker.h"

#define LOCAL_CONF_NAME "block.conf"
#define ETC_CONF_NAME "/etc/netblocker/block.conf"

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

int extract_time(FILE *config, char *buf, size_t *buf_size) {
    ssize_t n;
    int et;

    // scrolling through comments and blank lines
    while ((n = getline(&buf, buf_size, config)) != EOF) {
        if ((n = strip_fluff(buf)) != 0) {
            break;
        }
    }

    if (n != 5) {
        fprintf(stderr,
                "\"%s\" expected 24 hour format hh:mm (%td)\n", buf, n);
        exit(2);
    }

    if (buf[2] != ':') {
        fprintf(stderr,
                "\"%s\" expected 24 hour format hh:mm (%td)\n", buf, n);
        exit(2);
    }

    buf[2] = '\0';
    char *minute_buf = buf + 3;

    for (char *c = buf; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a numbers\n", buf);
            exit(2);
        }
    }

    for (char *c = minute_buf; *c != '\0'; ++c) {
        if (!isalnum(*c)) {
            fprintf(stderr, "Could not parse \"%s\" as a number\n",
                    minute_buf);
            exit(2);
        }
    }

    et = atoi(buf) * 3600 + atoi(minute_buf) * 60;

    return et;
}

struct times parse_config() {
    FILE *config;
    config = fopen(ETC_CONF_NAME, "r");

    if (config == 0) {
        config = fopen(LOCAL_CONF_NAME, "r");
    }

    if (config == 0) {
        fprintf(stderr, "could not find %s\n", LOCAL_CONF_NAME);
        exit(1);
    }

    size_t buf_size = 4096;
    char *buf = malloc(buf_size);
    struct times conf_times = { 0 };

    conf_times.start = extract_time(config, buf, &buf_size);
    conf_times.stop = extract_time(config, buf, &buf_size);

    fclose(config);
    free(buf);

    return conf_times;
}
