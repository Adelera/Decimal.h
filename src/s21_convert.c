#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "s21_common.h"
#include "s21_decimal.h"

#define S21_DEC_MAX_MANTISSA 7.9228162514264337593543950335e28
#define S21_DEC_MIN 5e-29
#define M_2_32 4294967296.0
#define M_2_64 18446744073709551616.0

int s21_from_decimal_to_float(s21_decimal src, float *dst) {
  if (!dst) return 1;

  unsigned int scale = ((unsigned int)src.bits[3] >> 16) & 0xFFu;
  unsigned int sign = ((unsigned int)src.bits[3] >> 31) & 1u;
  double mantissa = (unsigned int)src.bits[0] +
                    (unsigned int)src.bits[1] * M_2_32 +
                    (unsigned int)src.bits[2] * M_2_64;

  for (int i = scale; i > 0; i--) {
    mantissa /= 10;
  }

  if (sign) {
    mantissa = -mantissa;
  }

  *dst = (float)mantissa;
  return 0;
}

static void s21_parse_float_str(const char *buf, long *mantissa_out,
                                int *exp_out) {
  long mantissa = 0;
  int exp = 0;
  int i = 0;

  if (buf[i] == '-' || buf[i] == '+') i++;

  while (buf[i] >= '0' && buf[i] <= '9') {
    mantissa = mantissa * 10 + (buf[i] - '0');
    i++;
  }

  if (buf[i] == '.') i++;

  while (buf[i] >= '0' && buf[i] <= '9') {
    mantissa = mantissa * 10 + (buf[i] - '0');
    i++;
  }

  if (buf[i] == 'e' || buf[i] == 'E') {
    i++;
    int exp_sign = 1;
    if (buf[i] == '-') {
      exp_sign = -1;
      i++;
    } else if (buf[i] == '+') {
      i++;
    }

    while (buf[i] >= '0' && buf[i] <= '9') {
      exp = exp * 10 + (buf[i] - '0');
      i++;
    }
    exp *= exp_sign;
  }

  *mantissa_out = mantissa;
  *exp_out = exp;
}

int s21_from_float_to_decimal(float src, s21_decimal *result) {
  if (!result) return 1;
  s21_zero(result);

  if (isnan(src) || isinf(src)) return 1;
  if (src == 0.0f) return 0;

  int sign = signbit(src) ? 1 : 0;
  float abs_src = fabsf(src);

  if ((double)abs_src < S21_DEC_MIN) return 1;
  if ((double)abs_src > S21_DEC_MAX_MANTISSA) return 1;

  char buf[32];
  snprintf(buf, sizeof(buf), "%.7e", (double)abs_src);

  long mantissa = 0;
  int exp = 0;
  s21_parse_float_str(buf, &mantissa, &exp);

  int scale = 7 - exp;

  if (scale < 0) {
    while (scale < 0) {
      if (mantissa > (long)9.2e18 / 10) return 1;
      mantissa *= 10;
      scale++;
    }
  } else if (scale > 28) {
    while (scale > 28) {
      mantissa /= 10;
      scale--;
    }
    if (mantissa == 0) return 1;
  }

  if (exp >= 0) {
    while (scale > 0 && mantissa % 10 == 0) {
      mantissa /= 10;
      scale--;
    }
  }

  if ((double)mantissa > S21_DEC_MAX_MANTISSA) return 1;

  result->bits[0] = (int)((unsigned long)mantissa & 0xFFFFFFFFu);
  result->bits[1] = (int)(((unsigned long)mantissa >> 32) & 0xFFFFFFFFu);
  result->bits[2] = 0;

  s21_set_scale(result, scale);
  s21_set_sign(result, sign);

  return 0;
}

int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  if (!dst) return 1;
  memset(dst, 0, sizeof(s21_decimal));

  if (src == 0) return 0;

  unsigned int sign = 0;
  unsigned int value = src;

  if (src < 0) {
    sign = 1;
    value = (unsigned int)(-(long long)src);
  }

  dst->bits[0] = value;
  dst->bits[1] = 0;
  dst->bits[2] = 0;
  dst->bits[3] = 0;

  if (sign) dst->bits[3] |= (1u << 31);

  return 0;
}

int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  if (!dst) return 1;

  if (src.bits[2] != 0) return 1;

  uint64_t mantissa =
      (uint64_t)(unsigned)src.bits[0] | ((uint64_t)(unsigned)src.bits[1] << 32);

  unsigned int scale = ((unsigned int)src.bits[3] >> 16) & 0xFFu;
  if (scale > 28) return 1;

  unsigned int sign = ((unsigned int)src.bits[3] >> 31) & 1u;

  for (unsigned int i = 0; i < scale; i++) {
    mantissa /= 10;
  }
  const uint64_t max_pos = 2147483647ULL;
  const uint64_t max_neg = 2147483648ULL;

  if (sign == 0) {
    if (mantissa > max_pos) return 1;
    *dst = (int)mantissa;
  } else {
    if (mantissa > max_neg) return 1;
    if (mantissa == max_neg) {
      *dst = INT_MIN;
    } else {
      *dst = -(int)mantissa;
    }
  }

  return 0;
}