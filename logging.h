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

#ifndef _LOGGING_H
#define _LOGGING_H

/* $Id: logging.h,v 1.4 2017/04/10 16:53:58 bstern Exp $ */

#include <time.h>
#include <gps.h>

#define LOG_PATH "/tmp/gpsdlogger"

FILE *opengpslog(time_t timeref, const char *logpath, int is_tt);
int writegpslog(FILE *gpslog, FILE *ttlog, struct gps_data_t);
int closegpslog(FILE *);

#endif /* _LOGGING_H */
