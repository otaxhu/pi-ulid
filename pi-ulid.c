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

#include "pi-ulid.h"
#include <string.h>

#define VALIDATE_CTX_OR_RET(ctx, ret)                       \
  if (!(ctx) || !(ctx)->millis_func || !(ctx)->random_func) \
    return (ret)                                            \

// Crockford's base32 table, ASCII characters maps to its corresponding value in Crockford's
// base32, -1 means an invalid ASCII character
static const uint8_t ascii_to_b32_table[0x100] = {
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

  // 0-9
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09,   -1,   -1,   -1,   -1,   -1,   -1,

  // Upper letters
    -1, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
  0x11,   -1, 0x12, 0x13,   -1, 0x14, 0x15,   -1,
  0x16, 0x17, 0x18, 0x19, 0x1A,   -1, 0x1B, 0x1C,
  0x1D, 0x1E, 0x1F,   -1,   -1,   -1,   -1,   -1,

  // Lower letters
    -1, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
  0x11,   -1, 0x12, 0x13,   -1, 0x14, 0x15,   -1,
  0x16, 0x17, 0x18, 0x19, 0x1A,   -1, 0x1B, 0x1C,
  0x1D, 0x1E, 0x1F,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};

static const char b32_chars[] = {'0','1','2','3','4','5','6','7','8','9',
                                 'A','B','C','D','E','F','G','H','J','K',
                                 'M','N','P','Q','R','S','T','V','W','X','Y','Z'};

#define UINT80_ADD1(u80, overflows) do { \
    uint16_t hi = (u80).hi;              \
    uint64_t lo = (u80).lo;              \
    if (++(u80).lo < lo)                 \
      ((u80).hi)++;                      \
    (overflows) = (u80).hi < hi;         \
  } while (0)

#define UINT80_IS_ZERO(u80) (!(u80).hi && !(u80).lo)

#define UINT80_WRITE_TO(u80, buf) do {    \
    (buf)[0] = (uint8_t)((u80).hi >> 8);  \
    (buf)[1] = (uint8_t)((u80).hi);       \
    (buf)[2] = (uint8_t)((u80).lo >> 56); \
    (buf)[3] = (uint8_t)((u80).lo >> 48); \
    (buf)[4] = (uint8_t)((u80).lo >> 40); \
    (buf)[5] = (uint8_t)((u80).lo >> 32); \
    (buf)[6] = (uint8_t)((u80).lo >> 24); \
    (buf)[7] = (uint8_t)((u80).lo >> 16); \
    (buf)[8] = (uint8_t)((u80).lo >> 8);  \
    (buf)[9] = (uint8_t)((u80).lo);       \
  } while (0)

#define UINT80_SET_TO(u80, buf) do {          \
    (u80).hi = ((uint16_t)((buf)[0]) << 8) |  \
               (uint16_t)((buf)[1]);          \
    (u80).lo = ((uint64_t)((buf)[2]) << 56) | \
               ((uint64_t)((buf)[3]) << 48) | \
               ((uint64_t)((buf)[4]) << 40) | \
               ((uint64_t)((buf)[5]) << 32) | \
               ((uint64_t)((buf)[6]) << 24) | \
               ((uint64_t)((buf)[7]) << 16) | \
               ((uint64_t)((buf)[8]) << 8) |  \
               (uint64_t)((buf)[9]);          \
  } while (0)

#define MILLIS_WRITE_TO(millis, buf) do { \
    (buf)[0] = (uint8_t)((millis) >> 40); \
    (buf)[1] = (uint8_t)((millis) >> 32); \
    (buf)[2] = (uint8_t)((millis) >> 24); \
    (buf)[3] = (uint8_t)((millis) >> 16); \
    (buf)[4] = (uint8_t)((millis) >> 8);  \
    (buf)[5] = (uint8_t)((millis));       \
  } while (0)

#define MILLIS_SET_TO(millis, buf) \
  (millis) = ((uint64_t)((buf)[0]) << 40) |    \
             ((uint64_t)((buf)[1]) << 32) |    \
             ((uint64_t)((buf)[2]) << 24) |    \
             ((uint64_t)((buf)[3]) << 16) |    \
             ((uint64_t)((buf)[4]) << 8) |     \
             ((uint64_t)((buf)[5]))

static int ulid_new_internal(struct ulid_ctx *ctx, uint8_t buffer[ULID_SIZE])
{
  int ret;
  uint64_t millis;

  if (ret = ctx->millis_func(&millis))
    return ret;

  if (ctx->is_monotonic)
  {
    if (!UINT80_IS_ZERO(ctx->_PRIVATE_last_random) &&
        millis <= ctx->_PRIVATE_last_time)
    {
      UINT80_ADD1(ctx->_PRIVATE_last_random, ret);

      UINT80_WRITE_TO(ctx->_PRIVATE_last_random, buffer + ULID_MILLIS_SIZE);
    } else {
      ret = ctx->random_func(buffer + ULID_MILLIS_SIZE);

      ctx->_PRIVATE_last_time = millis;

      UINT80_SET_TO(ctx->_PRIVATE_last_random, buffer + ULID_MILLIS_SIZE);
    }
  } else {
    ret = ctx->random_func(buffer + ULID_MILLIS_SIZE);
  }

  MILLIS_WRITE_TO(millis, buffer);

  return ret;
}

int ulid_new(struct ulid_ctx *ctx, uint8_t buffer[ULID_SIZE])
{
  VALIDATE_CTX_OR_RET(ctx, 1);

  if (!buffer)
    return 1;

  return ulid_new_internal(ctx, buffer);
}

int ulid_new_string(struct ulid_ctx *ctx, char buffer[ULID_SIZE_STR + 1])
{
  VALIDATE_CTX_OR_RET(ctx, 1);

  if (!buffer)
    return 1;

  int ret;

  uint8_t temp[ULID_SIZE];

  if (ret = ulid_new_internal(ctx, temp))
    return ret;

  ulid_to_string((const uint8_t *)temp, buffer);

  return 0;
}

void ulid_to_string(const uint8_t in[ULID_SIZE], char out[ULID_SIZE_STR + 1])
{
  out[0] = b32_chars[in[0] >> 5];
  out[1] = b32_chars[in[0] & 0x1F];
  out[2] = b32_chars[in[1] >> 3];
  out[3] = b32_chars[((in[1] & 7) << 2) | (in[2] >> 6)];
  out[4] = b32_chars[(in[2] >> 1) & 0x1F];
  out[5] = b32_chars[((in[2] & 1) << 4) | (in[3] >> 4)];
  out[6] = b32_chars[((in[3] & 0xF) << 1) | (in[4] >> 7)];
  out[7] = b32_chars[(in[4] >> 2) & 0x1F];
  out[8] = b32_chars[((in[4] & 3) << 3) | (in[5] >> 5)];
  out[9] = b32_chars[in[5] & 0x1F];

  out[10] = b32_chars[in[6] >> 3];
  out[11] = b32_chars[((in[6] & 7) << 2) | (in[7] >> 6)];
  out[12] = b32_chars[(in[7] >> 1) & 0x1F];
  out[13] = b32_chars[((in[7] & 1) << 4) | (in[8] >> 4)];
  out[14] = b32_chars[((in[8] & 0xF) << 1) | (in[9] >> 7)];
  out[15] = b32_chars[(in[9] >> 2) & 0x1F];
  out[16] = b32_chars[((in[9] & 3) << 3) | (in[10] >> 5)];
  out[17] = b32_chars[in[10] & 0x1F];
  out[18] = b32_chars[in[11] >> 3];
  out[19] = b32_chars[((in[11] & 7) << 2) | (in[12] >> 6)];
  out[20] = b32_chars[((in[12] >> 1) & 0x1F)];
  out[21] = b32_chars[((in[12] & 1) << 4) | (in[13] >> 4)];
  out[22] = b32_chars[((in[13] & 0xF) << 1) | (in[14] >> 7)];
  out[23] = b32_chars[(in[14] >> 2) & 0x1F];
  out[24] = b32_chars[((in[14] & 3) << 3) | (in[15] >> 5)];
  out[25] = b32_chars[in[15] & 0x1F];

  out[26] = '\0';
}

int ulid_from_string(const char in[ULID_SIZE_STR], uint8_t out[ULID_SIZE])
{
  if (in[0] > '7' ||
      ascii_to_b32_table[in[0]] & 0x80 ||
      ascii_to_b32_table[in[1]] & 0x80 ||
      ascii_to_b32_table[in[2]] & 0x80 ||
      ascii_to_b32_table[in[3]] & 0x80 ||
      ascii_to_b32_table[in[4]] & 0x80 ||
      ascii_to_b32_table[in[5]] & 0x80 ||
      ascii_to_b32_table[in[6]] & 0x80 ||
      ascii_to_b32_table[in[7]] & 0x80 ||
      ascii_to_b32_table[in[8]] & 0x80 ||
      ascii_to_b32_table[in[9]] & 0x80 ||
      ascii_to_b32_table[in[10]] & 0x80 ||
      ascii_to_b32_table[in[11]] & 0x80 ||
      ascii_to_b32_table[in[12]] & 0x80 ||
      ascii_to_b32_table[in[13]] & 0x80 ||
      ascii_to_b32_table[in[14]] & 0x80 ||
      ascii_to_b32_table[in[15]] & 0x80 ||
      ascii_to_b32_table[in[16]] & 0x80 ||
      ascii_to_b32_table[in[17]] & 0x80 ||
      ascii_to_b32_table[in[18]] & 0x80 ||
      ascii_to_b32_table[in[19]] & 0x80 ||
      ascii_to_b32_table[in[20]] & 0x80 ||
      ascii_to_b32_table[in[21]] & 0x80 ||
      ascii_to_b32_table[in[22]] & 0x80 ||
      ascii_to_b32_table[in[23]] & 0x80 ||
      ascii_to_b32_table[in[24]] & 0x80 ||
      ascii_to_b32_table[in[25]] & 0x80)
  {
    return 1;
  }

  out[0] = (ascii_to_b32_table[in[0]] << 5) | ascii_to_b32_table[in[1]];
  out[1] = (ascii_to_b32_table[in[2]] << 3) | (ascii_to_b32_table[in[3]] >> 2);
  out[2] = (ascii_to_b32_table[in[3]] << 6) | (ascii_to_b32_table[in[4]] << 1) | (ascii_to_b32_table[in[5]] >> 4);
  out[3] = (ascii_to_b32_table[in[5]] << 4) | (ascii_to_b32_table[in[6]] >> 1);
  out[4] = (ascii_to_b32_table[in[6]] << 7) | (ascii_to_b32_table[in[7]] << 2) | (ascii_to_b32_table[in[8]] >> 3);
  out[5] = (ascii_to_b32_table[in[8]] << 5) | ascii_to_b32_table[in[9]];

  out[6] = (ascii_to_b32_table[in[10]] << 3) | (ascii_to_b32_table[in[11]] >> 2);
  out[7] = (ascii_to_b32_table[in[11]] << 6) | (ascii_to_b32_table[in[12]] << 1) | (ascii_to_b32_table[in[13]] >> 4);
  out[8] = (ascii_to_b32_table[in[13]] << 4) | (ascii_to_b32_table[in[14]] >> 1);
  out[9] = (ascii_to_b32_table[in[14]] << 7) | (ascii_to_b32_table[in[15]] << 2) | (ascii_to_b32_table[in[16]] >> 3);
  out[10] = (ascii_to_b32_table[in[16]] << 5) | ascii_to_b32_table[in[17]];
  out[11] = (ascii_to_b32_table[in[18]] << 3) | (ascii_to_b32_table[in[19]] >> 2);
  out[12] = (ascii_to_b32_table[in[19]] << 6) | (ascii_to_b32_table[in[20]] << 1) | (ascii_to_b32_table[in[21]] >> 4);
  out[13] = (ascii_to_b32_table[in[21]] << 4) | (ascii_to_b32_table[in[22]] >> 1);
  out[14] = (ascii_to_b32_table[in[22]] << 7) | (ascii_to_b32_table[in[23]] << 2) | (ascii_to_b32_table[in[24]] >> 3);
  out[15] = (ascii_to_b32_table[in[24]] << 5) | ascii_to_b32_table[in[25]];

  return 0;
}

int ulid_parse(uint8_t ulid_out[ULID_SIZE],
               const uint64_t *opt_millis,
               const uint8_t opt_random[ULID_RANDOM_SIZE])
{
  if (!opt_millis && !opt_random)
    return 1;

  if (opt_millis)
  {
    MILLIS_WRITE_TO(*opt_millis, ulid_out);
  }

  if (opt_random)
  {
    memcpy(ulid_out + ULID_MILLIS_SIZE, opt_random, ULID_RANDOM_SIZE);
  }

  return 0;
}

int ulid_unparse(const uint8_t ulid[ULID_SIZE],
                 uint64_t *opt_millis_out,
                 uint8_t opt_random_out[ULID_RANDOM_SIZE])
{
  if (!opt_millis_out && !opt_random_out)
    return 1;

  if (opt_millis_out)
  {
    MILLIS_SET_TO(*opt_millis_out, ulid);
  }

  if (opt_random_out)
  {
    memcpy(opt_random_out, ulid + ULID_MILLIS_SIZE, ULID_RANDOM_SIZE);
  }

  return 0;
}

int ulid_compare(const uint8_t ulid1[ULID_SIZE], const uint8_t ulid2[ULID_SIZE])
{
  return memcmp(ulid1, ulid2, ULID_SIZE);
}
