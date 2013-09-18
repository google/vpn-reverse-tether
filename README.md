# VPN-based USB "reverse tether"

Reverse tethering routes traffic from the device via host gateway over USB.
This allows the Android device to access the network through the host without
depending on Wifi or Bluetooth.

This method leverages VpnService API and **does not require root access**.

## Technical details

    +-----------------------+                  +------------------------+
    |    Android Device     |                  |       Linux Host       |
    |-----------------------|                  |------------------------|
    |                       | Android          |                        |
    |         ^ iptables    | VpnService       |          ^ iptables    |
    |         |             |                  |          |             |
    |     +--------+        |                  |      +--------+        |
    |     |  tun   |  ---------                |      |  tun   |  ---------
    |     +--------+        |  \               |      +--------+        |  \
    |         ^             |                  |          ^             |
    |         | forward()   | TetherService    |          | forward()   | forwarder
    |         v             |                  |          v             |
    |    +----------+       |  /               |   +---------------+    |  /
    |    | unix sock| ---------                |   | TCP/unix sock | ------
    |    +----------+       |                  |   +---------------+    |
    |         |             |                  |          |             |
    |         |             |                  |          |             |
    |     +--------+        |                  |      +--------+        |
    |     |  adbd  |        |                  |      |  adb   |        |
    |     +--------+        |                  |      +--------+        |
    |         +             |                  |          +             |
    |         |             |                  |          |             |
    +---------|-------------+                  +----------|-------------+
              |                                           |
              |                                           |
              |     adb forward XXX localabstract:YYY     |
              +-------------------------------------------+

## Installation

### ant

0. Get Android SDK and NDK. Using the SDK manager, get build tools.

1. Build the native library:

        $(ANDROID_NDK)/ndk-build

2. Build the apk and install it:

        ANDROID_HOME=$(ANDROID_SDK) ant debug install

3. Build the host-side forwarder:

        make -C jni

### Eclipse

0. Get Android Development Tools and Android Native Development Tools.

1. Import existing project into workspace and build.

2. Build the host-side forwarder:

        make -C jni

## Usage

0. Start the VPN service:

        ./run.sh setup  # You will be asked for sudo password.

    Wait for the alert dialog.

1. Start the tunnel:

        ./run.sh run  # You will be asked for sudo password.

    You should see a key icon in the status bar.

2. When done, press Enter to stop the tether.

## License

See [LICENSE](LICENSE).
