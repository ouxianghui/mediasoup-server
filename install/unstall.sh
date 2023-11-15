#!/bin/bash

if [[ -d /usr/lib/systemd/system ]]; then
  systemctl disable sfu
  systemctl stop sfu
  rm -f /usr/lib/systemd/system/sfu.service
  rm -f /etc/init.d/sfu
else
  /sbin/chkconfig sfu off
  /sbin/chkconfig --del sfu
  /etc/init.d/sfu stop
  rm -f /etc/init.d/sfu
fi
rm -rf /usr/local/sfu
echo "SFU uninstalled"
