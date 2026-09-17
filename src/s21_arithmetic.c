#include <stdint.h>

#include "s21_common.h"
#include "s21_decimal.h"

static int s21_should_round_up(s21_big_decimal remainder,
                               s21_big_decimal divisor,
                               s21_big_decimal quotient);
static int s21_big_is_zero(s21_big_decimal value);
static int s21_big_mul(s21_big_decimal a, s21_big_decimal b,
                       s21_big_decimal *result);
static void s21_big_div_mod(s21_big_decimal dividend, s21_big_decimal divisor,
                            s21_big_decimal *quotient,
                            s21_big_decimal *remainder);
static int s21_big_compare(s21_big_decimal value_1, s21_big_decimal value_2);
static s21_big_decimal s21_to_big(s21_decimal value);
static int s21_big_add_digit(s21_big_decimal *value, int digit);
static void s21_big_zero(s21_big_decimal *value);
static void s21_from_big(s21_big_decimal value, s21_decimal *result);
static int s21_big_add(s21_big_decimal value_1, s21_big_decimal value_2,
                       s21_big_decimal *result);
static int s21_big_sub(s21_big_decimal value_1, s21_big_decimal value_2,
                       s21_big_decimal *result);
static int s21_align_scales(s21_big_decimal *big_1, s21_big_decimal *big_2,
                            int *scale_1, int *scale_2);
static int s21_big_mul10(s21_big_decimal *value);
static int s21_reduce_result(s21_big_decimal *value, int *scale);
static int s21_bankers_round(s21_big_decimal *value, int remainder, int sticky);
static int s21_big_increment(s21_big_decimal *value);
static void s21_trim_trailing_zeros(s21_big_decimal *value, int *scale);
static int s21_big_div10(s21_big_decimal *value);
static int s21_big_get_bit(s21_big_decimal value, int index);
static int s21_big_set_bit(s21_big_decimal *value, int index, int bit);
static void s21_big_shift_left(s21_big_decimal *value);
static int s21_big_fits_decimal(s21_big_decimal value);

int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) {
    return S21_ERROR;
  }
  s21_zero(result);

  int sign_1 = s21_get_sign(value_1);
  int sign_2 = s21_get_sign(value_2);
  int scale_1 = s21_get_scale(value_1);
  int scale_2 = s21_get_scale(value_2);

  s21_big_decimal big_1 = s21_to_big(value_1);
  s21_big_decimal big_2 = s21_to_big(value_2);

  s21_big_decimal big_result;
  s21_big_zero(&big_result);

  if (s21_align_scales(&big_1, &big_2, &scale_1, &scale_2)) {
    return sign_1 ? S21_NEGATIVE_OVERFLOW : S21_POSITIVE_OVERFLOW;
  }

  if (sign_1 == sign_2) {
    s21_big_add(big_1, big_2, &big_result);
    int scale = scale_1;

    if (s21_reduce_result(&big_result, &scale)) {
      return sign_1 ? S21_NEGATIVE_OVERFLOW : S21_POSITIVE_OVERFLOW;
    }
  } else {
    int comparison = s21_big_compare(big_1, big_2);

    if (comparison == 0) {
      s21_zero(result);
      return S21_OK;
    }

    if (comparison > 0) {
      s21_big_sub(big_1, big_2, &big_result);
    } else {
      s21_big_sub(big_2, big_1, &big_result);
      sign_1 = sign_2;
    }
  }

  s21_from_big(big_result, result);
  s21_set_scale(result, scale_1);
  s21_set_sign(result, sign_1);

  if (s21_is_zero(*result)) {
    s21_set_sign(result, 0);
    s21_set_scale(result, 0);
  }

  return S21_OK;
}
int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }
  int sign = s21_get_sign(value_2);
  s21_set_sign(&value_2, !sign);
  return s21_add(value_1, value_2, result);
}

int s21_mul(s21_decimal value1, s21_decimal value2, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  s21_zero(result);

  if (s21_is_zero(value1) || s21_is_zero(value2)) {
    return 0;
  }

  int sign = s21_get_sign(value1) ^ s21_get_sign(value2);
  int scale = s21_get_scale(value1) + s21_get_scale(value2);

  s21_big_decimal big1 = s21_to_big(value1);
  s21_big_decimal big2 = s21_to_big(value2);
  s21_big_decimal product;
  s21_big_zero(&product);

  if (s21_big_mul(big1, big2, &product)) {
    return sign ? S21_NEGATIVE_OVERFLOW : S21_POSITIVE_OVERFLOW;
  }

  while (scale > 28) {
    s21_big_div10(&product);
    --scale;
  }

  while (!s21_big_fits_decimal(product)) {
    if (scale == 0) {
      return sign ? S21_NEGATIVE_OVERFLOW : S21_POSITIVE_OVERFLOW;
    }
    s21_big_div10(&product);
    --scale;
  }

  s21_from_big(product, result);

  s21_set_scale(result, scale);
  s21_set_sign(result, sign);

  if (s21_is_zero(*result)) {
    s21_set_scale(result, 0);
    s21_set_sign(result, 0);
  }

  return 0;
}

int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  s21_zero(result);

  if (s21_is_zero(value_2)) {
    return 3;
  }

  int sign_1 = s21_get_sign(value_1);
  int sign_2 = s21_get_sign(value_2);
  int result_sign = sign_1 ^ sign_2;

  if (s21_is_zero(value_1)) {
    s21_zero(result);
    return 0;
  }

  s21_big_decimal dividend = s21_to_big(value_1);
  s21_big_decimal divisor = s21_to_big(value_2);

  s21_big_decimal quotient;
  s21_big_decimal remainder;

  s21_big_div_mod(dividend, divisor, &quotient, &remainder);

  int scale_1 = s21_get_scale(value_1);
  int scale_2 = s21_get_scale(value_2);

  int scale_difference = scale_2 - scale_1;

  int fractional_digits;

  if (scale_difference >= 0) {
    fractional_digits = 28;
  } else {
    fractional_digits = 28 + scale_difference;
  }

  int result_scale = fractional_digits - scale_difference;

  for (int i = 0; i < fractional_digits; ++i) {
    if (s21_big_mul10(&remainder)) {
      break;
    }
    s21_big_decimal digit_big;
    s21_big_decimal new_remainder;

    s21_big_div_mod(remainder, divisor, &digit_big, &new_remainder);

    int digit = (int)digit_big.bits[0];

    if (s21_big_mul10(&quotient)) {
      return result_sign ? 2 : 1;
    }

    if (s21_big_add_digit(&quotient, digit)) {
      return result_sign ? 2 : 1;
    }

    remainder = new_remainder;
  }

  if (!s21_big_is_zero(remainder)) {
    if (s21_should_round_up(quotient, remainder, divisor)) {
      if (s21_big_increment(&quotient)) {
        return result_sign ? 2 : 1;
      }
    }
  }

  if (s21_reduce_result(&quotient, &result_scale)) {
    return result_sign ? 2 : 1;
  }

  s21_trim_trailing_zeros(&quotient, &result_scale);
  if (!s21_big_fits_decimal(quotient)) {
    return result_sign ? 2 : 1;
  }

  s21_from_big(quotient, result);

  s21_set_scale(result, result_scale);

  if (!s21_big_is_zero(quotient)) {
    s21_set_sign(result, result_sign);
  } else {
    s21_set_sign(result, 0);
  }

  return 0;
}

static int s21_should_round_up(s21_big_decimal remainder,
                               s21_big_decimal divisor,
                               s21_big_decimal quotient) {
  (void)quotient;
  if (s21_big_is_zero(remainder)) {
    return 0;
  }

  s21_big_decimal doubled_remainder = remainder;

  s21_big_shift_left(&doubled_remainder);

  int comparison = s21_big_compare(doubled_remainder, divisor);

  if (comparison > 0) {
    return 1;
  }

  if (comparison < 0) {
    return 0;
  }
  return (quotient.bits[0] & 1u);
}

static int s21_big_is_zero(s21_big_decimal value) {
  for (int i = 0; i < 6; ++i) {
    if (value.bits[i] != 0) {
      return 0;
    }
  }

  return 1;
}

static int s21_big_mul(s21_big_decimal a, s21_big_decimal b,
                       s21_big_decimal *result) {
  if (result == NULL) return 1;
  s21_big_zero(result);
  for (int i = 0; i < 192; i++) {
    if (s21_big_get_bit(b, i)) {
      s21_big_decimal temp = a;
      for (int j = 0; j < i; j++) {
        s21_big_shift_left(&temp);
      }
      s21_big_decimal new_result;
      s21_big_add(*result, temp, &new_result);
      *result = new_result;
    }
  }
  return 0;
}

static void s21_big_div_mod(s21_big_decimal dividend, s21_big_decimal divisor,
                            s21_big_decimal *quotient,
                            s21_big_decimal *remainder) {
  s21_big_zero(quotient);
  s21_big_zero(remainder);

  for (int i = 191; i >= 0; --i) {
    s21_big_shift_left(remainder);

    if (s21_big_get_bit(dividend, i)) {
      s21_big_set_bit(remainder, 0, 1);
    }

    if (s21_big_compare(*remainder, divisor) >= 0) {
      s21_big_decimal temp;

      s21_big_sub(*remainder, divisor, &temp);

      *remainder = temp;

      s21_big_set_bit(quotient, i, 1);
    }
  }
}

static int s21_big_compare(s21_big_decimal value_1, s21_big_decimal value_2) {
  for (int i = 5; i >= 0; --i) {
    if (value_1.bits[i] > value_2.bits[i]) {
      return 1;
    }

    if (value_1.bits[i] < value_2.bits[i]) {
      return -1;
    }
  }

  return 0;
}

static s21_big_decimal s21_to_big(s21_decimal value) {
  s21_big_decimal result;

  s21_big_zero(&result);

  result.bits[0] = (uint32_t)value.bits[0];
  result.bits[1] = (uint32_t)value.bits[1];
  result.bits[2] = (uint32_t)value.bits[2];

  return result;
}

static int s21_big_add_digit(s21_big_decimal *value, int digit) {
  if (value == NULL || digit < 0 || digit > 9) {
    return 1;
  }

  s21_big_decimal addend;
  s21_big_zero(&addend);

  addend.bits[0] = (uint32_t)digit;

  return s21_big_add(*value, addend, value);
}

static void s21_big_zero(s21_big_decimal *value) {
  if (value == NULL) {
    return;
  }

  for (int i = 0; i < 6; ++i) {
    value->bits[i] = 0;
  }
}

static void s21_from_big(s21_big_decimal value, s21_decimal *result) {
  if (result == NULL) {
    return;
  }

  result->bits[0] = (int)value.bits[0];
  result->bits[1] = (int)value.bits[1];
  result->bits[2] = (int)value.bits[2];
  result->bits[3] = 0;
}

static int s21_big_add(s21_big_decimal value_1, s21_big_decimal value_2,
                       s21_big_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  uint64_t carry = 0;

  for (int i = 0; i < 6; ++i) {
    uint64_t sum =
        (uint64_t)value_1.bits[i] + (uint64_t)value_2.bits[i] + carry;

    result->bits[i] = (uint32_t)sum;
    carry = sum >> 32;
  }

  return carry != 0;
}

static int s21_big_sub(s21_big_decimal value_1, s21_big_decimal value_2,
                       s21_big_decimal *result) {
  if (result == NULL) {
    return 1;
  }

  uint64_t borrow = 0;
  for (int i = 0; i < 6; ++i) {
    uint64_t first = value_1.bits[i];
    uint64_t second = value_2.bits[i];
    uint64_t sub = second + borrow;

    result->bits[i] = (uint32_t)(first - sub);
    borrow = (first < sub) ? 1 : 0;
  }
  return 0;
}

static int s21_align_scales(s21_big_decimal *big_1, s21_big_decimal *big_2,
                            int *scale_1, int *scale_2) {
  while (*scale_1 < *scale_2) {
    s21_big_decimal temp = *big_1;
    if (s21_big_mul10(&temp) || !s21_big_fits_decimal(temp)) {
      break;
    }
    *big_1 = temp;
    (*scale_1)++;
  }
  while (*scale_2 < *scale_1) {
    s21_big_decimal temp = *big_2;
    if (s21_big_mul10(&temp) || !s21_big_fits_decimal(temp)) {
      break;
    }
    *big_2 = temp;
    (*scale_2)++;
  }
  while (*scale_1 > *scale_2) {
    int remainder = s21_big_div10(big_1);
    if (remainder > 5 || (remainder == 5 && (big_1->bits[0] % 2 == 1))) {
      s21_big_increment(big_1);
    }
    (*scale_1)--;
  }
  while (*scale_2 > *scale_1) {
    int remainder = s21_big_div10(big_2);
    if (remainder > 5 || (remainder == 5 && (big_2->bits[0] % 2 == 1))) {
      s21_big_increment(big_2);
    }
    (*scale_2)--;
  }
  return 0;
}

static int s21_big_mul10(s21_big_decimal *value) {
  if (value == NULL) {
    return 1;
  }

  uint64_t carry = 0;

  for (int i = 0; i < 6; ++i) {
    uint64_t current = (uint64_t)value->bits[i] * 10u + carry;

    value->bits[i] = (uint32_t)current;
    carry = current >> 32;
  }

  return carry != 0;
}

static int s21_reduce_result(s21_big_decimal *value, int *scale) {
  int remainder = 0;
  int sticky = 0;

  while (!s21_big_fits_decimal(*value)) {
    if (*scale == 0) {
      return 1;
    }

    int current_remainder = s21_big_div10(value);

    if (remainder != 0) {
      sticky = 1;
    }

    remainder = current_remainder;
    --(*scale);
  }

  if (remainder == 0 && !sticky) {
    return 0;
  }

  if (s21_bankers_round(value, remainder, sticky)) {
    if (!s21_big_fits_decimal(*value)) {
      if (*scale == 0) {
        return 1;
      }

      int next_remainder = s21_big_div10(value);

      --(*scale);

      if (next_remainder != 0) {
        return s21_reduce_result(value, scale);
      }
    }
  }

  return 0;
}

static int s21_bankers_round(s21_big_decimal *value, int remainder,
                             int sticky) {
  int round_up = 0;

  if (remainder > 5) {
    round_up = 1;
  } else if (remainder == 5) {
    if (sticky) {
      round_up = 1;
    } else if (value->bits[0] & 1u) {
      round_up = 1;
    }
  }

  if (round_up) {
    return s21_big_increment(value);
  }

  return 0;
}

static int s21_big_increment(s21_big_decimal *value) {
  if (value == NULL) {
    return 1;
  }

  for (int i = 0; i < 6; ++i) {
    ++value->bits[i];

    if (value->bits[i] != 0) {
      return 0;
    }
  }

  return 1;
}

static void s21_trim_trailing_zeros(s21_big_decimal *value, int *scale) {
  if (value == NULL || scale == NULL) {
    return;
  }

  while (*scale > 0) {
    s21_big_decimal temp = *value;

    int remainder = s21_big_div10(&temp);

    if (remainder != 0) {
      break;
    }

    *value = temp;
    --(*scale);
  }
}

static int s21_big_div10(s21_big_decimal *value) {
  if (value == NULL) {
    return 0;
  }

  uint64_t remainder = 0;

  for (int i = 5; i >= 0; --i) {
    uint64_t current = (remainder << 32) | value->bits[i];

    value->bits[i] = (uint32_t)(current / 10u);
    remainder = current % 10u;
  }

  return (int)remainder;
}

static int s21_big_get_bit(s21_big_decimal value, int index) {
  if (index < 0 || index >= 192) return 0;
  return (value.bits[index / 32] >> (index % 32)) & 1;
}

static int s21_big_set_bit(s21_big_decimal *value, int index, int bit) {
  if (value == NULL || index < 0 || index >= 192) {
    return 1;
  }
  uint32_t num = value->bits[index / 32];
  uint32_t mask = 1u << (index % 32);
  if (bit) {
    num |= mask;
  } else {
    num &= ~mask;
  }
  value->bits[index / 32] = num;
  return 0;
}

static void s21_big_shift_left(s21_big_decimal *value) {
  if (value == NULL) {
    return;
  }

  uint32_t carry = 0;

  for (int i = 0; i < 6; ++i) {
    uint32_t new_carry = value->bits[i] >> 31;

    value->bits[i] = (value->bits[i] << 1) | carry;

    carry = new_carry;
  }
}

static int s21_big_fits_decimal(s21_big_decimal value) {
  return value.bits[3] == 0 && value.bits[4] == 0 && value.bits[5] == 0;
}