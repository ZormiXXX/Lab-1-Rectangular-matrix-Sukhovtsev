#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "matrix.h"
#include "tests.h"

static void flush_stdin_line(void) {
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF) {
  }
}

static int read_int_range(const char* prompt, int minv, int maxv, int* out) {
  int ok = 0;
  while (!ok) {
    printf("%s", prompt);
    int x;
    if (scanf("%d", &x) == 1 && x >= minv && x <= maxv) {
      *out = x;
      ok = 1;
    } else {
      printf("Некорректный ввод. Введите число от %d до %d.\n", minv, maxv);
      flush_stdin_line();
    }
  }
  return 0;
}

static int read_ull_min(const char* prompt, unsigned long long minv,
                        unsigned long long* out) {
  int ok = 0;
  while (!ok) {
    printf("%s", prompt);
    unsigned long long x;
    if (scanf("%llu", &x) == 1 && x >= minv) {
      *out = x;
      ok = 1;
    } else {
      printf("Некорректный ввод. Введите число >= %llu.\n", minv);
      flush_stdin_line();
    }
  }
  return 0;
}

static void print_error(MatrixStatus st) {
  printf("Ошибка: %s\n", MatrixStatusToString(st));
}

static const char* ring_name(const RingOps* ring) {
  if (ring == GetIntRing()) return "int";
  if (ring == GetComplexRing()) return "complex";
  return "custom";
}

static void print_format_hint(const RingOps* ring) {
  if (ring == GetIntRing()) {
    printf("Формат int: одно целое число (пример: 42)\n");
  } else if (ring == GetComplexRing()) {
    printf("Формат complex: два числа re im (пример: 3 -2.5)\n");
  } else {
    printf("Формат: согласно ring->read\n");
  }
}

static void format_elem_to_buf(const RingOps* ring, const void* elem, char* buf,
                               size_t bufsz) {
  if (ring == GetIntRing()) {
    snprintf(buf, bufsz, "%d", *(const int*)elem);
  } else if (ring == GetComplexRing()) {
    const Complex* z = (const Complex*)elem;
    if (z->im >= 0)
      snprintf(buf, bufsz, "%.6g+%.6gi", z->re, z->im);
    else
      snprintf(buf, bufsz, "%.6g%.6gi", z->re, z->im);
  } else {
    snprintf(buf, bufsz, "<elem>");
  }
}

static void print_pad_spaces(int n) {
  for (int i = 0; i < n; i++) putchar(' ');
}

static void print_hline_ascii_part(const size_t* w, size_t cols) {
  putchar('+');
  for (size_t j = 0; j < cols; j++) {
    for (size_t k = 0; k < w[j] + 2; k++) putchar('-');
    putchar('+');
  }
}

static void print_hline_ascii(const size_t* w, size_t cols) {
  print_hline_ascii_part(w, cols);
  putchar('\n');
}

static int matrix_row_line_width(const size_t* w, size_t cols) {
  int total = 1;
  for (size_t j = 0; j < cols; j++) total += (int)(w[j] + 2 + 1);
  return total;
}

static MatrixStatus compute_col_widths(const Matrix* m, size_t* widths) {
  void* tmp = malloc(m->ring->elem_size);
  if (!tmp) return MATRIX_ERR_ALLOC;

  char cell[128];
  for (size_t j = 0; j < m->cols; j++) widths[j] = 0;

  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++) {
      MatrixStatus st = MatrixGet(m, i, j, tmp);
      if (st != MATRIX_OK) {
        free(tmp);
        return st;
      }
      format_elem_to_buf(m->ring, tmp, cell, sizeof(cell));
      size_t len = strlen(cell);
      if (len > widths[j]) widths[j] = len;
    }
  }

  free(tmp);
  return MATRIX_OK;
}

static MatrixStatus print_matrix_boxed_ascii(const char* title,
                                             const Matrix* m) {
  if (!m) {
    printf("\n%s: (не задана)\n", title);
    return MATRIX_OK;
  }

  printf("\n%s (%zux%zu)\n", title, m->rows, m->cols);

  void* tmp = malloc(m->ring->elem_size);
  if (!tmp) return MATRIX_ERR_ALLOC;

  size_t* w = (size_t*)calloc(m->cols, sizeof(size_t));
  if (!w) {
    free(tmp);
    return MATRIX_ERR_ALLOC;
  }

  MatrixStatus st = compute_col_widths(m, w);
  if (st != MATRIX_OK) {
    free(tmp);
    free(w);
    return st;
  }

  char cell[128];

  print_hline_ascii(w, m->cols);
  for (size_t i = 0; i < m->rows; i++) {
    putchar('|');
    for (size_t j = 0; j < m->cols; j++) {
      st = MatrixGet(m, i, j, tmp);
      if (st != MATRIX_OK) {
        free(tmp);
        free(w);
        return st;
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
  return MATRIX_OK;
}

static void print_matrix_row_ascii(const Matrix* m, const size_t* w,
                                   size_t row_index, void* tmp) {
  putchar('|');
  for (size_t j = 0; j < m->cols; j++) {
    MatrixGet(m, row_index, j, tmp);
    char cell[128];
    format_elem_to_buf(m->ring, tmp, cell, sizeof(cell));
    printf(" %*s ", (int)w[j], cell);
    putchar('|');
  }
}

static void print_scene_ABC(const Matrix* A, const Matrix* B, const Matrix* C,
                            const char* op) {
  printf("\n--- СЦЕНА: A %s B = C ---\n", op);

  if (!A || !B || !C) {
    printf("Невозможно показать сцену: одна из матриц не задана.\n");
    return;
  }

  size_t* wA = (size_t*)calloc(A->cols, sizeof(size_t));
  size_t* wB = (size_t*)calloc(B->cols, sizeof(size_t));
  size_t* wC = (size_t*)calloc(C->cols, sizeof(size_t));
  void* tmp = malloc(A->ring->elem_size);

  if (!wA || !wB || !wC || !tmp) {
    free(wA);
    free(wB);
    free(wC);
    free(tmp);
    printf("(alloc failed)\n");
    return;
  }

  if (compute_col_widths(A, wA) != MATRIX_OK ||
      compute_col_widths(B, wB) != MATRIX_OK ||
      compute_col_widths(C, wC) != MATRIX_OK) {
    free(wA);
    free(wB);
    free(wC);
    free(tmp);
    printf("(width compute failed)\n");
    return;
  }

  const int gap = 2;
  const int op_width = 3;
  const int eq_width = 3;

  int wLineA = matrix_row_line_width(wA, A->cols);
  int wLineB = matrix_row_line_width(wB, B->cols);
  int wLineC = matrix_row_line_width(wC, C->cols);

  printf("A (%zux%zu)", A->rows, A->cols);
  print_pad_spaces(gap + op_width + gap);
  printf("B (%zux%zu)", B->rows, B->cols);
  print_pad_spaces(gap + eq_width + gap);
  printf("C (%zux%zu)\n", C->rows, C->cols);

  print_hline_ascii_part(wA, A->cols);
  print_pad_spaces(gap);
  print_pad_spaces(op_width);
  print_pad_spaces(gap);
  print_hline_ascii_part(wB, B->cols);
  print_pad_spaces(gap);
  print_pad_spaces(eq_width);
  print_pad_spaces(gap);
  print_hline_ascii_part(wC, C->cols);
  putchar('\n');

  size_t max_rows = A->rows;
  if (B->rows > max_rows) max_rows = B->rows;
  if (C->rows > max_rows) max_rows = C->rows;

  for (size_t i = 0; i < max_rows; i++) {
    if (i < A->rows)
      print_matrix_row_ascii(A, wA, i, tmp);
    else {
      putchar('|');
      print_pad_spaces(wLineA - 2);
      putchar('|');
    }

    print_pad_spaces(gap);

    if (i == max_rows / 2)
      printf(" %c ", op[0]);
    else
      print_pad_spaces(op_width);

    print_pad_spaces(gap);

    if (i < B->rows)
      print_matrix_row_ascii(B, wB, i, tmp);
    else {
      putchar('|');
      print_pad_spaces(wLineB - 2);
      putchar('|');
    }

    print_pad_spaces(gap);

    if (i == max_rows / 2)
      printf(" = ");
    else
      print_pad_spaces(eq_width);

    print_pad_spaces(gap);

    if (i < C->rows)
      print_matrix_row_ascii(C, wC, i, tmp);
    else {
      putchar('|');
      print_pad_spaces(wLineC - 2);
      putchar('|');
    }

    putchar('\n');

    if (i + 1 != max_rows) {
      print_hline_ascii_part(wA, A->cols);
      print_pad_spaces(gap);
      print_pad_spaces(op_width);
      print_pad_spaces(gap);
      print_hline_ascii_part(wB, B->cols);
      print_pad_spaces(gap);
      print_pad_spaces(eq_width);
      print_pad_spaces(gap);
      print_hline_ascii_part(wC, C->cols);
      putchar('\n');
    }
  }

  print_hline_ascii_part(wA, A->cols);
  print_pad_spaces(gap);
  print_pad_spaces(op_width);
  print_pad_spaces(gap);
  print_hline_ascii_part(wB, B->cols);
  print_pad_spaces(gap);
  print_pad_spaces(eq_width);
  print_pad_spaces(gap);
  print_hline_ascii_part(wC, C->cols);
  putchar('\n');

  free(wA);
  free(wB);
  free(wC);
  free(tmp);
}

static MatrixStatus fill_matrix_manual(Matrix* m) {
  if (!m) return MATRIX_ERR_NULL;

  void* tmp = malloc(m->ring->elem_size);
  if (!tmp) return MATRIX_ERR_ALLOC;

  printf("Введите элементы по строкам.\n");
  print_format_hint(m->ring);

  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++) {
      int got = 0;
      while (!got) {
        printf("  a[%zu][%zu] = ", i, j);
        if (m->ring->read(tmp, stdin) == 0) {
          MatrixStatus st = MatrixSet(m, i, j, tmp);
          if (st == MATRIX_OK)
            got = 1;
          else {
            free(tmp);
            return st;
          }
        } else {
          printf("Ошибка ввода. Повторите.\n");
          flush_stdin_line();
        }
      }
    }
  }

  free(tmp);
  return MATRIX_OK;
}

static MatrixStatus fill_matrix_zero(Matrix* m) {
  if (!m) return MATRIX_ERR_NULL;

  void* tmp = malloc(m->ring->elem_size);
  if (!tmp) return MATRIX_ERR_ALLOC;

  m->ring->zero(tmp);
  for (size_t i = 0; i < m->rows; i++)
    for (size_t j = 0; j < m->cols; j++) {
      MatrixStatus st = MatrixSet(m, i, j, tmp);
      if (st != MATRIX_OK) {
        free(tmp);
        return st;
      }
    }

  free(tmp);
  return MATRIX_OK;
}

static int rand_int_in_range(int lo, int hi) {
  int span = hi - lo + 1;
  int r = rand() % span;
  return lo + r;
}

static MatrixStatus fill_matrix_random(Matrix* m) {
  if (!m) return MATRIX_ERR_NULL;

  if (m->ring == GetIntRing()) {
    for (size_t i = 0; i < m->rows; i++)
      for (size_t j = 0; j < m->cols; j++) {
        int v = rand_int_in_range(-9, 9);
        MatrixStatus st = MatrixSet(m, i, j, &v);
        if (st != MATRIX_OK) return st;
      }
  } else if (m->ring == GetComplexRing()) {
    for (size_t i = 0; i < m->rows; i++)
      for (size_t j = 0; j < m->cols; j++) {
        Complex z;
        z.re = (double)rand_int_in_range(-5, 5);
        z.im = (double)rand_int_in_range(-5, 5);
        MatrixStatus st = MatrixSet(m, i, j, &z);
        if (st != MATRIX_OK) return st;
      }
  } else {
    return fill_matrix_zero(m);
  }

  return MATRIX_OK;
}

static Matrix* create_matrix_interactive(const RingOps* ring) {
  unsigned long long rr = 0, cc = 0;
  read_ull_min("Строки (>=1): ", 1, &rr);
  read_ull_min("Столбцы (>=1): ", 1, &cc);

  MatrixStatus st;
  Matrix* m = MatrixCreate((size_t)rr, (size_t)cc, ring, &st);
  if (!m) {
    print_error(st);
    return NULL;
  }

  printf("\nЗаполнение матрицы:\n");
  printf("  1) Вручную\n");
  printf("  2) Нулями\n");
  printf("  3) Случайно (малые значения)\n");

  int mode = 1;
  read_int_range("Выберите способ (1-3): ", 1, 3, &mode);

  if (mode == 1)
    st = fill_matrix_manual(m);
  else if (mode == 2)
    st = fill_matrix_zero(m);
  else
    st = fill_matrix_random(m);

  if (st != MATRIX_OK) {
    print_error(st);
    MatrixDestroy(m);
    return NULL;
  }

  return m;
}

typedef struct AppState {
  const RingOps* ring;
  Matrix* A;
  Matrix* B;
  Matrix* C;
  char last_op;
  int has_result;
} AppState;

static void state_init(AppState* s) {
  s->ring = GetIntRing();
  s->A = NULL;
  s->B = NULL;
  s->C = NULL;
  s->last_op = '?';
  s->has_result = 0;
}

static void state_clear_matrix(Matrix** pm) {
  if (*pm) {
    MatrixDestroy(*pm);
    *pm = NULL;
  }
}

static void state_reset_result(AppState* s) {
  state_clear_matrix(&s->C);
  s->has_result = 0;
  s->last_op = '?';
}

static void show_status(const AppState* s) {
  printf("\n[Состояние]\n");
  printf("  Тип элементов: %s\n", ring_name(s->ring));
  printf("  A: %s\n", s->A ? "задана" : "нет");
  printf("  B: %s\n", s->B ? "задана" : "нет");
  printf("  C: %s\n", s->C ? "задан" : "нет");
  if (s->has_result) printf("  Последняя операция: %c\n", s->last_op);
}

static void menu_main(void) {
  printf("\n=================================================\n");
  printf("  Matrix Lab\n");
  printf("=================================================\n");
  printf("  1) Выбрать тип элементов (int/complex)\n");
  printf("  2) Ввести/создать матрицу A\n");
  printf("  3) Ввести/создать матрицу B\n");
  printf("  4) Показать матрицы A и B\n");
  printf("  5) Выполнить операцию (A+B, A-B, -A, A*B, T(A))\n");
  printf("  6) Показать результат C\n");
  printf("  7) Показать сцену A op B = C\n");
  printf("  8) Очистить A/B/C\n");
  printf("  9) Запустить тесты\n");
  printf("  0) Выход\n");
}

static const RingOps* choose_type_menu(void) {
  printf("\nТип элементов:\n");
  printf("  1) int\n");
  printf("  2) complex\n");
  int t = 1;
  read_int_range("Выберите (1-2): ", 1, 2, &t);
  return (t == 1) ? GetIntRing() : GetComplexRing();
}

static int choose_operation_menu(void) {
  printf("\nОперации:\n");
  printf("  1) C = A + B\n");
  printf("  2) C = A - B\n");
  printf("  3) C = -A\n");
  printf("  4) C = A * B\n");
  printf("  5) C = Transpose(A)\n");
  int op = 1;
  read_int_range("Выберите (1-5): ", 1, 5, &op);
  return op;
}

static void apply_ring_change(AppState* s, const RingOps* new_ring) {
  if (new_ring && new_ring != s->ring) {
    s->ring = new_ring;
    state_clear_matrix(&s->A);
    state_clear_matrix(&s->B);
    state_reset_result(s);
    printf("Тип изменён. Матрицы очищены.\n");
  }
}

static void create_A(AppState* s) {
  state_clear_matrix(&s->A);
  state_reset_result(s);
  Matrix* m = create_matrix_interactive(s->ring);
  if (m) {
    s->A = m;
    (void)print_matrix_boxed_ascii("A", s->A);
  }
}

static void create_B(AppState* s) {
  state_clear_matrix(&s->B);
  state_reset_result(s);
  Matrix* m = create_matrix_interactive(s->ring);
  if (m) {
    s->B = m;
    (void)print_matrix_boxed_ascii("B", s->B);
  }
}

static void show_AB(const AppState* s) {
  (void)print_matrix_boxed_ascii("A", s->A);
  (void)print_matrix_boxed_ascii("B", s->B);
}

static void show_C(const AppState* s) {
  (void)print_matrix_boxed_ascii("C", s->C);
}

static void clear_all(AppState* s) {
  state_clear_matrix(&s->A);
  state_clear_matrix(&s->B);
  state_reset_result(s);
  printf("Все матрицы очищены.\n");
}

static void do_operation(AppState* s) {
  state_reset_result(s);

  int op = choose_operation_menu();
  MatrixStatus st = MATRIX_OK;

  if (op == 1 || op == 2 || op == 4) {
    if (!s->A || !s->B) {
      printf("Нужно сначала задать матрицы A и B.\n");
    } else {
      Matrix* C = NULL;
      if (op == 1) {
        C = MatrixAdd(s->A, s->B, &st);
        s->last_op = '+';
      } else if (op == 2) {
        C = MatrixSub(s->A, s->B, &st);
        s->last_op = '-';
      } else {
        C = MatrixMul(s->A, s->B, &st);
        s->last_op = '*';
      }

      if (!C) {
        print_error(st);
      } else {
        s->C = C;
        s->has_result = 1;
        (void)print_matrix_boxed_ascii("A", s->A);
        (void)print_matrix_boxed_ascii("B", s->B);
        (void)print_matrix_boxed_ascii("C", s->C);
      }
    }
  } else if (op == 3) {
    if (!s->A) {
      printf("Нужно сначала задать матрицу A.\n");
    } else {
      Matrix* C = MatrixNeg(s->A, &st);
      s->last_op = 'N';
      if (!C) {
        print_error(st);
      } else {
        s->C = C;
        s->has_result = 1;
        (void)print_matrix_boxed_ascii("A", s->A);
        (void)print_matrix_boxed_ascii("C = -A", s->C);
      }
    }
  } else {
    if (!s->A) {
      printf("Нужно сначала задать матрицу A.\n");
    } else {
      Matrix* C = MatrixTranspose(s->A, &st);
      s->last_op = 'T';
      if (!C) {
        print_error(st);
      } else {
        s->C = C;
        s->has_result = 1;
        (void)print_matrix_boxed_ascii("A", s->A);
        (void)print_matrix_boxed_ascii("C = Transpose(A)", s->C);
      }
    }
  }
}

static void show_scene(const AppState* s) {
  if (!s->has_result || !s->C) {
    printf("Сначала выполните операцию, чтобы получить C.\n");
  } else {
    if (s->last_op == '+' || s->last_op == '-' || s->last_op == '*') {
      char op_str[2];
      op_str[0] = s->last_op;
      op_str[1] = '\0';
      print_scene_ABC(s->A, s->B, s->C, op_str);
    } else if (s->last_op == 'N') {
      printf("\n--- СЦЕНА: -A = C ---\n");
      (void)print_matrix_boxed_ascii("A", s->A);
      (void)print_matrix_boxed_ascii("C", s->C);
    } else if (s->last_op == 'T') {
      printf("\n--- СЦЕНА: Transpose(A) = C ---\n");
      (void)print_matrix_boxed_ascii("A", s->A);
      (void)print_matrix_boxed_ascii("C", s->C);
    } else {
      printf("Неизвестная операция.\n");
    }
  }
}

int AppRun(void) {
  srand((unsigned)time(NULL));

  AppState s;
  state_init(&s);

  int done = 0;
  while (!done) {
    show_status(&s);
    menu_main();

    int choice = 0;
    read_int_range("Выберите пункт (0-9): ", 0, 9, &choice);

    if (choice == 0) {
      done = 1;
    } else if (choice == 1) {
      const RingOps* nr = choose_type_menu();
      apply_ring_change(&s, nr);
    } else if (choice == 2) {
      create_A(&s);
    } else if (choice == 3) {
      create_B(&s);
    } else if (choice == 4) {
      show_AB(&s);
    } else if (choice == 5) {
      do_operation(&s);
    } else if (choice == 6) {
      show_C(&s);
    } else if (choice == 7) {
      show_scene(&s);
    } else if (choice == 8) {
      clear_all(&s);
    } else {
      int rc = RunAllTests();
      printf("\nТесты завершены: %s\n", (rc == 0 ? "УСПЕХ" : "ЕСТЬ ОШИБКИ"));
    }
  }

  clear_all(&s);
  printf("Выход.\n");
  return 0;
}