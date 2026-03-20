#include "tests.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"

static void format_elem_to_buf(const RingOps* ring, const void* elem, char* buf,
                               size_t bufsz) {
  if (ring == GetIntRing()) {
    snprintf(buf, bufsz, "%d", *(const int*)elem);
    return;
  }
  if (ring == GetComplexRing()) {
    const Complex* z = (const Complex*)elem;
    if (z->im >= 0)
      snprintf(buf, bufsz, "%.6g+%.6gi", z->re, z->im);
    else
      snprintf(buf, bufsz, "%.6g%.6gi", z->re, z->im);
    return;
  }
  snprintf(buf, bufsz, "<elem>");
}

static void print_hline_ascii(const size_t* w, size_t cols) {
  putchar('+');
  for (size_t j = 0; j < cols; j++) {
    for (size_t k = 0; k < w[j] + 2; k++) putchar('-');
    putchar('+');
  }
  putchar('\n');
}

static void print_matrix_boxed_ascii(const char* title, const Matrix* m) {
  if (!m) {
    printf("\n%s: (null)\n", title);
    return;
  }

  printf("\n%s (%zux%zu)\n", title, m->rows, m->cols);

  void* tmp = malloc(m->ring->elem_size);
  if (!tmp) {
    printf("(alloc failed)\n");
    return;
  }

  size_t* w = (size_t*)calloc(m->cols, sizeof(size_t));
  if (!w) {
    free(tmp);
    printf("(alloc failed)\n");
    return;
  }

  char cell[128];

  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++) {
      if (MatrixGet(m, i, j, tmp) != MATRIX_OK) {
        free(tmp);
        free(w);
        return;
      }
      format_elem_to_buf(m->ring, tmp, cell, sizeof(cell));
      size_t len = strlen(cell);
      if (len > w[j]) w[j] = len;
    }
  }

  print_hline_ascii(w, m->cols);
  for (size_t i = 0; i < m->rows; i++) {
    putchar('|');
    for (size_t j = 0; j < m->cols; j++) {
      if (MatrixGet(m, i, j, tmp) != MATRIX_OK) {
        free(tmp);
        free(w);
        return;
      }
      format_elem_to_buf(m->ring, tmp, cell, sizeof(cell));
      printf(" %*s ", (int)w[j], cell);
      putchar('|');
    }
    putchar('\n');
    print_hline_ascii(w, m->cols);
  }

  free(tmp);
  free(w);
}

typedef struct TestCtx {
  int total_checks;
  int total_failed;
  int test_checks;
  int test_failed;
} TestCtx;

static void test_start(TestCtx* ctx, const char* name) {
  ctx->test_checks = 0;
  ctx->test_failed = 0;
  printf("\n=================================================\n");
  printf("[TEST] %s\n", name);
  printf("=================================================\n");
}

static void test_finish(TestCtx* ctx) {
  if (ctx->test_failed == 0) {
    printf("[PASS] checks=%d\n", ctx->test_checks);
  } else {
    printf("[FAIL] failed=%d/%d\n", ctx->test_failed, ctx->test_checks);
  }
}

#define CHECK(ctx, expr)                                       \
  do {                                                         \
    (ctx)->total_checks++;                                     \
    (ctx)->test_checks++;                                      \
    if (!(expr)) {                                             \
      (ctx)->total_failed++;                                   \
      (ctx)->test_failed++;                                    \
      printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    } else {                                                   \
      printf("  ok: %s\n", #expr);                             \
    }                                                          \
  } while (0)

#define CHECK_STATUS(ctx, st, exp)                                        \
  do {                                                                    \
    (ctx)->total_checks++;                                                \
    (ctx)->test_checks++;                                                 \
    if ((st) != (exp)) {                                                  \
      (ctx)->total_failed++;                                              \
      (ctx)->test_failed++;                                               \
      printf("  FAIL %s:%d: status=%s expected=%s\n", __FILE__, __LINE__, \
             MatrixStatusToString((st)), MatrixStatusToString((exp)));    \
    } else {                                                              \
      printf("  ok: status == %s\n", MatrixStatusToString((exp)));        \
    }                                                                     \
  } while (0)

static int complex_eq_eps(const Complex* a, const Complex* b, double eps) {
  return fabs(a->re - b->re) <= eps && fabs(a->im - b->im) <= eps;
}

static void t_create_invalid_sizes(TestCtx* ctx) {
  test_start(ctx, "MatrixCreate invalid sizes (0x0, 0xN, Nx0)");
  MatrixStatus st;

  Matrix* m00 = MatrixCreate(0, 0, GetIntRing(), &st);
  CHECK(ctx, m00 == NULL);
  CHECK(ctx, st != MATRIX_OK);

  Matrix* m05 = MatrixCreate(0, 5, GetIntRing(), &st);
  CHECK(ctx, m05 == NULL);
  CHECK(ctx, st != MATRIX_OK);

  Matrix* m70 = MatrixCreate(7, 0, GetIntRing(), &st);
  CHECK(ctx, m70 == NULL);
  CHECK(ctx, st != MATRIX_OK);

  test_finish(ctx);
}

static void t_null_ring(TestCtx* ctx) {
  test_start(ctx, "MatrixCreate NULL ring");
  MatrixStatus st;

  Matrix* m = MatrixCreate(2, 3, NULL, &st);
  CHECK(ctx, m == NULL);
  CHECK(ctx, st != MATRIX_OK);

  test_finish(ctx);
}

static void t_bounds_and_nulls(TestCtx* ctx) {
  test_start(ctx, "MatrixGet/Set bounds + NULL args");
  MatrixStatus st;
  Matrix* A = MatrixCreate(2, 3, GetIntRing(), &st);
  CHECK(ctx, A != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);

  print_matrix_boxed_ascii("A (initial)", A);

  int v = 123, out = 0;

  CHECK_STATUS(ctx, MatrixSet(A, 2, 0, &v), MATRIX_ERR_BOUNDS);
  CHECK_STATUS(ctx, MatrixSet(A, 0, 3, &v), MATRIX_ERR_BOUNDS);
  CHECK_STATUS(ctx, MatrixGet(A, 2, 0, &out), MATRIX_ERR_BOUNDS);
  CHECK_STATUS(ctx, MatrixGet(A, 0, 3, &out), MATRIX_ERR_BOUNDS);

  CHECK_STATUS(ctx, MatrixSet(NULL, 0, 0, &v), MATRIX_ERR_NULL);
  CHECK_STATUS(ctx, MatrixSet(A, 0, 0, NULL), MATRIX_ERR_NULL);
  CHECK_STATUS(ctx, MatrixGet(NULL, 0, 0, &out), MATRIX_ERR_NULL);
  CHECK_STATUS(ctx, MatrixGet(A, 0, 0, NULL), MATRIX_ERR_NULL);

  MatrixDestroy(A);
  test_finish(ctx);
}

static void t_shape_mismatch(TestCtx* ctx) {
  test_start(ctx, "Ops shape mismatch");
  MatrixStatus st;

  Matrix* A23 = MatrixCreate(2, 3, GetIntRing(), &st);
  Matrix* B34 = MatrixCreate(3, 4, GetIntRing(), &st);
  Matrix* C22 = MatrixCreate(2, 2, GetIntRing(), &st);
  CHECK(ctx, A23 && B34 && C22);

  print_matrix_boxed_ascii("A23", A23);
  print_matrix_boxed_ascii("B34", B34);
  print_matrix_boxed_ascii("C22", C22);

  Matrix* add_bad = MatrixAdd(A23, B34, &st);
  CHECK(ctx, add_bad == NULL);
  CHECK_STATUS(ctx, st, MATRIX_ERR_SHAPE);

  Matrix* sub_bad = MatrixSub(A23, C22, &st);
  CHECK(ctx, sub_bad == NULL);
  CHECK_STATUS(ctx, st, MATRIX_ERR_SHAPE);

  Matrix* mul_bad = MatrixMul(C22, B34, &st);
  CHECK(ctx, mul_bad == NULL);
  CHECK_STATUS(ctx, st, MATRIX_ERR_SHAPE);

  MatrixDestroy(A23);
  MatrixDestroy(B34);
  MatrixDestroy(C22);
  test_finish(ctx);
}

static void t_zero_matrix_int(TestCtx* ctx) {
  test_start(ctx, "Zero matrix int defaults + closed ops");
  MatrixStatus st;

  Matrix* A = MatrixCreate(2, 3, GetIntRing(), &st);
  CHECK(ctx, A != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);

  print_matrix_boxed_ascii("A (should be all zeros)", A);

  int x = 1;
  for (size_t i = 0; i < A->rows; i++)
    for (size_t j = 0; j < A->cols; j++) {
      CHECK_STATUS(ctx, MatrixGet(A, i, j, &x), MATRIX_OK);
      CHECK(ctx, x == 0);
    }

  Matrix* N = MatrixNeg(A, &st);
  CHECK(ctx, N != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);
  print_matrix_boxed_ascii("-A", N);

  Matrix* S = MatrixAdd(A, A, &st);
  CHECK(ctx, S != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);
  print_matrix_boxed_ascii("A + A", S);

  MatrixDestroy(A);
  MatrixDestroy(N);
  MatrixDestroy(S);
  test_finish(ctx);
}

static void t_one_by_one(TestCtx* ctx) {
  test_start(ctx, "1x1 allowed (data layer)");
  MatrixStatus st;

  Matrix* A = MatrixCreate(1, 1, GetIntRing(), &st);
  CHECK(ctx, A != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);

  int v = 7;
  CHECK_STATUS(ctx, MatrixSet(A, 0, 0, &v), MATRIX_OK);
  print_matrix_boxed_ascii("A", A);

  Matrix* T = MatrixTranspose(A, &st);
  CHECK(ctx, T != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);
  print_matrix_boxed_ascii("Transpose(A)", T);

  int out = 0;
  CHECK_STATUS(ctx, MatrixGet(T, 0, 0, &out), MATRIX_OK);
  CHECK(ctx, out == 7);

  MatrixDestroy(A);
  MatrixDestroy(T);
  test_finish(ctx);
}

static void t_int_mul_known(TestCtx* ctx) {
  test_start(ctx, "int: Mul 2x3 * 3x2 known result");
  MatrixStatus st;

  const RingOps* R = GetIntRing();
  Matrix* A = MatrixCreate(2, 3, R, &st);
  Matrix* B = MatrixCreate(3, 2, R, &st);
  CHECK(ctx, A && B);

  int a_data[6] = {1, 2, 3, 4, 5, 6};
  int b_data[6] = {7, 8, 9, 10, 11, 12};

  size_t idx = 0;
  for (size_t i = 0; i < 2; i++)
    for (size_t j = 0; j < 3; j++)
      CHECK_STATUS(ctx, MatrixSet(A, i, j, &a_data[idx++]), MATRIX_OK);

  idx = 0;
  for (size_t i = 0; i < 3; i++)
    for (size_t j = 0; j < 2; j++)
      CHECK_STATUS(ctx, MatrixSet(B, i, j, &b_data[idx++]), MATRIX_OK);

  print_matrix_boxed_ascii("A", A);
  print_matrix_boxed_ascii("B", B);

  Matrix* C = MatrixMul(A, B, &st);
  CHECK(ctx, C != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);
  print_matrix_boxed_ascii("C = A*B", C);

  int x = 0;
  CHECK_STATUS(ctx, MatrixGet(C, 0, 0, &x), MATRIX_OK);
  CHECK(ctx, x == 58);
  CHECK_STATUS(ctx, MatrixGet(C, 0, 1, &x), MATRIX_OK);
  CHECK(ctx, x == 64);
  CHECK_STATUS(ctx, MatrixGet(C, 1, 0, &x), MATRIX_OK);
  CHECK(ctx, x == 139);
  CHECK_STATUS(ctx, MatrixGet(C, 1, 1, &x), MATRIX_OK);
  CHECK(ctx, x == 154);

  MatrixDestroy(A);
  MatrixDestroy(B);
  MatrixDestroy(C);
  test_finish(ctx);
}

static void t_transpose_int(TestCtx* ctx) {
  test_start(ctx, "int: Transpose 2x3 -> 3x2");
  MatrixStatus st;

  Matrix* A = MatrixCreate(2, 3, GetIntRing(), &st);
  CHECK(ctx, A != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);

  int v = 1;
  for (size_t i = 0; i < 2; i++)
    for (size_t j = 0; j < 3; j++)
      CHECK_STATUS(ctx, MatrixSet(A, i, j, &v), MATRIX_OK);
  v++;

  print_matrix_boxed_ascii("A", A);

  Matrix* T = MatrixTranspose(A, &st);
  CHECK(ctx, T != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);
  print_matrix_boxed_ascii("Transpose(A)", T);

  int x = 0;
  CHECK_STATUS(ctx, MatrixGet(T, 2, 1, &x), MATRIX_OK);
  CHECK(ctx, x == 6);

  MatrixDestroy(A);
  MatrixDestroy(T);
  test_finish(ctx);
}

static void t_complex_mul_small(TestCtx* ctx) {
  test_start(ctx, "complex: Mul 1x2 * 2x1 = 3+11i");
  MatrixStatus st;

  const RingOps* R = GetComplexRing();
  CHECK(ctx, R != NULL);

  Matrix* A = MatrixCreate(1, 2, R, &st);
  Matrix* B = MatrixCreate(2, 1, R, &st);
  CHECK(ctx, A && B);

  Complex c1 = {1, 1}, c2 = {2, 0};
  Complex d1 = {3, 0}, d2 = {0, 4};

  CHECK_STATUS(ctx, MatrixSet(A, 0, 0, &c1), MATRIX_OK);
  CHECK_STATUS(ctx, MatrixSet(A, 0, 1, &c2), MATRIX_OK);
  CHECK_STATUS(ctx, MatrixSet(B, 0, 0, &d1), MATRIX_OK);
  CHECK_STATUS(ctx, MatrixSet(B, 1, 0, &d2), MATRIX_OK);

  print_matrix_boxed_ascii("A", A);
  print_matrix_boxed_ascii("B", B);

  Matrix* C = MatrixMul(A, B, &st);
  CHECK(ctx, C != NULL);
  CHECK_STATUS(ctx, st, MATRIX_OK);
  print_matrix_boxed_ascii("C = A*B", C);

  Complex out;
  CHECK_STATUS(ctx, MatrixGet(C, 0, 0, &out), MATRIX_OK);
  Complex exp = {3, 11};
  CHECK(ctx, complex_eq_eps(&out, &exp, 1e-9));

  MatrixDestroy(A);
  MatrixDestroy(B);
  MatrixDestroy(C);
  test_finish(ctx);
}

static void t_type_mismatch(TestCtx* ctx) {
  test_start(ctx, "type mismatch: int + complex must fail");
  MatrixStatus st;

  Matrix* A = MatrixCreate(2, 3, GetIntRing(), &st);
  Matrix* B = MatrixCreate(2, 3, GetComplexRing(), &st);
  CHECK(ctx, A && B);

  print_matrix_boxed_ascii("A (int)", A);
  print_matrix_boxed_ascii("B (complex)", B);

  Matrix* C = MatrixAdd(A, B, &st);
  CHECK(ctx, C == NULL);
  CHECK_STATUS(ctx, st, MATRIX_ERR_TYPE_MISMATCH);

  MatrixDestroy(A);
  MatrixDestroy(B);
  test_finish(ctx);
}

int RunAllTests(void) {
  TestCtx ctx;
  memset(&ctx, 0, sizeof(ctx));

  printf("\n============================\n");
  printf(" TEST RUN (always print matrices)\n");
  printf("============================\n");

  t_create_invalid_sizes(&ctx);
  t_null_ring(&ctx);
  t_bounds_and_nulls(&ctx);
  t_shape_mismatch(&ctx);
  t_zero_matrix_int(&ctx);
  t_one_by_one(&ctx);

  t_int_mul_known(&ctx);
  t_transpose_int(&ctx);
  t_complex_mul_small(&ctx);
  t_type_mismatch(&ctx);

  printf("\n============================\n");
  printf(" SUMMARY\n");
  printf("============================\n");
  printf("Total checks:  %d\n", ctx.total_checks);
  printf("Failed checks: %d\n", ctx.total_failed);

  if (ctx.total_failed == 0) {
    printf("ALL TESTS PASSED\n");
    return 0;
  } else {
    printf("SOME TESTS FAILED\n");
    return 1;
  }
}