/* gpsdLogger: publish GPSD-provided PL/I to shared memory for use w/MGEN et al.
   Copyright (C) 2018  Fortian Inc.  <github@fortian.com>

This program is free software; you can redistribute it and/or modify it only
under the terms of Version 2 of the GNU General Public License, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef _GPS
#define _GPS

/* $Id: gpsPub.h,v 1.2 2017/04/09 22:49:50 bstern Exp $ */

#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef NULL
#define NULL 0
#endif // !NULL

typedef const void* GPSHandle;

typedef struct GPSPosition {
    double          x;          // longitude
    double          y;          // latitude
    double          z;          // altitude
    struct timeval  gps_time;   // gps time of GPS position fix
    struct timeval  sys_time;   // system time of GPS position fix
    int             xyvalid;
    int             zvalid;
    int             tvalid;     // true if time _and_ date was given in NMEA
    int		    stale;
} GPSPosition;

char* GPSMemoryInit(const char* keyFile, unsigned int size);

/*
inline GPSHandle GPSPublishInit(const char *keyFile) {
    return (GPSHandle)GPSMemoryInit(keyFile, sizeof (GPSPosition));
}
*/

#define GPSPublishInit(x) ((GPSHandle)GPSMemoryInit(x, sizeof (GPSPosition)))

void GPSPublishUpdate(GPSHandle gpsHandle, const GPSPosition* currentPosition);
void GPSPublishShutdown(GPSHandle gpsHandle, const char *keyFile);

GPSHandle GPSSubscribe(const char *keyFile);
void GPSGetCurrentPosition(GPSHandle gpsHandle, GPSPosition* currentPosition);
void GPSUnsubscribe(GPSHandle gpsHandle);

// Generic data publishing
unsigned int GPSSetMemory(GPSHandle gpsHandle, unsigned int offset,
                          const char *buffer, unsigned int len);
unsigned int GPSGetMemorySize(GPSHandle gpsHandle);
unsigned int GPSGetMemory(GPSHandle gpsHandle, unsigned int offset,
                          char *buffer, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif // _GPS
