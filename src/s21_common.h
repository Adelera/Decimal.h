#ifndef S21_COMMON_H
#define S21_COMMON_H

#include <stdint.h>

#include "s21_decimal.h"

int s21_get_scale(s21_decimal value);
void s21_zero(s21_decimal *value);
int s21_is_zero(s21_decimal value);
int s21_get_sign(s21_decimal value);
void s21_set_sign(s21_decimal *value, int sign);
void s21_set_scale(s21_decimal *value, int scale);

#endif