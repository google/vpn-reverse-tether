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

#include <jni.h>
#include <errno.h>

#include "forwarder.h"

JNIEXPORT jint JNICALL
Java_vpntether_TetherService_forward(
  JNIEnv* env, jclass klass,
  jint control_fd, jint fd1, jint fd2)
{
  errno = 0;
  forward((int)control_fd, (int)fd1, (int)fd2);
  return (jint)errno;
}
