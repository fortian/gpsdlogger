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
Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include "logging.h"

/* $Id: logging.c,v 1.7 2017/04/10 16:53:57 bstern Exp $ */

FILE *opengpslog(time_t timeref, const char *logpath, int is_tt) {
    char filename[PATH_MAX];
    char timestr[16];
    time_t timer;
    struct tm* tm_info;
    FILE *retptr = NULL;
    const char *lp = logpath;
    struct stat sb;

    if (lp == NULL) {
        lp = LOG_PATH;
    }

    if (timeref > 0) {
        timer = timeref;
    } else {
        timer = time(NULL);
    }
    if (timer == (time_t)(-1)) {
        syslog(LOG_ERR, "opengpslog: time: %m");
        return NULL;
    }

    if (stat(lp, &sb) < 0) {
        if (errno != ENOENT) {
            syslog(LOG_ERR, "opengpslog: stat: %m");
            return NULL;
        }
        if (mkdir(lp, 0755) < 0) {
            syslog(LOG_ERR, "opengpslog: mkdir: %m");
            return NULL;
        }
    }

    tm_info = gmtime(&timer);

    // Format YYYYMMDD.HHMMSS
    strftime(timestr, 32, "%Y%m%d.%H%M%S", tm_info);
    snprintf(filename, PATH_MAX, "%s/%s%s", lp, timestr, is_tt ? ".tt" : "");
    filename[PATH_MAX - 1] = 0;

    syslog(LOG_DEBUG, "Opening '%s' for writing", filename);
    if ((retptr = fopen(filename, "w")) == NULL) {
        syslog(LOG_ERR, "opengpslog: fopen(%s): %m", filename);
    }

    return retptr;
}

int writegpslog(FILE *gpslog, FILE *ttlog, struct gps_data_t gps_data) {
    int rv2 = 0;
    int rv1;
    int hour, min, sec;
    suseconds_t usec;

    hour = (gps_data.toff.real.tv_sec % 86400) / 3600;
    min = (gps_data.toff.real.tv_sec % 3600) / 60;
    sec = gps_data.toff.real.tv_sec % 60;
    usec = gps_data.toff.real.tv_nsec / 1000;

    rv1 = fprintf(gpslog, "time>%02d:%02d:%02d.%06lu position>%f,%f,%f\n",
        hour, min, sec, usec,
        gps_data.fix.latitude, gps_data.fix.longitude, gps_data.fix.altitude);
    if (rv1 < 0) {
        syslog(LOG_ERR, "writegpslog: fprintf gpslog: %m");
    }

    /* try to write the truetime log anyway, if possible */
    if (ttlog != NULL) {
        /* sec, fix, lat, long, alt, track, horiz speed m/s, climb m/s,
           uncertainty for each stat except for fix;
           lat, long only valid for 2D/3D fix, alt only valid for 3D fix */
        rv2 = fprintf(ttlog, "%ld,%d,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
            gps_data.toff.real.tv_sec,
            /* gps_data.toff.real.tv_nsec, */ /* this is always 0 */
            gps_data.fix.mode, gps_data.fix.latitude, gps_data.fix.longitude,
            gps_data.fix.altitude,
            gps_data.fix.track, gps_data.fix.speed, gps_data.fix.climb,
            gps_data.fix.ept, gps_data.fix.epy, gps_data.fix.epx,
            gps_data.fix.epv, gps_data.fix.epd, gps_data.fix.eps,
            gps_data.fix.epc);
        if (rv2 < 0) {
            syslog(LOG_ERR, "writegpslog: fprintf ttlog: %m");
            rv1 = rv2;
        }
        fflush(ttlog);
    }
    fflush(gpslog);
    return rv1;
}

int closegpslog(FILE *log) {
    int rv = 0;
    
    if ((log != NULL) && ((rv = fclose(log)))) {
        syslog(LOG_ERR, "closegpslog: fclose: %m");
    }
    return rv;
}
