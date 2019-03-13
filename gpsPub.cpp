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

#include <syslog.h>
#include "gpsPub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *GPS_DEFAULT_KEY_FILE = "/tmp/gpskey";

#ifdef __cplusplus
extern "C" {
#endif

// Upon success, this returns a pointer for storage of published GPS position.
char *GPSMemoryInit(const char *keyFile, unsigned int size) { char *posPtr; int
    id; FILE *filePtr; struct shmid_ds ds;

    if (keyFile == NULL) {
        keyFile = GPS_DEFAULT_KEY_FILE;
    }
    posPtr = ((char *)-1);
    id = -1;

    // First read file to see if shared memory already active
    // If active, try to use it
    filePtr = fopen(keyFile, "r");
    if (filePtr != NULL) {
        if (1 == fscanf(filePtr, "%d", &id)) {
            if (((char *) -1) != (posPtr = (char *)shmat(id, 0, 0))) {
                // Make sure pre-existing shared memory is right size
                if (size != *((unsigned int *)posPtr)) {
                    GPSPublishShutdown((GPSHandle)posPtr, keyFile);
                    posPtr = (char *)-1;
                }
            } else {
               syslog(LOG_WARNING, "GPSPublishInit(): shmat(): %m");
            }
        }
        fclose(filePtr);
    }

    if (((char *)-1) == posPtr) {
        // Create new shared memory segment
        // and advertise its "id" in the keyFile
        id = shmget(0, (int)(size + sizeof (unsigned int)),
            IPC_CREAT | SHM_R | S_IRGRP | S_IROTH | SHM_W);
        if (-1 == id) {
            syslog(LOG_ERR, "GPSPublishInit(): shmget(): %m");
            return NULL;
        }
        if (((char*)-1) ==(posPtr = (char*)shmat(id, 0, 0))) {
            syslog(LOG_ERR, "GPSPublishInit(): shmat(): %m");
            if (-1 == shmctl(id, IPC_RMID, &ds)) {
                syslog(LOG_ERR, "GPSPublishInit(): shmctl(IPC_RMID): %m");
            }
            return NULL;
        }
        // Write "id" to "keyFile"
        if ((filePtr = fopen(keyFile, "w+"))) {
            if (fprintf(filePtr, "%d", id) <= 0) {
                syslog(LOG_ERR, "GPSPublishInit() fprintf(): %m");
            }
            fclose(filePtr);
            memset(posPtr + sizeof (unsigned int), 0, size);
            *((unsigned int *)posPtr) = size;
            return (posPtr + sizeof (unsigned int));
        } else {
            syslog(LOG_ERR, "GPSPublishInit() fopen(): %m");
        }
        if (-1 == shmdt(posPtr)) {
            syslog(LOG_WARNING, "GPSPublishInit() shmdt(): %m");
        }
        if (-1 == shmctl(id, IPC_RMID, &ds)) {
            syslog(LOG_WARNING, "GPSPublishInit(): shmctl(IPC_RMID): %m");
        }
        return NULL;
    }

    if (((char *)-1) == posPtr) {
        return NULL;
    }
    return (posPtr + sizeof (unsigned int));
}

void GPSPublishShutdown(GPSHandle gpsHandle, const char *keyFile) {
    char *ptr;
    FILE *filePtr;
    int id;
    struct shmid_ds ds;

    ptr = (char *)gpsHandle - sizeof (unsigned int);
    if (keyFile == NULL) {
        keyFile = GPS_DEFAULT_KEY_FILE;
    }
    if (-1 == shmdt((void *)ptr)) {
        syslog(LOG_WARNING, "GPSPublishShutdown() shmdt(): %m");
    }
    filePtr = fopen(keyFile, "r");
    if (filePtr != NULL) {
        if (1 == fscanf(filePtr, "%d", &id)) {
            if (-1 == shmctl(id, IPC_RMID, &ds)) {
                syslog(LOG_WARNING,
                    "GPSPublishShutdown(): shmctl(IPC_RMID): %m");
            }
        }
        fclose(filePtr);
        if (unlink(keyFile)) {
            syslog(LOG_WARNING, "GPSPublishShutdown(): unlink(): %m");
        }
    } else {
        syslog(LOG_WARNING, "GPSPublishShutdown(): fopen(): %m");
    }
}  // end GPSPublishShutdown;

// Upon success, this returns a pointer for storage of published GPS position.
GPSHandle GPSSubscribe(const char *keyFile) {
    char* posPtr;
    int id;
    FILE *filePtr;
    char *rv = NULL;

    id = -1;
    posPtr = ((char *)-1);

    // First read file to see if shared memory already active.  If active, try
    // to use it.
    if (keyFile == NULL) {
        keyFile = GPS_DEFAULT_KEY_FILE;
    }
    filePtr = fopen(keyFile, "r");
    if (filePtr != NULL) {
        if (1 == fscanf(filePtr, "%d", &id)) {
            if (((char *)-1) == (posPtr = (char *)shmat(id, 0, SHM_RDONLY))) {
               syslog(LOG_ERR, "GPSSubscribe(): shmat(): %m");
               fclose(filePtr);
            } else {
                fclose(filePtr);
                rv = posPtr + sizeof (unsigned int);
            }
        } else {
            syslog(LOG_ERR, "GPSSubscribe(): fscanf(): %m");
            fclose(filePtr);
        }
    } else {
        syslog(LOG_ERR, "GPSSubscribe(): fopen(): %m");
    }

    return rv;
}

void GPSUnsubscribe(GPSHandle gpsHandle) {
    char *ptr;

    ptr = (char *)gpsHandle - sizeof (unsigned int);
    if (-1 == shmdt((void *)ptr)) {
        syslog(LOG_WARNING, "GPSUnsubscribe() shmdt(): %m");
    }
}

void GPSPublishUpdate(GPSHandle gpsHandle, const GPSPosition* currentPosition) {
    memcpy((char *)gpsHandle, (char *)currentPosition, sizeof (GPSPosition));
}

void GPSGetCurrentPosition(GPSHandle gpsHandle, GPSPosition* currentPosition) {
    memcpy((char *)currentPosition, (char *)gpsHandle, sizeof (GPSPosition));
}

unsigned int GPSSetMemory(GPSHandle gpsHandle, unsigned int offset,
    const char *buffer, unsigned int len) {
    char *ptr;
    unsigned int size, delta;

    ptr = (char *)gpsHandle - sizeof (unsigned int);
    size = (*((unsigned int *)ptr));

    // Make sure request fits into available shared memory.
    if ((offset + len) > size) {
        syslog(LOG_WARNING,
            "GPSSetMemory() request exceeds allocated shared memory");
        delta = offset + len - size;
        if (delta > len) {
            return 0;
        } else {
            len -= delta;
        }
    }
    ptr = (char *)gpsHandle + offset;
    memcpy(ptr, buffer, len);
    return len;
}

unsigned int GPSGetMemorySize(GPSHandle gpsHandle) {
    char *ptr;

    ptr = (char *)gpsHandle - sizeof (unsigned int);
    unsigned int size = *((unsigned int *)ptr);
    return size;
}

unsigned int GPSGetMemory(GPSHandle gpsHandle, unsigned int offset,
    char *buffer, unsigned int len) {
    char *ptr;
    unsigned int size, delta;

    ptr = (char *)gpsHandle - sizeof (unsigned int);
    size = *((unsigned int *)ptr);
    if (size < (offset + len)) {
        delta = offset + len - size;
        if (delta > len) {
            syslog(LOG_ERR, "GPSGetMemory(): invalid request");
            return 0;
        } else {
            len -= delta;
        }
    }
    ptr = (char *)gpsHandle + offset;
    memcpy(buffer, ptr, len);
    return len;
}

#ifdef __cplusplus
}
#endif
