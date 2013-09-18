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

// Forwards packets between a stream-like and a datagram-like file descriptor.
// On the stream-like file, each packet has a 2-byte header encoding length in
// big-endian. On the datagram-like file, there are no headers.
//
// This call blocks until data arrives at control_fd or an error occurs.
// Always closes all the descriptors.
void forward(int control_fd, int stream_fd, int datagram_fd);
