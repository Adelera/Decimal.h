#include <check.h>
#include <stdlib.h>

#include "s21_decimal.h"

s21_decimal create_decimal(unsigned int b0, unsigned int b1, unsigned int b2,
                           unsigned int sign, unsigned int scale) {
  s21_decimal d;
  d.bits[0] = b0;
  d.bits[1] = b1;
  d.bits[2] = b2;
  // Собираем bits[3] из знака и скейла
  d.bits[3] = (sign << 31) | (scale << 16);
  return d;
}

// ТЕСТЫ ДЛЯ S21_SUB

START_TEST(test_sub_simple) {
  s21_decimal a = create_decimal(500, 0, 0, 0, 0);
  s21_decimal b = create_decimal(200, 0, 0, 0, 0);
  s21_decimal result;
  s21_sub(a, b, &result);
  ck_assert_int_eq(result.bits[0], 300);
}

START_TEST(test_sub_to_negative) {
  s21_decimal a = create_decimal(100, 0, 0, 0, 0);
  s21_decimal b = create_decimal(300, 0, 0, 0, 0);
  s21_decimal result;
  s21_sub(a, b, &result);
  ck_assert_int_eq(result.bits[0], 200);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_sub_negative_minus_positive) {
  s21_decimal a = create_decimal(100, 0, 0, 1, 0);  // -100
  s21_decimal b = create_decimal(50, 0, 0, 0, 0);   // 50
  s21_decimal result;
  s21_sub(a, b, &result);
  ck_assert_int_eq(result.bits[0], 150);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_sub_negative_minus_negative) {
  s21_decimal a = create_decimal(100, 0, 0, 1, 0);  // -100
  s21_decimal b = create_decimal(50, 0, 0, 1, 0);   // -50
  s21_decimal result;
  s21_sub(a, b, &result);
  ck_assert_int_eq(result.bits[0], 50);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_sub_equal) {
  s21_decimal a = create_decimal(100, 0, 0, 0, 0);
  s21_decimal b = create_decimal(100, 0, 0, 0, 0);
  s21_decimal result;
  s21_sub(a, b, &result);
  ck_assert_int_eq(result.bits[0], 0);
}

START_TEST(test_sub_null) {
  s21_decimal a = create_decimal(1, 0, 0, 0, 0);
  ck_assert_int_eq(s21_sub(a, a, NULL), 1);
}

// ВОТ ЭТОТ ТЕСТ ПАДАЕТ!!!! ПРОВЕРИТЬ И НА ДРУГИХ АРИФ ОПЕРАЦИЯХ ПОМИМО
START_TEST(test_sub_bankers_rounding_max) {
  s21_decimal value_1 =
      create_decimal(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  s21_decimal value_2 = create_decimal(6, 0, 0, 0, 1);
  s21_decimal result;

  s21_decimal expected =
      create_decimal(0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0);

  int status = s21_sub(value_1, value_2, &result);

  ck_assert_int_eq(status, 0);  // Ошибки переполнения быть не должно, так как
                                // число влезло после округления
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
  ck_assert_int_eq(result.bits[1], expected.bits[1]);
  ck_assert_int_eq(result.bits[2], expected.bits[2]);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
}
END_TEST

// ТЕСТЫ ДЛЯ S21_ADD

START_TEST(test_add_simple) {
  s21_decimal a = create_decimal(123, 0, 0, 0, 0);
  s21_decimal b = create_decimal(456, 0, 0, 0, 0);
  s21_decimal result;
  int status = s21_add(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 579);
  ck_assert_int_eq(result.bits[3], 0);
}

START_TEST(test_add_negative) {
  s21_decimal a = create_decimal(100, 0, 0, 0, 0);
  s21_decimal b = create_decimal(50, 0, 0, 1, 0);  // -50
  s21_decimal result;
  s21_add(a, b, &result);
  ck_assert_int_eq(result.bits[0], 50);
  ck_assert_int_eq(result.bits[3] >> 31, 0);
}

START_TEST(test_add_both_negative) {
  s21_decimal a = create_decimal(100, 0, 0, 1, 0);  // -100
  s21_decimal b = create_decimal(50, 0, 0, 1, 0);   // -50
  s21_decimal result;
  s21_add(a, b, &result);
  ck_assert_int_eq(result.bits[0], 150);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_add_diff_scale) {
  s21_decimal a = create_decimal(1230, 0, 0, 0, 1);  // 123.0
  s21_decimal b = create_decimal(45, 0, 0, 0, 0);    // 45
  s21_decimal result;
  s21_add(a, b, &result);
  ck_assert_int_eq(result.bits[0], 1680);
  ck_assert_int_eq((result.bits[3] >> 16) & 0xFF, 1);
}

START_TEST(test_add_equal_diff_sign) {
  s21_decimal a = create_decimal(100, 0, 0, 0, 0);
  s21_decimal b = create_decimal(100, 0, 0, 1, 0);
  s21_decimal result;
  s21_add(a, b, &result);
  ck_assert_int_eq(result.bits[0], 0);
  ck_assert_int_eq(result.bits[3], 0);
}

START_TEST(test_add_zero) {
  s21_decimal a = create_decimal(0, 0, 0, 0, 0);
  s21_decimal b = create_decimal(100, 0, 0, 0, 0);
  s21_decimal result;
  s21_add(a, b, &result);
  ck_assert_int_eq(result.bits[0], 100);
}

START_TEST(test_add_null) {
  s21_decimal a = create_decimal(1, 0, 0, 0, 0);
  ck_assert_int_eq(s21_add(a, a, NULL), 1);
}

// ТЕСТЫ ДЛЯ S21_DIV

START_TEST(test_div_simple) {
  s21_decimal a = create_decimal(10, 0, 0, 0, 0);
  s21_decimal b = create_decimal(2, 0, 0, 0, 0);
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 5);
}

START_TEST(test_div_negative) {
  s21_decimal a = create_decimal(10, 0, 0, 1, 0);  // -10
  s21_decimal b = create_decimal(2, 0, 0, 0, 0);   // 2
  s21_decimal result;
  s21_div(a, b, &result);
  ck_assert_int_eq(result.bits[0], 5);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_div_by_zero) {
  s21_decimal a = create_decimal(10, 0, 0, 0, 0);
  s21_decimal b = create_decimal(0, 0, 0, 0, 0);
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 3);
}

START_TEST(test_div_zero_by_something) {
  s21_decimal a = create_decimal(0, 0, 0, 0, 0);
  s21_decimal b = create_decimal(5, 0, 0, 0, 0);
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 0);
}

START_TEST(test_div_with_fraction) {
  s21_decimal a = create_decimal(1, 0, 0, 0, 0);
  s21_decimal b = create_decimal(4, 0, 0, 0, 0);
  s21_decimal result;
  s21_div(a, b, &result);
  // 1/4 = 0.25
  ck_assert_int_eq(result.bits[0], 25);
  ck_assert_int_eq((result.bits[3] >> 16) & 0xFF, 2);
}

START_TEST(test_div_both_negative) {
  s21_decimal a = create_decimal(10, 0, 0, 1, 0);  // -10
  s21_decimal b = create_decimal(2, 0, 0, 1, 0);   // -2
  s21_decimal result;
  s21_div(a, b, &result);
  ck_assert_int_eq(result.bits[0], 5);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 0);
}

START_TEST(test_div_null) {
  s21_decimal a = create_decimal(1, 0, 0, 0, 0);
  ck_assert_int_eq(s21_div(a, a, NULL), 1);
}

// ТЕСТЫ ДЛЯ S21_MUL

START_TEST(test_mul_simple) {
  s21_decimal a = create_decimal(5, 0, 0, 0, 0);
  s21_decimal b = create_decimal(3, 0, 0, 0, 0);
  s21_decimal result;
  int status = s21_mul(a, b, &result);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(result.bits[0], 15);
}

START_TEST(test_mul_negative) {
  s21_decimal a = create_decimal(5, 0, 0, 1, 0);  // -5
  s21_decimal b = create_decimal(3, 0, 0, 0, 0);  // 3
  s21_decimal result;
  s21_mul(a, b, &result);
  ck_assert_int_eq(result.bits[0], 15);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_mul_both_negative) {
  s21_decimal a = create_decimal(5, 0, 0, 1, 0);  // -5
  s21_decimal b = create_decimal(3, 0, 0, 1, 0);  // -3
  s21_decimal result;
  s21_mul(a, b, &result);
  ck_assert_int_eq(result.bits[0], 15);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 0);
}

START_TEST(test_mul_zero) {
  s21_decimal a = create_decimal(0, 0, 0, 0, 0);
  s21_decimal b = create_decimal(100, 0, 0, 0, 0);
  s21_decimal result;
  s21_mul(a, b, &result);
  ck_assert_int_eq(result.bits[0], 0);
}

START_TEST(test_mul_with_scale) {
  s21_decimal a = create_decimal(25, 0, 0, 0, 1);  // 2.5
  s21_decimal b = create_decimal(4, 0, 0, 0, 0);   // 4
  s21_decimal result;
  s21_mul(a, b, &result);
  ck_assert_int_eq(result.bits[0], 100);
  ck_assert_int_eq((result.bits[3] >> 16) & 0xFF, 1);
}

START_TEST(test_mul_overflow_scale) {
  s21_decimal a = create_decimal(1, 0, 0, 0, 20);
  s21_decimal b = create_decimal(1, 0, 0, 0, 20);
  s21_decimal result;
  int status = s21_mul(a, b, &result);
  // scale 40 → обрезается до 28
  ck_assert_int_eq(status, 0);
}

START_TEST(test_mul_null) {
  s21_decimal a = create_decimal(1, 0, 0, 0, 0);
  ck_assert_int_eq(s21_mul(a, a, NULL), 1);
}

// ТЕСТЫ ДЛЯ ФУНКЦИЙ СРАВНЕНИЯ
START_TEST(test_equal_diff_scale) {
  s21_decimal val1 = create_decimal(0, 0, 4300, 0, 2);
  s21_decimal val2 = create_decimal(0, 0, 43, 0, 0);
  ck_assert_int_eq(s21_is_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_greater(val1, val2), 0);
  ck_assert_int_eq(s21_is_less(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater_or_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_less_or_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_not_equal(val1, val2), 0);
}

START_TEST(test_scale2_greater_scale1) {
  s21_decimal val1 = create_decimal(0, 0, 15036, 0, 2);
  s21_decimal val2 = create_decimal(0, 0, 3694056, 0, 3);
  ck_assert_int_eq(s21_is_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater(val1, val2), 0);
  ck_assert_int_eq(s21_is_less(val1, val2), 1);
  ck_assert_int_eq(s21_is_greater_or_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_less_or_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_not_equal(val1, val2), 1);
}

START_TEST(test_val1_greater_val2) {
  s21_decimal val1 = create_decimal(0, 0, 3694056, 0, 2);
  s21_decimal val2 = create_decimal(0, 0, 36940, 0, 3);
  ck_assert_int_eq(s21_is_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater(val1, val2), 1);
  ck_assert_int_eq(s21_is_less(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater_or_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_less_or_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_not_equal(val1, val2), 1);
}

START_TEST(test_two_zeros) {
  s21_decimal val1 = create_decimal(0, 0, 0, 0, 0);
  s21_decimal val2 = create_decimal(0, 0, 0, 1, 0);
  ck_assert_int_eq(s21_is_equal(val1, val2), 1);
}

START_TEST(test_two_zeros_with_scale) {
  s21_decimal val1 = create_decimal(0, 0, 0, 0, 3);
  s21_decimal val2 = create_decimal(0, 0, 0, 0, 0);
  ck_assert_int_eq(s21_is_equal(val1, val2), 1);
}

START_TEST(test_two_zeros_with_scale_diff_sign) {
  s21_decimal val1 = create_decimal(0, 0, 0, 0, 3);
  s21_decimal val2 = create_decimal(0, 0, 0, 1, 0);
  ck_assert_int_eq(s21_is_equal(val1, val2), 1);
}

START_TEST(test_neg_and_pos) {
  s21_decimal val1 = create_decimal(0, 0, 100, 0, 0);
  s21_decimal val2 = create_decimal(0, 0, 100, 1, 0);
  ck_assert_int_eq(s21_is_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater(val1, val2), 1);
  ck_assert_int_eq(s21_is_less(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater_or_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_less_or_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_not_equal(val1, val2), 1);
}

START_TEST(test_two_neg) {
  s21_decimal val1 = create_decimal(0, 0, 10, 1, 0);
  s21_decimal val2 = create_decimal(0, 0, 20, 1, 0);
  ck_assert_int_eq(s21_is_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater(val1, val2), 1);
  ck_assert_int_eq(s21_is_less(val1, val2), 0);
  ck_assert_int_eq(s21_is_greater_or_equal(val1, val2), 1);
  ck_assert_int_eq(s21_is_less_or_equal(val1, val2), 0);
  ck_assert_int_eq(s21_is_not_equal(val1, val2), 1);
}

// ТЕСТЫ ДЛЯ S21_ROUND

START_TEST(test_round_positive_down) {
  s21_decimal src = create_decimal(1234, 0, 0, 0, 1);  // 123.4
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 0);
}

START_TEST(test_round_positive_up) {
  s21_decimal src = create_decimal(1236, 0, 0, 0, 1);  // 123.6
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
}

START_TEST(test_round_positive_half) {
  s21_decimal src = create_decimal(1235, 0, 0, 0, 1);  // 123.5
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);  // 123.5 → 124
}

START_TEST(test_round_negative_down) {
  s21_decimal src = create_decimal(1234, 0, 0, 1, 1);  // -123.4
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_round_negative_up) {
  s21_decimal src = create_decimal(1236, 0, 0, 1, 1);  // -123.6
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_round_negative_half) {
  s21_decimal src = create_decimal(1235, 0, 0, 1, 1);  // -123.5
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 124);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_round_zero_scale) {
  s21_decimal src = create_decimal(123, 0, 0, 0, 0);
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(result.bits[3], 0);
}

START_TEST(test_round_zero) {
  s21_decimal src = create_decimal(0, 0, 0, 0, 0);
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 0);
}

START_TEST(test_round_small_fraction) {
  s21_decimal src = create_decimal(1, 0, 0, 0, 2);  // 0.01
  s21_decimal result;
  s21_round(src, &result);
  ck_assert_int_eq(result.bits[0], 0);
}

START_TEST(test_round_null) {
  s21_decimal src = create_decimal(123, 0, 0, 0, 2);
  ck_assert_int_eq(s21_round(src, NULL), 1);
}

// ТЕСТЫ ДЛЯ S21_NEGATE

START_TEST(test_negate_positive) {
  s21_decimal src = create_decimal(12345, 0, 0, 0, 0);
  s21_decimal result;
  s21_negate(src, &result);
  ck_assert_int_eq(result.bits[0], 12345);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);  // стало отрицательным
}

START_TEST(test_negate_negative) {
  s21_decimal src = create_decimal(12345, 0, 0, 1, 0);
  s21_decimal result;
  s21_negate(src, &result);
  ck_assert_int_eq(result.bits[0], 12345);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 0);  // стало положительным
}

START_TEST(test_negate_zero) {
  s21_decimal src = create_decimal(0, 0, 0, 0, 0);
  s21_decimal result;
  s21_negate(src, &result);
  ck_assert_int_eq(result.bits[0], 0);
  ck_assert_int_eq(result.bits[3], 0);
}

START_TEST(test_negate_negative_zero) {
  s21_decimal src = create_decimal(0, 0, 0, 1, 0);
  s21_decimal result;
  s21_negate(src, &result);
  ck_assert_int_eq(result.bits[0], 0);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 0);  // -0 → 0
}

START_TEST(test_negate_with_scale) {
  s21_decimal src = create_decimal(12345, 0, 0, 0, 2);  // 123.45
  s21_decimal result;
  s21_negate(src, &result);
  ck_assert_int_eq(result.bits[0], 12345);
  ck_assert_int_eq((result.bits[3] >> 16) & 0xFF, 2);  // степень сохранилась
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_negate_null) {
  s21_decimal src = create_decimal(123, 0, 0, 0, 0);
  ck_assert_int_eq(s21_negate(src, NULL), 1);
}

// ТЕСТЫ ДЛЯ S21_FLOOR

START_TEST(test_floor_positive) {
  s21_decimal src = create_decimal(12345, 0, 0, 0, 2);  // 123.45
  s21_decimal result;
  s21_floor(src, &result);
  s21_decimal expected = create_decimal(123, 0, 0, 0, 0);
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
}

START_TEST(test_floor_negative) {
  s21_decimal src = create_decimal(12345, 0, 0, 1, 2);  // -123.45
  s21_decimal result;
  s21_floor(src, &result);
  s21_decimal expected = create_decimal(124, 0, 0, 1, 0);  // -124
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_floor_zero_scale) {
  s21_decimal src = create_decimal(123, 0, 0, 0, 0);
  s21_decimal result;
  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq(result.bits[3], 0);
}

START_TEST(test_floor_zero) {
  s21_decimal src = create_decimal(0, 0, 0, 0, 0);
  s21_decimal result;
  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 0);
  ck_assert_int_eq(result.bits[3], 0);
}

START_TEST(test_floor_negative_zero) {
  s21_decimal src = create_decimal(0, 0, 0, 1, 0);
  s21_decimal result;
  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 0);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 0);  // -0 → 0
}

START_TEST(test_floor_negative_integer) {
  s21_decimal src = create_decimal(123, 0, 0, 1, 0);  // -123
  s21_decimal result;
  s21_floor(src, &result);
  ck_assert_int_eq(result.bits[0], 123);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_floor_negative_fraction) {
  s21_decimal src = create_decimal(5, 0, 0, 1, 1);  // -0.5
  s21_decimal result;
  s21_floor(src, &result);
  s21_decimal expected = create_decimal(1, 0, 0, 1, 0);  // -1
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
  ck_assert_int_eq((result.bits[3] >> 31) & 1, 1);
}

START_TEST(test_floor_null) {
  s21_decimal src = create_decimal(123, 0, 0, 0, 2);
  ck_assert_int_eq(s21_floor(src, NULL), 1);
}

// ТЕСТЫ ДЛЯ S21_FROM_FLOAT_TO_DECIMAL
START_TEST(test_from_float_zero) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(0.0f, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 0);
  ck_assert_int_eq(dst.bits[1], 0);
  ck_assert_int_eq(dst.bits[2], 0);
}

START_TEST(test_from_float_positive) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(123.45f, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 12345);
  ck_assert_int_eq((dst.bits[3] >> 16) & 0xFF, 2);
}

START_TEST(test_from_float_negative) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(-123.45f, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 12345);
  ck_assert_int_eq((dst.bits[3] >> 31) & 1, 1);
}

START_TEST(test_from_float_integer) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(42.0f, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 42);
  ck_assert_int_eq((dst.bits[3] >> 16) & 0xFF, 0);
}

START_TEST(test_from_float_small) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(0.001f, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert(dst.bits[0] >= 10000000 && dst.bits[0] <= 10000010);
  int scale = (dst.bits[3] >> 16) & 0xFF;
  ck_assert(scale >= 8 && scale <= 12);
}

START_TEST(test_from_float_too_small) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(1e-29f, &dst);
  ck_assert_int_eq(status, 1);
}

START_TEST(test_from_float_too_big) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(INFINITY, &dst);
  ck_assert_int_eq(status, 1);
}

START_TEST(test_from_float_nan) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(NAN, &dst);
  ck_assert_int_eq(status, 1);
}

START_TEST(test_from_float_inf) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(INFINITY, &dst);
  ck_assert_int_eq(status, 1);
}

START_TEST(test_from_float_neg_inf) {
  s21_decimal dst;
  int status = s21_from_float_to_decimal(-INFINITY, &dst);
  ck_assert_int_eq(status, 1);
}

START_TEST(test_from_float_null) {
  int status = s21_from_float_to_decimal(5.0f, NULL);
  ck_assert_int_eq(status, 1);
}

// ТЕСТЫ ДЛЯ S21_FROM_INT_TO_DECIMAL
START_TEST(test_from_int_zero) {
  s21_decimal dst;
  int status = s21_from_int_to_decimal(0, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 0);
  ck_assert_int_eq(dst.bits[1], 0);
  ck_assert_int_eq(dst.bits[2], 0);
  ck_assert_int_eq(dst.bits[3], 0);
}

START_TEST(test_from_int_positive) {
  s21_decimal dst;
  int status = s21_from_int_to_decimal(12345, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 12345);
  ck_assert_int_eq((dst.bits[3] >> 31) & 1, 0);
}

START_TEST(test_from_int_negative) {
  s21_decimal dst;
  int status = s21_from_int_to_decimal(-12345, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 12345);
  ck_assert_int_eq((dst.bits[3] >> 31) & 1, 1);
}

START_TEST(test_from_int_max) {
  s21_decimal dst;
  int status = s21_from_int_to_decimal(2147483647, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 2147483647);
  ck_assert_int_eq((dst.bits[3] >> 31) & 1, 0);
}

START_TEST(test_from_int_min) {
  s21_decimal dst;
  int status = s21_from_int_to_decimal(-2147483648, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq((int)dst.bits[0], -2147483648);
  ck_assert_int_eq((dst.bits[3] >> 31) & 1, 1);
}

START_TEST(test_from_int_one) {
  s21_decimal dst;
  int status = s21_from_int_to_decimal(1, &dst);
  ck_assert_int_eq(status, 0);
  ck_assert_int_eq(dst.bits[0], 1);
}

START_TEST(test_from_int_null) {
  int status = s21_from_int_to_decimal(5, NULL);
  ck_assert_int_eq(status, 1);
}

// ТЕСТЫ ДЛЯ S21_FROM_DECIMAL_TO_INT
START_TEST(test_simple) {
  s21_decimal src = create_decimal(153, 0, 0, 0, 1);
  int dst;
  s21_from_decimal_to_int(src, &dst);
  ck_assert_int_eq(dst, 15);
}
END_TEST

START_TEST(test_neg_and_float) {
  s21_decimal src = create_decimal(5, 0, 0, 1, 2);
  int dst;
  s21_from_decimal_to_int(src, &dst);
  ck_assert_int_eq(dst, 0);
}
END_TEST

START_TEST(test_zero_mantissa) {
  s21_decimal src = create_decimal(0, 0, 0, 1, 0);
  int dst;
  s21_from_decimal_to_int(src, &dst);
  ck_assert_int_eq(dst, 0);
}

START_TEST(test_too_big) {
  s21_decimal src = create_decimal(2147483648, 0, 0, 0, 0);
  int dst;
  ck_assert_int_eq(s21_from_decimal_to_int(src, &dst), 1);
}

START_TEST(test_too_big2) {
  s21_decimal src = create_decimal(2147483647, 2147483647, 0, 0, 0);
  int dst;
  ck_assert_int_eq(s21_from_decimal_to_int(src, &dst), 1);
}

START_TEST(test_int_min) {
  s21_decimal src = create_decimal(2147483648, 0, 0, 1, 0);
  int dst;
  s21_from_decimal_to_int(src, &dst);
  ck_assert_int_eq(dst, -2147483648);
}

START_TEST(test_bigger_int_min) {
  s21_decimal src = create_decimal(2147483649, 0, 0, 0, 0);
  int dst;
  ck_assert_int_eq(s21_from_decimal_to_int(src, &dst), 1);
}

START_TEST(test_big_mantissa) {
  s21_decimal src = create_decimal(0, 1, 0, 0, 1);
  int dst;
  s21_from_decimal_to_int(src, &dst);
  ck_assert_int_eq(dst, 429496729);
}

START_TEST(test_null_pointer) {
  s21_decimal src = create_decimal(10, 0, 0, 0, 0);
  ck_assert_int_eq(s21_from_decimal_to_int(src, NULL), 1);
}

START_TEST(test_invalid_scale) {
  s21_decimal src = create_decimal(105, 0, 0, 0, 30);
  int dst;
  ck_assert_int_eq(s21_from_decimal_to_int(src, &dst), 1);
}

// ТЕСТЫ ДЛЯ S21_FROM_DECIMAL_TO_FLOAT
START_TEST(test_float) {
  s21_decimal src = create_decimal(12345, 0, 0, 0, 2);
  float dst;
  s21_from_decimal_to_float(src, &dst);
  ck_assert_float_eq(dst, 123.45);
}

START_TEST(test_neg_float) {
  s21_decimal src = create_decimal(987654, 0, 0, 1, 0);
  float dst;
  s21_from_decimal_to_float(src, &dst);
  ck_assert_float_eq(dst, -987654.0);
}

START_TEST(test_zero_float) {
  s21_decimal src = create_decimal(0, 0, 0, 0, 0);
  float dst;
  s21_from_decimal_to_float(src, &dst);
  ck_assert_float_eq(dst, 0.0);
}

START_TEST(test_neg_zero_float) {
  s21_decimal src = create_decimal(0, 0, 0, 1, 0);
  float dst;
  s21_from_decimal_to_float(src, &dst);
  ck_assert_float_eq_tol(dst, 0.0f, 1e-7);
}

START_TEST(test_neg_float_scale) {
  s21_decimal src = create_decimal(5, 0, 0, 1, 1);
  float dst;
  s21_from_decimal_to_float(src, &dst);
  ck_assert_float_eq(dst, -0.5);
}

START_TEST(test_float_max_scale) {
  s21_decimal src = create_decimal(1, 0, 0, 0, 28);
  float dst;
  s21_from_decimal_to_float(src, &dst);
  ck_assert_float_eq_tol(dst, 1e-28f, 1e-35);
}

// ТЕСТЫ ДЛЯ S21_TRUNCATE

START_TEST(test_truncate) {
  s21_decimal value = create_decimal(123456789, 0, 0, 1, 3);
  s21_decimal result;
  s21_decimal expected = create_decimal(123456, 0, 0, 1, 0);
  s21_truncate(value, &result);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
  ck_assert_int_eq(result.bits[2], expected.bits[2]);
  ck_assert_int_eq(result.bits[1], expected.bits[1]);
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
}

START_TEST(test_truncate_zero) {
  s21_decimal value = create_decimal(0, 0, 0, 1, 0);
  s21_decimal result;
  s21_decimal expected = create_decimal(0, 0, 0, 0, 0);
  s21_truncate(value, &result);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
  ck_assert_int_eq(result.bits[2], expected.bits[2]);
  ck_assert_int_eq(result.bits[1], expected.bits[1]);
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
}

START_TEST(test_truncate_integer) {
  s21_decimal value = create_decimal(123, 0, 0, 0, 0);
  s21_decimal result;
  s21_decimal expected = create_decimal(123, 0, 0, 0, 0);
  s21_truncate(value, &result);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
  ck_assert_int_eq(result.bits[2], expected.bits[2]);
  ck_assert_int_eq(result.bits[1], expected.bits[1]);
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
}

START_TEST(test_truncate_integer_neg) {
  s21_decimal value = create_decimal(123, 0, 0, 1, 0);
  s21_decimal result;
  s21_decimal expected = create_decimal(123, 0, 0, 1, 0);
  s21_truncate(value, &result);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
  ck_assert_int_eq(result.bits[2], expected.bits[2]);
  ck_assert_int_eq(result.bits[1], expected.bits[1]);
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
}

START_TEST(test_truncate_small) {
  s21_decimal value = create_decimal(99999, 0, 0, 0, 6);
  s21_decimal result;
  s21_decimal expected = create_decimal(0, 0, 0, 0, 0);
  s21_truncate(value, &result);
  ck_assert_int_eq(result.bits[3], expected.bits[3]);
  ck_assert_int_eq(result.bits[2], expected.bits[2]);
  ck_assert_int_eq(result.bits[1], expected.bits[1]);
  ck_assert_int_eq(result.bits[0], expected.bits[0]);
}

START_TEST(test_div_with_remainder) {
  s21_decimal a = {{1, 0, 0, 0}};
  s21_decimal b = {{3, 0, 0, 0}};
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
  // Проверить результат
}
END_TEST

START_TEST(test_div_rounding_branch) {
  s21_decimal a = {{2, 0, 0, 0}};
  s21_decimal b = {{3, 0, 0, 0}};
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_div_rounding_down) {
  s21_decimal a = {{1, 0, 0, 0}};
  s21_decimal b = {{3, 0, 0, 0}};
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_div_bankers_round) {
  s21_decimal a = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0}};
  s21_decimal b = {{3, 0, 0, 0}};
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_div_max_by_7) {
  s21_decimal a = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0}};
  s21_decimal b = {{7, 0, 0, 0}};
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_div_max_scaled_by_3) {
  s21_decimal a = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, (10 << 16)}};
  s21_decimal b = {{3, 0, 0, 0}};
  s21_decimal result;
  int status = s21_div(a, b, &result);
  ck_assert_int_eq(status, 0);
}
END_TEST

int main(void) {
  Suite *suite = suite_create("s21_decimal");
  TCase *tcase_core = tcase_create("core");

  // floor
  tcase_add_test(tcase_core, test_floor_positive);
  tcase_add_test(tcase_core, test_floor_negative);
  tcase_add_test(tcase_core, test_floor_zero_scale);
  tcase_add_test(tcase_core, test_floor_zero);
  tcase_add_test(tcase_core, test_floor_negative_zero);
  tcase_add_test(tcase_core, test_floor_negative_integer);
  tcase_add_test(tcase_core, test_floor_negative_fraction);
  tcase_add_test(tcase_core, test_floor_null);

  // negate
  tcase_add_test(tcase_core, test_negate_positive);
  tcase_add_test(tcase_core, test_negate_negative);
  tcase_add_test(tcase_core, test_negate_zero);
  tcase_add_test(tcase_core, test_negate_negative_zero);
  tcase_add_test(tcase_core, test_negate_with_scale);
  tcase_add_test(tcase_core, test_negate_null);

  // round
  tcase_add_test(tcase_core, test_round_positive_down);
  tcase_add_test(tcase_core, test_round_positive_up);
  tcase_add_test(tcase_core, test_round_positive_half);
  tcase_add_test(tcase_core, test_round_negative_down);
  tcase_add_test(tcase_core, test_round_negative_up);
  tcase_add_test(tcase_core, test_round_negative_half);
  tcase_add_test(tcase_core, test_round_zero_scale);
  tcase_add_test(tcase_core, test_round_zero);
  tcase_add_test(tcase_core, test_round_small_fraction);
  tcase_add_test(tcase_core, test_round_null);

  tcase_add_test(tcase_core, test_equal_diff_scale);
  tcase_add_test(tcase_core, test_two_zeros);
  tcase_add_test(tcase_core, test_two_zeros_with_scale);
  tcase_add_test(tcase_core, test_two_zeros_with_scale_diff_sign);
  tcase_add_test(tcase_core, test_neg_and_pos);
  tcase_add_test(tcase_core, test_two_neg);
  tcase_add_test(tcase_core, test_scale2_greater_scale1);
  tcase_add_test(tcase_core, test_val1_greater_val2);

  tcase_add_test(tcase_core, test_simple);
  tcase_add_test(tcase_core, test_neg_and_float);
  tcase_add_test(tcase_core, test_zero_mantissa);
  tcase_add_test(tcase_core, test_too_big);
  tcase_add_test(tcase_core, test_too_big2);
  tcase_add_test(tcase_core, test_int_min);
  tcase_add_test(tcase_core, test_bigger_int_min);
  tcase_add_test(tcase_core, test_big_mantissa);
  tcase_add_test(tcase_core, test_null_pointer);
  tcase_add_test(tcase_core, test_invalid_scale);

  tcase_add_test(tcase_core, test_float);
  tcase_add_test(tcase_core, test_neg_float);
  tcase_add_test(tcase_core, test_zero_float);
  tcase_add_test(tcase_core, test_neg_zero_float);
  tcase_add_test(tcase_core, test_neg_float_scale);
  tcase_add_test(tcase_core, test_float_max_scale);

  // from_float_to_decimal
  tcase_add_test(tcase_core, test_from_float_zero);
  tcase_add_test(tcase_core, test_from_float_positive);
  tcase_add_test(tcase_core, test_from_float_negative);
  tcase_add_test(tcase_core, test_from_float_integer);
  tcase_add_test(tcase_core, test_from_float_small);
  tcase_add_test(tcase_core, test_from_float_too_small);
  tcase_add_test(tcase_core, test_from_float_too_big);
  tcase_add_test(tcase_core, test_from_float_nan);
  tcase_add_test(tcase_core, test_from_float_inf);
  tcase_add_test(tcase_core, test_from_float_neg_inf);
  tcase_add_test(tcase_core, test_from_float_null);

  // from_int_to_decimal
  tcase_add_test(tcase_core, test_from_int_zero);
  tcase_add_test(tcase_core, test_from_int_positive);
  tcase_add_test(tcase_core, test_from_int_negative);
  tcase_add_test(tcase_core, test_from_int_max);
  tcase_add_test(tcase_core, test_from_int_min);
  tcase_add_test(tcase_core, test_from_int_one);
  tcase_add_test(tcase_core, test_from_int_null);

  // add
  tcase_add_test(tcase_core, test_add_simple);
  tcase_add_test(tcase_core, test_add_negative);
  tcase_add_test(tcase_core, test_add_both_negative);
  tcase_add_test(tcase_core, test_add_diff_scale);
  tcase_add_test(tcase_core, test_add_equal_diff_sign);
  tcase_add_test(tcase_core, test_add_zero);
  tcase_add_test(tcase_core, test_add_null);

  // sub
  tcase_add_test(tcase_core, test_sub_simple);
  tcase_add_test(tcase_core, test_sub_to_negative);
  tcase_add_test(tcase_core, test_sub_negative_minus_positive);
  tcase_add_test(tcase_core, test_sub_negative_minus_negative);
  tcase_add_test(tcase_core, test_sub_equal);
  tcase_add_test(tcase_core, test_sub_null);
  tcase_add_test(tcase_core, test_sub_bankers_rounding_max);

  // mul
  tcase_add_test(tcase_core, test_mul_simple);
  tcase_add_test(tcase_core, test_mul_negative);
  tcase_add_test(tcase_core, test_mul_both_negative);
  tcase_add_test(tcase_core, test_mul_zero);
  tcase_add_test(tcase_core, test_mul_with_scale);
  tcase_add_test(tcase_core, test_mul_overflow_scale);
  tcase_add_test(tcase_core, test_mul_null);

  // div
  tcase_add_test(tcase_core, test_div_simple);
  tcase_add_test(tcase_core, test_div_negative);
  tcase_add_test(tcase_core, test_div_by_zero);
  tcase_add_test(tcase_core, test_div_zero_by_something);
  tcase_add_test(tcase_core, test_div_with_fraction);
  tcase_add_test(tcase_core, test_div_both_negative);
  tcase_add_test(tcase_core, test_div_null);

  tcase_add_test(tcase_core, test_truncate);
  tcase_add_test(tcase_core, test_truncate_zero);
  tcase_add_test(tcase_core, test_truncate_integer);
  tcase_add_test(tcase_core, test_truncate_integer_neg);
  tcase_add_test(tcase_core, test_truncate_small);
  tcase_add_test(tcase_core, test_div_with_remainder);
  tcase_add_test(tcase_core, test_div_rounding_branch);
  tcase_add_test(tcase_core, test_div_rounding_down);
  tcase_add_test(tcase_core, test_div_bankers_round);
  tcase_add_test(tcase_core, test_div_max_by_7);
  tcase_add_test(tcase_core, test_div_max_scaled_by_3);

  suite_add_tcase(suite, tcase_core);

  SRunner *runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed ? 1 : 0;
}