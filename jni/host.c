// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance  with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// Author: szym@google.com (Szymon Jakubczak)

// Host-side runner forwards traffic between adb-forwarded TCP socket and the
// local tun interface.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "forwarder.h"

int get_interface(char* name) {
  int interface = open("/dev/net/tun", O_RDWR | O_NONBLOCK);

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

  if (ioctl(interface, TUNSETIFF, &ifr)) {
    perror("Cannot get TUN interface");
    return -1;
  }

  return interface;
}

int get_tunnel(int port) {
  int tunnel = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (connect(tunnel, (struct sockaddr*) &addr, sizeof(addr))) {
    perror("Connect to tunnel");
    return -1;
  }

  return tunnel;
}

int get_local_tunnel(char* name) {
  int tunnel = socket(PF_LOCAL, SOCK_STREAM, 0);

  struct sockaddr_un addr = {};
  addr.sun_family = AF_LOCAL;

  addr.sun_path[0] = '\0';
  strncpy(addr.sun_path + 1, name, sizeof(addr.sun_path) - 1);

  int addrlen = strlen(name);
  if (addrlen > sizeof(addr.sun_path) - 1) addrlen = sizeof(addr.sun_path) - 1;
  addrlen += 1 + sizeof(addr.sun_family);

  if (connect(tunnel, (struct sockaddr*) &addr, addrlen)) {
    perror("Connect to tunnel");
    return -1;
  }

  return tunnel;
}

// Concatenates params from commnand line ("-m 1500 -a 10.0.0.2 32 ...") into
// something easier to parse in Java ("m,1500 a,10.0.0.2,32 ...").
// Returns length of written parameters.
int build_parameters(char* parameters, int size, int argc, char* argv[]) {
  int offset = 0;
  for (int i = 0; i < argc; ++i) {
    char *parameter = argv[i];
    int length = strlen(parameter);
    char delimiter = ',';

    // If it looks like an option, prepend a space instead of a comma.
    if (length == 2 && parameter[0] == '-') {
      ++parameter;
      --length;
      delimiter = ' ';
    }

    if (offset + length + 1 >= size) {
      puts("Parameters are too large!");
      return -1;
    }

    if (i > 0) {
      parameters[offset] = delimiter;
      ++offset;
    }

    memcpy(&parameters[offset], parameter, length);
    offset += length;
  }

  return offset;
}

//-----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: %s <tunN> <port|path> options...\n"
        "\n"
        "Options:\n"
        "  -s <session name>\n"
        "  -m <MTU> for the maximum transmission unit\n"
        "  -a <address> <prefix-length> for the private address\n"
        "  -r <address> <prefix-length> for the forwarding route\n"
        "  -d <address> for the domain name server\n"
        "  -s <domain> for the search domain\n", argv[0]);
    return 2;
  }

  char parameters[1024];
  int16_t param_length = build_parameters(parameters, sizeof(parameters), argc - 3, argv + 3);
  if (param_length < 0)
    return 2;

  int interface = get_interface(argv[1]);
  if (interface < 0)
    return 1;

  int tunnel = -1;
  int port = atoi(argv[2]);
  if (port) {
    tunnel = get_tunnel(port);
  } else {
    tunnel = get_local_tunnel(argv[2]);
  }
  if (tunnel < 0)
    return 1;

  puts("Connected.");

  int16_t param_length_be = htons(param_length);
  send(tunnel, &param_length_be, sizeof(param_length_be), 0);
  send(tunnel, parameters, param_length, 0);

  puts("Forwarding. Press Enter to stop.");

  // Keep forwarding packets until something goes wrong.
  forward(STDIN_FILENO, tunnel, interface);

  perror("Finished");
  return 1;
}
