ARCH=$(shell uname -m)
ifeq ($(ARCH),x86_64)
    ARCH=amd64
endif

.PHONY: gpsdlogger_1.1.1-1_$(ARCH).deb clean

gpsdlogger_1.1.1-1_$(ARCH).deb: ../gpsdlogger_1.1.1-1_$(ARCH).deb

control/md5sums: data/usr/bin/gpsdlogger
	cd data && find usr -type f -print0 | grep -v CVS | xargs -0 md5sum > ../$@

control/control: control.in data/usr/bin/gpsdlogger
	sed 's/@ARCH@/$(ARCH)/;s/@SIZE@/$(shell du -k data/usr/bin/gpsdlogger | cut -f 1)/' control.in > control/control

control.tar.gz: control/control control/md5sums
	mkdir -p control
	ssh -x -l root localhost chown -vR root:root ${PWD}/debian/control
	cd control && tar --exclude CVS -cvpzf ../$@ .
	ssh -x -l root localhost chown -vR $(USER):users ${PWD}/debian/control

data.tar.gz: data/usr/bin/gpsdlogger
	mkdir -p data/usr/bin
	ssh -x -l root localhost chown -vR root:root ${PWD}/debian/data
	cd data && tar --exclude CVS -cvpzf ../$@ .
	ssh -x -l root localhost chown -vR $(USER):users ${PWD}/debian/data

../gpsdlogger_1.1.1-1_$(ARCH).deb: debian-binary control.tar.gz data.tar.gz
	ar cr $@ $^

clean:
	rm -f control/control control/md5sums control.tar.gz data.tar.gz \
	    data/usr/bin/gpsdlogger
