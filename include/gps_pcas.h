#include <stddef.h>
#include <stdint.h>

#include "lwgps/lwgps_opt.h"

static const char pcas03[] =
    "$PCAS03,"
#if LWGPS_CFG_STATEMENT_GPGGA
    "1," // GGA
#else
    "0," // GGA
#endif
    "0," // GLL
#if LWGPS_CFG_STATEMENT_GPGSA
    "1," // GSA
#else
    "0," // GSA
#endif
#if LWGPS_CFG_STATEMENT_GPGSV
    "1," // GSV
#else
    "0," // GSV
#endif
#if LWGPS_CFG_STATEMENT_GPRMC
    "1," // RMC
#else
    "0," // RMC
#endif
    "0,"        // VTG
    "0,"        // ZDA
    "0,"        // ANT
    "0,0,,,0,0" // Reserved
    "*"         // Start of checksum
#if LWGPS_CFG_STATEMENT_GPGGA ^ LWGPS_CFG_STATEMENT_GPGSA ^                    \
    LWGPS_CFG_STATEMENT_GPGSV ^ LWGPS_CFG_STATEMENT_GPRMC
    "03" // Checksum (recalculate if you manually change anything)
#else
    "02" // Checksum (recalculate if you manually change anything)
#endif
    "\r\n";

static const size_t pcas03_len = sizeof(pcas03) - 1; // Exclude null terminator
