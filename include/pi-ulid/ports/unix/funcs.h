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

#ifndef _PI_ULID_PORT_UNIX_FUNCS_HEADER
#define _PI_ULID_PORT_UNIX_FUNCS_HEADER

#include "pi-ulid.h"

#ifdef __cplusplus
extern "C"
{
#endif

int ulid_port_unix_millis_func(uint64_t *millis);

int ulid_port_unix_random_func(uint8_t buffer[ULID_RANDOM_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* _PI_ULID_PORT_UNIX_FUNCS_HEADER */
