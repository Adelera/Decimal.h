#include "s21_decimal.h"

static int compare_decimals(s21_decimal value_1, s21_decimal value_2);
static void compare_abs_mantissa(unsigned int mantissa_val1[6],
                                 unsigned int mantissa_val2[6], int *result);
static void to_the_general_scale(unsigned int mantissa_val1[6],
                                 unsigned int mantissa_val2[6],
                                 unsigned int scale_val1,
                                 unsigned int scale_val2);

int s21_is_less(s21_decimal value_1, s21_decimal value_2) {
  return compare_decimals(value_1, value_2) == -1;
}

int s21_is_less_or_equal(s21_decimal value_1, s21_decimal value_2) {
  int result = compare_decimals(value_1, value_2);
  return (result == -1 || result == 0);
}

int s21_is_greater(s21_decimal value_1, s21_decimal value_2) {
  return compare_decimals(value_1, value_2) == 1;
}

int s21_is_greater_or_equal(s21_decimal value_1, s21_decimal value_2) {
  int result = compare_decimals(value_1, value_2);
  return (result == 1 || result == 0);
}

int s21_is_equal(s21_decimal value_1, s21_decimal value_2) {
  return compare_decimals(value_1, value_2) == 0;
}

int s21_is_not_equal(s21_decimal value_1, s21_decimal value_2) {
  return compare_decimals(value_1, value_2) != 0;
}

static int compare_decimals(s21_decimal value_1, s21_decimal value_2) {
  int result = 0;
  // 0 - числа равны
  // 1 - value_1 > value_2
  // -1 - value_1 < value_2

  // локальная копия мантиссы + 12 байт запас на случай переполнения при
  // нормализации
  unsigned int mantissa_val1[6] = {0};
  unsigned int mantissa_val2[6] = {0};
  mantissa_val1[0] = value_1.bits[0];
  mantissa_val1[1] = value_1.bits[1];
  mantissa_val1[2] = value_1.bits[2];
  mantissa_val2[0] = value_2.bits[0];
  mantissa_val2[1] = value_2.bits[1];
  mantissa_val2[2] = value_2.bits[2];

  // вычленяем знак и коэф. масштабирования
  unsigned int sign_val1 = ((unsigned int)value_1.bits[3] >> 31) & 1u;
  unsigned int sign_val2 = ((unsigned int)value_2.bits[3] >> 31) & 1u;
  unsigned int scale_val1 = ((unsigned int)value_1.bits[3] >> 16) & 0xFFu;
  unsigned int scale_val2 = ((unsigned int)value_2.bits[3] >> 16) & 0xFFu;

  if ((mantissa_val1[2] == 0 && mantissa_val1[1] == 0 &&
       mantissa_val1[0] == 0) &&
      (mantissa_val2[2] == 0 && mantissa_val2[1] == 0 &&
       mantissa_val2[0] == 0)) {
    result = 0;
  } else if (sign_val1 == sign_val2) {
    if (scale_val1 == scale_val2) {
      compare_abs_mantissa(mantissa_val1, mantissa_val2, &result);
    } else {
      to_the_general_scale(mantissa_val1, mantissa_val2, scale_val1,
                           scale_val2);
      compare_abs_mantissa(mantissa_val1, mantissa_val2, &result);
    }
  } else {
    result = sign_val1 == 0 ? 1 : -1;
  }

  if (sign_val1 == 1 && sign_val2 == 1) {
    result = -result;
  }
  return result;
}

static void compare_abs_mantissa(unsigned int mantissa_val1[6],
                                 unsigned int mantissa_val2[6], int *result) {
  *result = 0;
  int is_difference = 0;  // есть ли отличия

  for (int i = 5; i >= 0 && !is_difference; i--) {
    if (mantissa_val1[i] != mantissa_val2[i]) {
      if (mantissa_val1[i] > mantissa_val2[i]) {
        *result = 1;
      } else if (mantissa_val1[i] < mantissa_val2[i]) {
        *result = -1;
      }

      is_difference = 1;
    }
  }
}

static void to_the_general_scale(unsigned int mantissa_val1[6],
                                 unsigned int mantissa_val2[6],
                                 unsigned int scale_val1,
                                 unsigned int scale_val2) {
  unsigned long long temp = 0;
  unsigned long long carry = 0;

  if (scale_val1 > scale_val2) {
    for (unsigned int i = scale_val1; i > scale_val2; i--) {
      carry = 0;

      for (int j = 0; j < 6; j++) {  // по всем элементам массива
        temp = (unsigned long long)mantissa_val2[j] * 10ULL + carry;
        mantissa_val2[j] = (unsigned int)temp;
        carry = (temp >> 32);
      }
    }
  } else if (scale_val2 > scale_val1) {
    for (unsigned int i = scale_val2; i > scale_val1; i--) {
      carry = 0;

      for (int j = 0; j < 6; j++) {
        temp = (unsigned long long)mantissa_val1[j] * 10ULL + carry;
        mantissa_val1[j] = (unsigned int)temp;
        carry = (temp >> 32);
      }
    }
  }
}