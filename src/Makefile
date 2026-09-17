CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11 -g
GCOV_FLAGS = -fprofile-arcs -ftest-coverage
LIBS = -lcheck -lsubunit -lm -lpthread -lrt
TARGET = s21_decimal.a
SRCS = s21_arithmetic.c s21_convert.c s21_rounding.c s21_compare_functions.c s21_common.c
OBJS = $(SRCS:.c=.o)
TEST_SRCS = s21_test.c
TEST_BIN = test_bin

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(TEST_SRCS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $< $(TARGET) $(LIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BIN)
	rm -rf gcov_report
	rm -f *.gcda *.gcno *.gcov coverage.info

rebuild: clean all

gcov_report:
	$(CC) $(CFLAGS) $(GCOV_FLAGS) -o $(TEST_BIN) $(SRCS) $(TEST_SRCS) $(LIBS)
	./$(TEST_BIN)
	lcov --capture --directory . --output-file coverage.info --no-external
	genhtml coverage.info --output-directory gcov_report
	rm -f *.gcda *.gcno *.gcov coverage.info

.PHONY: all clean rebuild test gcov_report