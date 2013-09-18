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

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

struct buffer {
  size_t size;
  size_t progress;
  char data[4096];
};

// > 0, done
// = 0, repeat
// < 0, error
int read_stream(int fd, struct buffer* buf) {
  if (!buf->size) {
    int num_read = read(fd, buf->data + buf->progress,
                        sizeof(int16_t) - buf->progress);
    if (num_read < 0)
      return (errno == EAGAIN) ? 0 : -1;
    buf->progress += num_read;
    if (buf->progress >= sizeof(int16_t)) {
      buf->progress = 0;
      buf->size = ntohs(*(int16_t*)(buf->data));
      if (buf->size == 0)
        return -1;
    }
  }
  if (buf->size && (buf->size - buf->progress) > 0) {
    int num_read = read(fd, buf->data + buf->progress,
                        buf->size - buf->progress);
    if (num_read < 0)
      return (errno == EAGAIN) ? 0 : -1;
    buf->progress += num_read;
  }
  return buf->progress == buf->size;
}

// input buf includes header
int write_stream(int fd, struct buffer* buf) {
  if (buf->progress < buf->size) {
    int num_written = write(fd, buf->data + buf->progress,
                            buf->size - buf->progress);
    if (num_written < 0)
      return (errno == EAGAIN) ? 0 : -1;
    buf->progress += num_written;
  }
  return buf->progress == buf->size;
}

// output buf includes header
int read_datagram(int fd, struct buffer* buf) {
  int num_read = read(fd, buf->data + sizeof(int16_t),
                      sizeof(buf->data) - sizeof(int16_t));
  if (num_read < 0)
    return (errno == EAGAIN) ? 0 : -1;
  *(int16_t*)(buf->data) = htons(num_read);
  buf->size = num_read + sizeof(int16_t);
  buf->progress = 0;
  return 1;
}

int write_datagram(int fd, struct buffer* buf) {
  int num_written = write(fd, buf->data, buf->size);
  if (num_written < 0)
    return (errno == EAGAIN) ? 0 : -1;
  buf->progress = buf->size = 0;
  return 1;
}

void forward(int control_fd, int stream_fd, int datagram_fd) {
  int fds[] = { control_fd, stream_fd, datagram_fd };
  int nfds = 0;
  for (int i = 0; i < 3; ++i) {
    if (fcntl(fds[i], F_SETFL, fcntl(fds[i], F_GETFL, 0) | O_NONBLOCK) != 0)
      goto exit;
    if (fds[i] > nfds)
      nfds = fds[i];
  }
  nfds += 1;

  struct buffer stream_buf;   // stream_fd -> stream_buf -> datagram_fd
  struct buffer datagram_buf; // datagram_fd -> datagram_buf -> stream_fd
  int have_stream = 0;
  int have_datagram = 0;
  fd_set fds_read;
  fd_set fds_write;

  FD_ZERO(&fds_read);
  FD_ZERO(&fds_write);

  while (1) {
    FD_SET(control_fd, &fds_read);

    if (have_stream) {
      FD_CLR(stream_fd, &fds_read);
      FD_SET(datagram_fd, &fds_write);
    } else {
      FD_SET(stream_fd, &fds_read);
      FD_CLR(datagram_fd, &fds_write);
    }

    if (have_datagram) {
      FD_CLR(datagram_fd, &fds_read);
      FD_SET(stream_fd, &fds_write);
    } else {
      FD_SET(datagram_fd, &fds_read);
      FD_CLR(stream_fd, &fds_write);
    }

    int n = select(nfds, &fds_read, &fds_write, NULL, NULL);
    if (FD_ISSET(control_fd, &fds_read)) {
      goto exit;
    }

    if (FD_ISSET(stream_fd, &fds_read)) {
      int rv = read_stream(stream_fd, &stream_buf);
      if (rv < 0)
        goto exit;
      if (rv > 0)
        have_stream = 1;
    }

    if (FD_ISSET(stream_fd, &fds_write)) {
      int rv = write_stream(stream_fd, &datagram_buf);
      if (rv < 0)
        goto exit;
      if (rv > 0)
        have_datagram = 0;
    }

    if (FD_ISSET(datagram_fd, &fds_read)) {
      int rv = read_datagram(datagram_fd, &datagram_buf);
      if (rv < 0)
        goto exit;
      if (rv > 0)
        have_datagram = 1;
    }

    if (FD_ISSET(datagram_fd, &fds_write)) {
      int rv = write_datagram(datagram_fd, &stream_buf);
      if (rv < 0)
        goto exit;
      if (rv > 0)
        have_stream = 0;
    }
  }

exit:
  close(stream_fd);
  close(datagram_fd);
  close(control_fd);
}
