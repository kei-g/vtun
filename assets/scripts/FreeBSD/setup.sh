#!/bin/sh

epair_create() {
	local bridge=$1
	local name=$2
	local addr=$3
	local dev_a=`ifconfig epair create`
	local dev_b=`echo -n $dev_a | sed -r 's/(epair[0-9]+)a/\1b/'`
	ifconfig $bridge addm $dev_a >&2
	ifconfig $dev_a up >&2
	ifconfig $dev_b vnet $name >&2
	jexec $name ifconfig $dev_b inet $addr up >&2
	echo $dev_a
}

extract() {
	local dest=$1
	local release=$2
	local host=ftp`tr -dc [1-9] < /dev/urandom | head -c1`.jp.freebsd.org
	local arch=`uname -m`/`uname -p`
	local release=`uname -r | cut -d'-' -f-2`
	local path=pub/FreeBSD/releases/$arch/$release/base.txz
	local targeturl=ftp://anonymous@$host/$path
	curl -fL $targeturl -o - | tar -C $dest -Jxf -
	cp /etc/localtime $dest/etc/
	{
		echo clear_tmp_enable="YES"
		echo cron_enable="NO"
		echo dumpdev="NO"
		echo gateway_enable="YES"
		echo sendmail_enable="NONE"
		echo syslogd_enable="NO"
	} > $dest/etc/rc.conf
	echo "nameserver 8.8.8.8" > $dest/etc/resolv.conf
}

zfs_clone() {
	local name=$1
	zfs clone -o compression=gzip-9 jail/au@base jail/$name
}

md=`mdconfig -a -t malloc -s 512m`

zpool create -O atime=off jail /dev/$md
zfs create -o compression=gzip-9 jail/au

extract /jail/au 13.0-RELEASE

bridge=`ifconfig bridge create`
ifconfig $bridge up

service jail onestart au
ep=`epair_create $bridge au 192.168.0.2/29`
ifconfig $ep inet 192.168.0.1/29 up
jexec au route add default 192.168.0.1
jexec au freebsd-update fetch install
jexec au pkg
jexec au pkg update
ifconfig $ep destroy
service jail onestop au

zfs snapshot -r jail/au@base
for name in au dc sb; do
	[ $name = "au" ] || zfs_clone $name
	for i in `seq 7`; do
		zfs_clone $name-$i
	done
done

service jail onestart

printf "au 2\ndc 3\nsb 4\n" | while read name addr; do
	epair_create $bridge $name 192.168.0.$addr/29
	jexec $name route add default 192.168.0.1
	br=`ifconfig bridge create`
	ifconfig $br name $name-bridge up
	netaddr=`expr $addr \+ 2`
	epair_create $name-bridge $name 192.168.$netaddr.1/24
	for i in `seq 7`; do
		ifaddr=192.168.$netaddr.`expr $i \+ 1`
		epair_create $name-bridge $name-$i $ifaddr/24
		jexec $name-$i route add default 192.168.$netaddr.1
	done
done
