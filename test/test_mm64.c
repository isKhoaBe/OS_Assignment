#define MM_PAGING
#include <stdio.h>
#include "mm64.h"

int main() {
    #ifdef MM64
    printf("MM64 is defined\n");
    #else
    printf("MM64 NOT defined\n");
    #endif
    return 0;
}