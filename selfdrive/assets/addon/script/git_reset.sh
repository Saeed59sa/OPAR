#!/usr/bin/bash

export LD_LIBRARY_PATH=/data/data/com.termux/files/usr/lib
export HOME=/data/data/com.termux/files/home
export PATH=/usr/local/bin:/data/data/com.termux/files/usr/bin:/data/data/com.termux/files/usr/sbin:/data/data/com.termux/files/usr/bin/applets:/bin:/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin:/data/data/com.termux/files/usr/bin/python
export PYTHONPATH=/data/openpilot

cd /data/openpilot
ping -q -c 1 -w 1 google.com &> /dev/null
if [ "$?" == "0" ]; then
  CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
  /data/data/com.termux/files/usr/bin/git clean -d -f -f
  /data/data/com.termux/files/usr/bin/git fetch --all
  /data/data/com.termux/files/usr/bin/git reset --hard origin/$CURRENT_BRANCH
  /data/data/com.termux/files/usr/bin/git pull

  if [ -f "/data/openpilot/prebuilt" ]; then
    pkill -f thermald
    rm -f /data/openpilot/prebuilt
  fi

  reboot
fi