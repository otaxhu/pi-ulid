/*
  Platform-Independant ULID C Library.

  Portable to any platform through providing a "millis_func" and "random_func" to context structs

  See /include/pi-ulid/ports/ you will find implementations of those functions for the most popular
  operative systems (Linux, MacOS, Windows)
*/

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

#ifndef _PI_ULID_HEADER
#define _PI_ULID_HEADER

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ULID_RANDOM_SIZE 10 // Size of random part of ULID
#define ULID_MILLIS_SIZE 6  // Size of milliseconds part of ULID

#define ULID_SIZE     16 // Size of ULID in raw format
#define ULID_SIZE_STR 26 // Size of ULID in string format. Not including NUL terminator

/**
 * @param millis Pointer to int64_t where the current time in Unix time format should be written to
 *               in milliseconds
 *
 * @returns Error code, 0 if there was no error (millis has the current time in milliseconds),
 *          everything else is error
*/
typedef int (*ulid_millis_func_t)(uint64_t *millis);

/**
 * @param buffer Buffer of length ULID_RANDOM_SIZE where random values should be written to
 *
 * @returns Error code, 0 if there was no error (buffer has random values inside of it), everything
 *          else is error. Not filling all of the buffer with random values should be considered
 *          an error
*/
typedef int (*ulid_random_func_t)(uint8_t buffer[ULID_RANDOM_SIZE]);

/**
 * Structure to be used as configuration and as context for some of the functions here defined.
 *
 * You need to set "millis_func", "random_func" and "is_monotonic" to desired values, "millis_func"
 * and "random_func" are required to be set.
 *
 * @note This structure needs to be zero-initialized before using it if you are using monotonic
 *       configuration (is_monotonic is set to true).
*/
struct ulid_ctx
{
  ulid_millis_func_t millis_func;  // Configurable param
  ulid_random_func_t random_func;  // Configurable param
  int                is_monotonic; // Configurable param

  /*
    PRIVATE FIELDS BELOW DO NOT USE!
  */

  uint64_t _PRIVATE_last_time;
  struct uint80_t {
    uint16_t hi;
    uint64_t lo;
  } _PRIVATE_last_random;
};

/**
 * @param ctx Pointer to context structure
 * @param buffer Buffer of length ULID_SIZE where a new ULID will be written to
 *
 * @returns Error code, 0 if there was no error (buffer contains valid ULID), everything else is
 *          error. Errors should be caused by a bad context (NULL context, bad settings), or
 *          failure of millis_func or random_func functions
*/
int ulid_new(struct ulid_ctx *ctx, uint8_t buffer[ULID_SIZE]);

/**
 * @param ctx Pointer to context structure
 * @param buffer Buffer of length ULID_SIZE + 1 where a new ULID in string format will be written
 *               to, NUL terminator is added at the end of the string
 *
 * @returns Error code, 0 if there was no error (buffer contains valid ULID), everything else is
 *          error. Errors should be caused by a bad context (NULL context, bad settings), or
 *          failure of millis_func or random_func functions
*/
int ulid_new_string(struct ulid_ctx *ctx, char buffer[ULID_SIZE_STR + 1]);

/**
 * @param in Buffer of length ULID_SIZE, containing a ULID in raw format
 * @param out Buffer of length ULID_SIZE_STR + 1 where the ULID in string format is gonna be
 *            written to, NUL terminator is added at the end of the string
*/
void ulid_to_string(const uint8_t in[ULID_SIZE], char out[ULID_SIZE_STR + 1]);

/**
 * @param in Buffer of length ULID_SIZE_STR, containing a ULID in string format
 * @param out Buffer of length ULID_SIZE where the ULID in raw format is gonna be written to, NUL
 *            terminator is added at the end of the string
 *
 * @returns Error code, 0 if there was no error ("out" contains "in" converted to raw format),
 *          everything else is error. Errors should be caused by a bad param (NULL pointer,
 *          invalid ULID)
*/
int ulid_from_string(const char in[ULID_SIZE_STR], uint8_t out[ULID_SIZE]);

/**
 * @param ulid_out Buffer of length ULID_SIZE where the parsed ULID is gonna be written to
 * @param opt_millis (Optional parameter) if specified, the value is gonna be written to
 *                   "ulid_out" from index 0 to ULID_MILLIS_SIZE - 1
 * @param opt_random (Optional parameter) if specified, the value is gonna be written to
 *                   "ulid_out" from index ULID_MILLIS_SIZE to ULID_SIZE - 1
 *
 * @returns Error code, 0 if there was no error ("opt_millis" and/or "opt_random" were written to
 *          "ulid_out"), everything else is error. Errors should be caused by a bad param (NULL pointer,
 *          )
*/
int ulid_parse(uint8_t ulid_out[ULID_SIZE],
               const uint64_t *opt_millis,
               const uint8_t opt_random[ULID_RANDOM_SIZE]);

/**
 * 
*/
int ulid_unparse(const uint8_t ulid[ULID_SIZE],
                 uint64_t *opt_millis_out,
                 uint8_t opt_random_out[ULID_RANDOM_SIZE]);

/**
 * @returns Negative value if ulid1 is lesser than ulid2 in lexicographical order.
 *          0 if ulid1 and ulid2 are equal each other.
 *          Positive value if ulid1 is greater than ulid2 in lexicographical order.
*/
int ulid_compare(const uint8_t ulid1[ULID_SIZE], const uint8_t ulid2[ULID_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* _PI_ULID_HEADER */
