#include "matrix.h"
#include "tests.h"
#include <stdio.h>
#include <stdlib.h>

static int read_size_t(const char* prompt, size_t* out) {
    printf("%s", prompt);
    unsigned long long x;
    if (scanf("%llu", &x) != 1) return 1;
    if (x == 0) return 1;
    *out = (size_t)x;
    return 0;
}

static int read_int_menu(const char* prompt, int minv, int maxv, int* out) {
    printf("%s", prompt);
    int x;
    if (scanf("%d", &x) != 1) return 1;
    if (x < minv || x > maxv) return 1;
    *out = x;
    return 0;
}

static void print_error(MatrixStatus st) {
    printf("Ошибка: %s\n", MatrixStatusToString(st));
}

static Matrix* create_and_fill(const FieldInfo* fi) {
    size_t r, c;
    if (read_size_t("Введите число строк: ", &r) != 0) { printf("Некорректный ввод.\n"); return NULL; }
    if (read_size_t("Введите число столбцов: ", &c) != 0) { printf("Некорректный ввод.\n"); return NULL; }

    if (r == c) {
        printf("По условию варианта нужна прямоугольная матрица: число строк не должно равняться числу столбцов.\n");
        return NULL;
    }

    MatrixStatus st;
    Matrix* m = MatrixCreate(r, c, fi, &st);
    if (!m) { print_error(st); return NULL; }

    st = MatrixRead(m);
    if (st != MATRIX_OK) { print_error(st); MatrixDestroy(m); return NULL; }

    return m;
}

int main(void) {
    while (1) {
        printf("\n=== ЛР-1 (Вариант 22): Прямоугольная матрица (int, complex) ===\n");
        printf("1) Запустить тесты\n");
        printf("2) Ручной режим (создать матрицы и выполнить операции)\n");
        printf("0) Выход\n");

        int mode;
        if (read_int_menu("Выберите пункт: ", 0, 2, &mode) != 0) {
            printf("Некорректный ввод.\n");
            return 1;
        }
        if (mode == 0) break;

        if (mode == 1) {
            RunAllTests();
            continue;
        }

        printf("\nТип элементов:\n");
        printf("1) int\n");
        printf("2) complex (ввод: re im)\n");
        int t;
        if (read_int_menu("Выберите тип: ", 1, 2, &t) != 0) { printf("Некорректный ввод.\n"); continue; }

        const FieldInfo* fi = (t == 1) ? GetIntFieldInfo() : GetComplexFieldInfo();
        if (!fi) { printf("Не удалось инициализировать FieldInfo.\n"); continue; }

        printf("\nОперации:\n");
        printf("1) A + B\n");
        printf("2) A * B\n");
        printf("3) Transpose(A)\n");

        int op;
        if (read_int_menu("Выберите операцию: ", 1, 3, &op) != 0) { printf("Некорректный ввод.\n"); continue; }

        MatrixStatus st = MATRIX_OK;

        if (op == 1) {
            printf("\n--- Ввод матрицы A ---\n");
            Matrix* A = create_and_fill(fi);
            if (!A) continue;

            printf("\n--- Ввод матрицы B ---\n");
            Matrix* B = create_and_fill(fi);
            if (!B) { MatrixDestroy(A); continue; }

            Matrix* C = MatrixAdd(A, B, &st);
            if (!C) {
                print_error(st);
            } else {
                printf("\nA:\n"); MatrixPrint(A);
                printf("\nB:\n"); MatrixPrint(B);
                printf("\nA+B:\n"); MatrixPrint(C);
                MatrixDestroy(C);
            }

            MatrixDestroy(A);
            MatrixDestroy(B);
        } else if (op == 2) {
            printf("\n--- Ввод матрицы A ---\n");
            Matrix* A = create_and_fill(fi);
            if (!A) continue;

            printf("\n--- Ввод матрицы B ---\n");
            Matrix* B = create_and_fill(fi);
            if (!B) { MatrixDestroy(A); continue; }

            Matrix* C = MatrixMul(A, B, &st);
            if (!C) {
                print_error(st);
            } else {
                printf("\nA:\n"); MatrixPrint(A);
                printf("\nB:\n"); MatrixPrint(B);
                printf("\nA*B:\n"); MatrixPrint(C);
                MatrixDestroy(C);
            }

            MatrixDestroy(A);
            MatrixDestroy(B);
        } else {
            printf("\n--- Ввод матрицы A ---\n");
            Matrix* A = create_and_fill(fi);
            if (!A) continue;

            Matrix* Tm = MatrixTranspose(A, &st);
            if (!Tm) {
                print_error(st);
            } else {
                printf("\nA:\n"); MatrixPrint(A);
                printf("\nTranspose(A):\n"); MatrixPrint(Tm);
                MatrixDestroy(Tm);
            }
            MatrixDestroy(A);
        }
    }

    printf("Выход.\n");
    return 0;
}