#!/bin/bash

# Set defaults:
: ${LOCAL_PORT:=7890}
: ${ANDROID_SOCK:=vpntether}

: ${EXT_DEV:=eth0}
: ${TUN_DEV:=tun0}

: ${DNS:=8.8.8.8}

: ${SESSION_NAME:=$USER}  # Useful only for the notification.

: ${MODE:=unix}

case $MODE in
  tcp)
LOCAL_ADDR=$LOCAL_PORT
LOCAL_ADDR_FORWARD=tcp:$LOCAL_PORT
  ;;
  unix)
LOCAL_ADDR="$ANDROID_SOCK"
LOCAL_ADDR_FORWARD=localabstract:$ANDROID_SOCK
  ;;
esac

# export ADB=/path/to/sdk/adb for custom adb
ADB="${ADB:-adb} ${SERIAL:+-s $SERIAL}"

MTU=`cat /sys/class/net/$EXT_DEV/mtu`

TUN_IP=10.0.0.2

setup() {
  sudo ip tuntap add dev $TUN_DEV mode tun

  # NAT mode
  sudo sysctl -w net.ipv4.ip_forward=1
  sudo iptables -t nat -F
  sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/8 -o $EXT_DEV -j MASQUERADE
  sudo iptables -P FORWARD ACCEPT
  sudo ifconfig $TUN_DEV 10.0.0.1 dstaddr 10.0.0.2 mtu $MTU up

  # TODO(szym): BRIDGE mode
  # sudo brctl addbr $BRIDGE
  # sudo brctl setfd $BRIDGE 0
  # sudo ifconfig $EXT_DEV 0.0.0.0
  # sudo brctl addif $BRIDGE $EXT_DEV

  $ADB forward $LOCAL_ADDR_FORWARD localabstract:$ANDROID_SOCK
  $ADB shell am start -n com.google.android.vpntether/vpntether.StartActivity -e SOCKNAME $ANDROID_SOCK
}

run() {
  sudo jni/forwarder $TUN_DEV $LOCAL_ADDR -m $MTU -a $TUN_IP 32 -d $DNS -r 0.0.0.0 0 -n $SESSION_NAME
}

case $1 in
  setup|run) $1;;
  *) echo "Usage: $0 setup|run"
esac


