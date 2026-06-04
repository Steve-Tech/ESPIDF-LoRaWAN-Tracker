#pragma once

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CONSTRAIN(amt, low, high)                                              \
    ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
