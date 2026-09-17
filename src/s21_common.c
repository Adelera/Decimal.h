#include "s21_common.h"

#include <stdint.h>

#include "s21_decimal.h"

int s21_get_scale(s21_decimal value) {
  return ((uint32_t)value.bits[3] >> 16) & 0xFF;
}

void s21_zero(s21_decimal *value) {
  if (value) {
    value->bits[0] = 0;
    value->bits[1] = 0;
    value->bits[2] = 0;
    value->bits[3] = 0;
  }
}

int s21_is_zero(s21_decimal value) {
  return value.bits[0] == 0 && value.bits[1] == 0 && value.bits[2] == 0;
}

int s21_get_sign(s21_decimal value) { return (value.bits[3] >> 31) & 1; }

void s21_set_sign(s21_decimal *value, int sign) {
  if (value) {
    if (sign) {
      value->bits[3] |= (1u << 31);
    } else {
      value->bits[3] &= ~(1u << 31);
    }
  }
}

void s21_set_scale(s21_decimal *value, int scale) {
  if (value) {
    value->bits[3] = (value->bits[3] & ~(0xFF << 16)) | (scale << 16);
  }
}