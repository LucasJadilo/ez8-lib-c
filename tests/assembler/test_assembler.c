#include "ez8.h"
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }

    char *last_slash;
    if ((NULL != (last_slash = strrchr(argv[0], '/'))) ||
        (NULL != (last_slash = strrchr(argv[0], '\\')))) {
        *last_slash = '\0';
    }

    return ez8_compile_assembly(argv[1], argv[0]);
}
