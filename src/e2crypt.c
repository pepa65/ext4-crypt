// e2crypt.c

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>

#include "e2crypt.h"

char *contents_cipher = "aes-256-xts";
char *filename_cipher = "aes-256-cts";
unsigned padding = 0;
int usage_showed = 0;

static
void usage(FILE *std)
{
    fprintf(std, "%s - userspace tool to manage encrypted directories on ext4 filesystems\n\n", NAME);
    fprintf(std, "USAGE: %s [ [-p <len>] -i|--init | -d|--decrypt | -e|--encrypt ] <dir>\n", NAME);
    fprintf(std, "    -p|--padding <len>:  Padding of filename (4, 8, 16 or 32, default 4)\n");
    fprintf(std, "    -i|--init <dir>:     Initialize empty directory for encryption <dir>\n");
    fprintf(std, "    -d|--decrypt <dir>:  Decrypt initialized directory <dir>\n");
    fprintf(std, "    -e|--encrypt <dir>:  Encrypt initialized directory <dir>\n");
    fprintf(std, "  No options: display encryption information on directory <dir>\n");
}

void error(bool show_usage, const char *fmt, ...)
{
		if (show_usage && (!usage_showed++)) {
        usage(stderr);
        fprintf(stderr, "\n");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static
bool is_valid_padding(unsigned padding)
{
    return (padding == 4 || padding == 8 || padding == 16 || padding == 32);
}

int main(int argc, char *argv[])
{
    int c;
    char command = 0;
    char *dir_path = "";

    const char *optstring = ":hp:i:d:e:";
    static struct option longopts[] = {
        { "help", no_argument, 0, 'h' },
        { "padding", required_argument, 0, 'p' },
        { "init", required_argument, 0, 'i' },
        { "decrypt", required_argument, 0, 'd' },
        { "encrypt", required_argument, 0, 'e' },
        { 0, 0, 0, 0 },
    };

    while (true) {
        c = getopt_long(argc, argv, optstring, longopts, 0);
        if (c == -1) break;

        switch (c) {
            case 'h': usage(stdout); return EXIT_SUCCESS;
            case 'p':
                if (optarg == 0) error(1, "Option -%c requires padding as an argument", c);
                else padding = atoi(optarg);
                if (!is_valid_padding(padding))
                    error(1, "Invalid filename padding length: must be 4, 8, 16 or 32");
                break;
            case 'i':
            case 'd':
            case 'e':
                if (optarg == 0) error(1, "Option -%c requires a directory as an argument", c);
                else dir_path = optarg;
                if (command)
                    error(1, "Only one of -i|--init, -d|--decrypt and -e|--encrypt allowed");
                command = c;
                break;
            case ':': error(1, "Missing argument to -%c", optopt); break;
            case '?': error(1, "Unknown command option -%c", optopt); break;
            default: error(1, "Invalid command option -%c", optopt);
        }
    }
    if (padding && command != 'i')
        error(1, "Option -p|--padding only allowed with -i/--init");
    if (!padding) padding = 4;

    if (*dir_path == 0)
        if (argv[optind] == 0) error(1, "No directory specified");
        else dir_path = argv[optind];
    else if (argv[optind] != 0) error(1, "Only one directory at a time allowed");

    int ret = usage_showed;
    if (!ret) {
        if (command == 'i') ret = container_create(dir_path);
        else if (command == 'd') ret = container_attach(dir_path);
        else if (command == 'e') ret = container_detach(dir_path);
        else ret = container_status(dir_path);
    }
    return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
