#!/bin/sh

thekext=./build/Deployment/DWXboxHIDDriver.kext

/usr/sbin/chown -R root:wheel $thekext
find $thekext -type f -exec chmod 644 "{}" ";"
find $thekext -type d -exec chmod 755 "{}" ";"
