`gpsdLogger` is a [`gpsd`](http://www.catb.org/gpsd/) client which publishes
Position/Location Information (P/LI) to a shared memory segment, compatible
with
[`gpsLogger`](https://downloads.pf.itd.nrl.navy.mil/docs/proteantools/gpsLogger.html)
and [MGEN](https://www.nrl.navy.mil/itd/ncs/products/mgen).  It does not set
time, as it is expected that systems using `gpsdLogger` will also use
[`ntpd`](http://ntp.org/downloads.html) for time management.  (Otherwise,
`gpsLogger`, without the `d`, is a more suitable program, as it manages time
and logs P/LI in a single program.)

It was created to address the situation where sole access to a GPS receiver
could not be ensured, and therefore `gpsd` was used to multiplex GPS data
access.  Although `gpsd` provides timing information suitable for use with
`ntpd`, it does not publish P/LI in a format that MGEN can understand.
Rather than patch MGEN, this utility was designed to emulate `gpsLogger`'s
shared memory publishing, allowing MGEN to automatically acquire P/LI from
`gpsd`.

---

This software is licensed only under Version 2 of the GNU Public License
(GPL).  It is provided for Government use with Restricted Rights, as defined
in the FAR, and remains the property of Fortian.

This software incorporates:

- previously unpublished property of Fortian, which is not public domain and
  remains the property of Fortian

- software derived from `gpsLogger`, which, under 17 USC &sect;105, is
  public domain software

----

Fortian is a registered trademark of Fortian Inc.
