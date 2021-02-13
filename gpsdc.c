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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <gps.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#define SYSLOG_NAMES
#include <syslog.h>
#include <errno.h>

#include "gpsPub.h"
#include "logging.h"

#if GPSD_API_MAJOR_VERSION != 6
#error "GPSD API version doesn't match"
#endif

#define S(x) #x
#define SS(x) S(x)

//#define GPSD_HOST "192.168.2.156"
//#define GPSD_HOST "10.0.0.10"
#define GPSD_HOST "127.0.0.1"
#define GPSD_PORT "2947"
#define PUBLISH_FILE NULL

#define CVSID "$Id: gpsdc.c,v 1.10 2017/04/10 16:53:57 bstern Exp $"

volatile int no_exit = 1;

void sig_handler(int signo) {
    printf("Received signal %d.  Exiting.\n", signo);
    no_exit = 0;
}

int usage(FILE *f, const char *program) {
    /* fputs(CVSID "\n", f); */
    fprintf(f, "Usage: %s [-t] [-d] [-l logpath] [-f facility]\n", program);
    fputs("\t-t enable truetime logging (in addition to GPS logging)\n", f);
    fputs("\t-d enable debug mode; don't fork, output to stdout/stderr\n", f);
    fputs("\tfacility should be one of: cron daemon local[0-7]\n", f);
    return (f == stdout) ? 0 : EINVAL;
}

int main(int argc, char *argv[]) {
    char *gps_keyfile = NULL;
    FILE *gpslog = NULL;
    FILE *ttlog = NULL;
    char *logpath = NULL;
    int hasfix = 0;
    int logtt = 0;
    int facility = LOG_DAEMON;
    int debug = 0;
    int i, j, rc;
    struct gps_data_t gps_data;
    GPSHandle gps_handle;
    GPSPosition p;
    long usec;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h")) {
            return usage(stdout, argv[0]);
        } else if (!strcmp(argv[i], "-t")) {
            logtt = 1;
        } else if (!strcmp(argv[i], "-d")) {
            debug = 1;
        } else if (i >= argc - 1) {
            return usage(stderr, argv[0]);
        } else if (!strcmp(argv[i], "-l")) {
            i++;
            if (i >= argc) {
                fputs("-l requires an argument\n", stderr);
                return usage(stderr, argv[0]);
            }
            logpath = argv[i];
        } else if (!strcmp(argv[1], "-f")) {
            i++;
            if (i >= argc) {
                fputs("-f requires an argument\n", stderr);
                return usage(stderr, argv[0]);
            }
            facility = -1;
            for (j = 0; (facility < 0) && (facilitynames[j].c_name != NULL); j++) {
                if (!strcmp(argv[i], facilitynames[j].c_name)) {
                    facility = facilitynames[j].c_val;
                }
            }
            if (facility < 0) {
                fprintf(stderr, "Invalid facility %s\n", argv[i]);
                return usage(stderr, argv[0]);
            }
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return usage(stderr, argv[0]);
        }
    }

    if (!debug) {
#if 0
        pid = fork();
        if (pid < 0) {
            perror("Couldn't fork");
            return __LINE__;
        } else if (pid) {
            printf("%ld\n", pid);
            return 0;
        } else {
            fclose(stdin);
            fclose(stdout);
            fclose(stderr);
            chdir("/");
        }
#else
        errno = 0;
        if (daemon(0, 0) < 0) {
            if (errno) {
                perror("daemon");
            } else {
                fputs("daemon failed; is something wrong with /dev/null?\n",
                    stderr);
            }
            return __LINE__;
        }
#endif
        /* We ought to become a process leader and fork again to prevent an
           accidentally opened terminal from becoming the controlling terminal,
           but this code shouldn't open any terminals, so let's skip that. */
    }

    openlog("gpsdlogger", LOG_PID, facility);
    syslog(LOG_INFO, "GPSD API version: " SS(GPSD_API_MAJOR_VERSION) "."
        SS(GPSD_API_MINOR_VERSION));
    if (debug) {
        puts("Info: GPSD API version: " SS(GPSD_API_MAJOR_VERSION) "."
            SS(GPSD_API_MINOR_VERSION));
    }

    /* Set up the signal handlers now so that I can exit cleanly. */
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Can't catch SIGINT"); 
        if (debug) {
            fputs("Error: Can't catch SIGINT\n", stderr);
        }
        closelog();
        return __LINE__;
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Can't catch SIGTERM"); 
        if (debug) {
            fputs("Error: Can't catch SIGTERM\n", stderr);
        }
        closelog();
        return __LINE__;
    }

    if ((rc = gps_open(GPSD_HOST, GPSD_PORT, &gps_data)) < 0) {
        syslog(LOG_ERR, "gps_open failed: code: %d, reason: %s",
            rc, gps_errstr(rc));
        if (debug) {
            fprintf(stderr, "Error: gps_open failed: code: %d, reason: %s",
                rc, gps_errstr(rc));
        }
        return __LINE__;
    }
    gps_stream(&gps_data, WATCH_ENABLE | WATCH_JSON | WATCH_PPS, NULL);

    if ((gps_handle = GPSPublishInit(gps_keyfile)) == NULL) {
        syslog(LOG_ERR, "Can't publish to shared memory, exiting...");
        if (debug) {
            fputs("Error: Can't publish to shared memory, exiting...\n",
                stderr);
        }
        gps_stream(&gps_data, WATCH_DISABLE, NULL);
        gps_close(&gps_data);
        closelog();
        return __LINE__;
    }
    
    /* gpsLogger does this and it is probably a good idea. */
    memset(&p, 0, sizeof(GPSPosition));

    while (no_exit) {
        p.stale = true;

        /* Wait for 2 seconds to receive data. */
        if (gps_waiting(&gps_data, 1000000)) {
            if ((rc = gps_read(&gps_data)) == -1) {
                syslog(LOG_ERR, "Error reading GPS data; code: %d, reason: %s",
                    rc, gps_errstr(rc));
                if (debug) {
                    fprintf(stderr,
                        "Error reading GPS data; code: %d, reason: %s\n",
                        rc, gps_errstr(rc));
                }
            } else {
                /* Display data from the GPS receiver. */
                if (gps_data.status == STATUS_FIX) {
                    if (!hasfix) {
                        syslog(LOG_DEBUG, "GPS fix acquired: %dD",
                            gps_data.fix.mode);
                        if (debug) {
                            fprintf(stderr, "Debug: GPS fix acquired: %dD\n",
                                gps_data.fix.mode);
                        }
                        hasfix++;
                    }
                    gettimeofday(&p.sys_time, NULL);

                    if (gps_data.fix.mode >= MODE_2D) {
                        p.xyvalid = true;
                        if (gps_data.fix.mode == MODE_3D) {
                            p.zvalid = true;
                        }
                    }
                    p.x = gps_data.fix.longitude;
                    p.y = gps_data.fix.latitude;
                    p.z = gps_data.fix.altitude;

                    usec = gps_data.toff.clock.tv_nsec / 1000;
                    /* if (p.gps_time.tv_usec != usec) { */
                        p.gps_time.tv_usec = usec;
                        p.gps_time.tv_sec = gps_data.toff.clock.tv_sec;
                        p.tvalid = true;

                        p.stale = false;
                    /* } else {
                        p.stale = true;
                    } */

#if 0
                    printf("latitude: %f, longitude: %f, altitude %f\nspeed: %f, timestamp: %ld, fix: %d/%d, gps time: %ld.%09ld, stale: %d\n\n", gps_data.fix.latitude, gps_data.fix.longitude, gps_data.fix.altitude, gps_data.fix.speed, p.sys_time.tv_sec, gps_data.status, gps_data.fix.mode, gps_data.toff.clock.tv_sec, gps_data.toff.clock.tv_nsec, p.stale);
                    printf("latitude: %f, longitude: %f, altitude %f\nsystime: %ld.%06ld, gps time: %ld.%06ld, stale: %d\n\n", p.y, p.x, p.z, p.sys_time.tv_sec, p.sys_time.tv_usec, p.gps_time.tv_sec, p.gps_time.tv_usec, p.stale);
#endif
                } else {
                    /* always complain about losing fix */
                    syslog(LOG_DEBUG, "No GPS fix; status: %d, fix: %d\n",
                        gps_data.status, gps_data.fix.mode);
                    if (debug) {
                        fprintf(stderr,
                            "Debug: No GPS fix; status: %d, fix: %d\n",
                            gps_data.status, gps_data.fix.mode);
                    }
                    hasfix = 0;
                }
            }
        }

        GPSPublishUpdate(gps_handle, &p);
        if (p.stale == false) {
            /* Defer log opening so that we can use the timestamp from the GPS
               for the filename. */
            if (gpslog == NULL) {
                gpslog = opengpslog(gps_data.toff.real.tv_sec, logpath, 0);
                if (logtt) {
                    ttlog = opengpslog(gps_data.toff.clock.tv_sec, logpath, 1);
                }
                if ((gpslog == NULL) || (logtt && (ttlog == NULL))) {
                    syslog(LOG_ERR,
                        "Unable to open local file for logging: %m");
                    if (debug) {
                        perror("Error: Unable to open local file for logging");
                    }
                    closegpslog(gpslog);
                    closegpslog(ttlog);
                    closelog();
                    gps_stream(&gps_data, WATCH_DISABLE, NULL);
                    gps_close(&gps_data);
                    return __LINE__;
                }
            }
            writegpslog(gpslog, ttlog, gps_data);
        }
    }

    closegpslog(gpslog);
    closegpslog(ttlog);

    GPSPublishShutdown(gps_handle, NULL);
    gps_handle = NULL;

    gps_stream(&gps_data, WATCH_DISABLE, NULL);
    gps_close(&gps_data);

    syslog(LOG_INFO, "Exiting...");
    if (debug) {
        fputs("Info: Exiting...\n", stderr);
    }
    closelog();
    return 0;
}
