# About

`gpsdLogger` is a [`gpsd`](https://gpsd.gitlab.io/gpsd/index.html) client which
publishes Position/Location Information (P/LI) to a shared memory segment,
compatible with
[`gpsLogger`](https://github.com/USNavalResearchLaboratory/gpsLogger) and
[MGEN](https://github.com/USNavalResearchLaboratory/mgen).  It does not set
time, as it is expected that systems using `gpsdLogger` will also use
[`ntpd`](http://ntp.org/downloads.html) for time management.  (Otherwise,
`gpsLogger`, without the `d`, is a more suitable program, as it manages time and
logs P/LI in a single program.)

It was created to address the situation where sole access to a GPS receiver
could not be ensured, and therefore `gpsd` was used to multiplex GPS data
access.  Although `gpsd` provides timing information suitable for use with
`ntpd`, it does not publish P/LI in a format that MGEN can understand.  Rather
than patch MGEN, this utility was designed to emulate `gpsLogger`'s shared
memory publishing, allowing MGEN to automatically acquire P/LI from `gpsd`.

# Prerequisites

You will need the following packages to build `gpsdlogger`:

## Slackware et al.

- `gcc`
- `gpsd` (from [SlackBuilds](http://slackbuilds.org/repository/14.2/gis/gpsd/))
- `make`
- `scons` (only to build `gpsd`)

## Debian et al. (Ubuntu, etc.)

- `build-essential`
- `libgps-dev`

For troubleshooting, you will probably also want:

- `gpsd-clients`
- `gpsd-tools`

## Red Hat et al. (CentOS, Fedora, etc.)

- `gcc`
- `gpsd`
- `libgpsd-devel`
- `make`

## Additional Packages

You probably also want `ntp` (or `ntpd`) to set the time on the system, and
`mgen` or similar to consume the data published by `gpsdlogger`.  (Neither of
these are requirements for building nor running `gpsdlogger`.)

# Building

    make

# Running

You need `gpsd` configured for your GPS (which is outside the scope of this
document) and listening for network connections.  (This means `gpsd` and
`gpsdlogger` both need not to be blocked by SELinux or `iptables`.)

After that, just run `gpsdlogger` and it will automatically attempt to connect
to the local `gpsd` instance.

`gpsdlogger` will close all logfiles and exit upon receipt of `SIGTERM`.

### Runtime Options

#### Debug Mode

By default, `gpsdlogger` forks to the background and doesn't print anything to
`stdout` and `stderr`.  If you enable "debug mode" with `-d`, it won't `fork`
and will print additional information to `stdout`/`stderr`, in addition to
normal logging.

#### Logging Control

By default, logs are written to `/tmp/gpsdlogger` in filenames named after the
timestamp (YYYYMMDD.HHMMSS) when `gpsdlogger` first gets fix.  This path can be
controlled with `-l`  (lowercase "ell" or "Lima") and does not need to exist
(though the parent directory does).  You should change it from the default to
keep the logs safe from automatic cleaners and system reboots.

An example filename is `20210213.131214`, which would be created if `gpsdlogger`
first got fix at 13:12:14 on 13 Feb 2021.  These timestamps are from `gpsd` and
are therefore in GPS time.

If "truetime" logging is enabled (see below), an additional log with more
information will be created, with the same filename format, but using the
system clock timestamp at time of first fix.

#### Syslog Facility

`gpsdlogger` logs to syslog with identifier `gpsdlogger` and tries to include
the PID with each log message.  By default, it uses the "daemon" logging
facility, but you can change it with `-f` to any valid facility name.
Recommendations are one of "cron", "daemon", or "local0" through "local7". (See
`syslog(3)` for more details.)

#### Logging

`gpsdlogger` only logs to the logfile(s) when it has fix.  It will log to
`syslog` whenever it doesn't have 3D fix, and whenever it acquires fix, at log
level "debug".  (Note that most systems don't actually record "debug" `syslog`
messages, so you may need to enable them when troubleshooting.)

Normally, `gpsdlogger` only logs the following information:

- GPS timestamp
  - Hour
  - Minute
  - Second
  - Microsecond
- GPS position
  - Latitude
  - Longitude
  - Altitude

##### Important Note

If no logfile exists in `logpath`, `gpsdlogger` hasn't gotten fix yet.

If a logfile exists but it's not getting updated, and `gpsdlogger` is still
running, it doesn't have fix.

#### Truetime Logging

If "truetime" logging is enabled (with `-t`), `gpsdlogger` will also log the
following information to the "truetime" logfile:

- GPS timestamp (seconds)
- Fix mode (not status)
- Position (lat/long/alt)
- Track
- Speed over ground
- Climb
- Estimated error for all values (in order, `ept`, `epy`, `epx`, `epv`, `epd`,
  `eps`, and `epc`)

See the [documentation for the TPV
object](https://gpsd.gitlab.io/gpsd/gpsd_json.html#_tpv) for explanations of the
above fields and their possible values.

# Licensing

This software is licensed only under Version 2 of the GNU Public License
(GPL).  It is provided for Government use with Restricted Rights, as defined
in the FAR, and remains the property of Fortian.

This software incorporates:

- previously unpublished property of Fortian, which is not public domain and
  remains the property of Fortian

- software derived from `gpsLogger`, which, under 17 USC &sect;105, is
  public domain software

---

Fortian is a registered trademark of Fortian Inc.
