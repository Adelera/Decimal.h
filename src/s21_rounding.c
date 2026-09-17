#include "s21_common.h"
#include "s21_decimal.h"

static int s21_div10(s21_decimal *value);
static int s21_increment(s21_decimal *value);

int s21_floor(s21_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  s21_zero(result);
  int scale = s21_get_scale(value);
  int sign = s21_get_sign(value);

  if (scale == 0) {
    *result = value;
    if (s21_is_zero(*result)) {
      s21_set_sign(result, 0);
    }
    return 0;
  }

  int has_remainder = 0;
  for (int i = 0; i < scale; i++) {
    if (s21_div10(&value) != 0) {
      has_remainder = 1;
    }
  }

  if (sign && has_remainder) {
    s21_increment(&value);
  }

  *result = value;
  s21_set_scale(result, 0);

  if (!s21_is_zero(value)) {
    s21_set_sign(result, sign);
  } else {
    s21_set_sign(result, 0);
  }

  return 0;
}

int s21_truncate(s21_decimal value, s21_decimal *result) {
  if (!result) return 1;

  if (s21_is_zero(value)) {
    result->bits[3] = 0;
    result->bits[2] = 0;
    result->bits[1] = 0;
    result->bits[0] = 0;
  } else {
    unsigned int mantissa[3] = {0};
    mantissa[2] = value.bits[2];
    mantissa[1] = value.bits[1];
    mantissa[0] = value.bits[0];
    unsigned int scale = ((unsigned int)value.bits[3] >> 16) & 0xFFu;

    if (scale > 28) return 1;

    unsigned long long remainder = 0;

    for (int i = scale; i > 0; i--) {
      remainder = 0;

      for (int j = 2; j >= 0; j--) {
        unsigned long long current =
            remainder * 4294967296ULL + mantissa[j];  // 4294967296ULL это 2^32
        remainder = current % 10;
        mantissa[j] = current / 10;
      }
    }

    result->bits[3] = value.bits[3] & 0x80000000;
    result->bits[2] = mantissa[2];
    result->bits[1] = mantissa[1];
    result->bits[0] = mantissa[0];
  }

  return 0;
}

int s21_negate(s21_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  *result = value;

  if (s21_is_zero(*result)) {
    s21_set_sign(result, 0);
    return 0;
  }

  s21_set_sign(result, !s21_get_sign(*result));

  return 0;
}

int s21_round(s21_decimal value, s21_decimal *result) {
  if (!result) return 1;

  unsigned int sign = ((unsigned int)value.bits[3] >> 31) & 1u;
  unsigned int scale = ((unsigned int)value.bits[3] >> 16) & 0xFFu;
  if (scale > 28) return 1;

  s21_decimal abs_value;
  if (sign == 1) {
    s21_negate(value, &abs_value);
  } else {
    abs_value = value;
  }

  if (scale == 0) {
    *result = value;
  } else {
    s21_decimal truncated_val;
    s21_truncate(abs_value, &truncated_val);

    s21_decimal float_part;
    s21_sub(abs_value, truncated_val, &float_part);

    s21_decimal ten = {{10, 0, 0, 0}};
    s21_decimal multiplied_float;
    s21_mul(float_part, ten, &multiplied_float);

    s21_decimal first_num_of_float;
    s21_truncate(multiplied_float, &first_num_of_float);

    s21_decimal five = {{5, 0, 0, 0}};
    if (s21_is_less(first_num_of_float, five)) {
      *result = truncated_val;
    } else {
      s21_decimal rounded_val;
      s21_decimal one = {{1, 0, 0, 0}};
      s21_add(truncated_val, one, &rounded_val);
      *result = rounded_val;
    }
  }

  if (sign == 1) {
    result->bits[3] |= 0x80000000;  // Включаем старший бит знака (минус)
  } else {
    result->bits[3] &= 0x7FFFFFFF;  // Выключаем старший бит знака (плюс)
  }

  return 0;
}

static int s21_div10(s21_decimal *value) {
    if (value == NULL) {
        return 0;
    }

    uint32_t remainder = 0;

    for (int i = 2; i >= 0; --i) {
        uint64_t current = ((uint64_t)remainder << 32) | (uint32_t)value->bits[i];

        value->bits[i] = (int)(current / 10u);
        remainder = (uint32_t)(current % 10u);
    }

  return (int)remainder;
}

static int s21_increment(s21_decimal *value) {
  if (value == NULL) {
    return 1;
  }

  for (int i = 0; i < 4; ++i) {
    ++value->bits[i];

    if (value->bits[i] != 0) {
      return 0;
    }
  }

  return 1;
}