/*
  Copyright 2024 Oscar Pernia

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "pi-ulid/ports/unix/funcs.h"
#include <stdio.h>

#include <time.h>

int ulid_port_unix_millis_func(uint64_t *millis)
{
  struct timespec t;

  int ret;

  if (ret = clock_gettime(CLOCK_REALTIME, &t))
    return ret;

  *millis = t.tv_sec * 1000 + t.tv_nsec / 1000000;

  return ret;
}

int ulid_port_unix_random_func(uint8_t buffer[ULID_RANDOM_SIZE])
{
  FILE *f = fopen("/dev/urandom", "rb");

  if (!f)
    return 1;

  int ret = fread(buffer, ULID_RANDOM_SIZE, 1, f);
  fclose(f);

  return !ret;
}
