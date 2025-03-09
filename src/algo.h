#include <stdio.h>
#include <stdlib.h>

struct curve {
    /* to do: dynamic. */
    unsigned int curve[10];
    unsigned int rate;
    unsigned int critical_lower;
    unsigned int critical_upper;
    
    int util;
};
