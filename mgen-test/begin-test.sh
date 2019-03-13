#!/bin/sh

pkill mgen

mgen log mgen-1.logfile instance 1 >mgen-1.out 2>mgen-1.err &

mgen instance 1 input mgen.conf
