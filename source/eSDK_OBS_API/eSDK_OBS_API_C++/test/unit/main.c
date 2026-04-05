#include <stdio.h>
#include <string.h>

#include "test_framework.h"

static void parse_args(int argc, char **argv)
{
    int i = 0;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            g_test_verbose = 1;
        } else if (strncmp(argv[i], "--filter=", 9) == 0) {
            g_test_filter = argv[i] + 9;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--verbose] [--filter=SuiteName]\n", argv[0]);
            printf("Runs the OBS SSL/GM unit test suite.\n");
            return;
        }
    }
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);
    return test_run_all();
}
