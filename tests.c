#include "matrix.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static int complex_eq(const Complex* a, const Complex* b, double eps) {
    return fabs(a->re - b->re) <= eps && fabs(a->im - b->im) <= eps;
}

static void test_int_add(void) {
    MatrixStatus st;
    const FieldInfo* fi = GetIntFieldInfo();
    assert(fi);

    Matrix* A = MatrixCreate(2, 3, fi, &st);
    assert(A && st == MATRIX_OK);
    Matrix* B = MatrixCreate(2, 3, fi, &st);
    assert(B && st == MATRIX_OK);

    int v = 1;
    for (size_t i = 0; i < 2; i++)
        for (size_t j = 0; j < 3; j++) {
            MatrixSet(A, i, j, &v);
            int w = (int)(10 + v);
            MatrixSet(B, i, j, &w);
            v++;
        }

    Matrix* C = MatrixAdd(A, B, &st);
    assert(C && st == MATRIX_OK);

    int x;
    MatrixGet(C, 0, 0, &x); assert(x == 12); 
    MatrixGet(C, 1, 2, &x); assert(x == 6 + 16); 

    MatrixDestroy(A); MatrixDestroy(B); MatrixDestroy(C);
}

static void test_int_mul(void) {
    MatrixStatus st;
    const FieldInfo* fi = GetIntFieldInfo();
    Matrix* A = MatrixCreate(2, 3, fi, &st);
    Matrix* B = MatrixCreate(3, 2, fi, &st);
    assert(A && B);

    int a_data[6] = {1,2,3,4,5,6};
    size_t idx = 0;
    for (size_t i = 0; i < 2; i++)
        for (size_t j = 0; j < 3; j++)
            MatrixSet(A, i, j, &a_data[idx++]);

    int b_data[6] = {7,8,9,10,11,12};
    idx = 0;
    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 2; j++)
            MatrixSet(B, i, j, &b_data[idx++]);

    Matrix* C = MatrixMul(A, B, &st);
    assert(C && st == MATRIX_OK);

    int x;
    MatrixGet(C, 0, 0, &x); assert(x == 58);
    MatrixGet(C, 0, 1, &x); assert(x == 64);
    MatrixGet(C, 1, 0, &x); assert(x == 139);
    MatrixGet(C, 1, 1, &x); assert(x == 154);

    MatrixDestroy(A); MatrixDestroy(B); MatrixDestroy(C);
}

static void test_transpose_int(void) {
    MatrixStatus st;
    const FieldInfo* fi = GetIntFieldInfo();
    Matrix* A = MatrixCreate(2, 3, fi, &st);
    assert(A);

    int v = 1;
    for (size_t i = 0; i < 2; i++)
        for (size_t j = 0; j < 3; j++)
            MatrixSet(A, i, j, &v), v++;

    Matrix* T = MatrixTranspose(A, &st);
    assert(T && st == MATRIX_OK);
    assert(T->rows == 3 && T->cols == 2);

    int x;
    MatrixGet(T, 2, 1, &x); assert(x == 6); 

    MatrixDestroy(A); MatrixDestroy(T);
}

static void test_complex_mul_small(void) {
    MatrixStatus st;
    const FieldInfo* fi = GetComplexFieldInfo();
    assert(fi);

    Matrix* A = MatrixCreate(1, 2, fi, &st);
    Matrix* B = MatrixCreate(2, 1, fi, &st);
    assert(A && B);

    Complex c1 = {1,1}, c2 = {2,0};
    MatrixSet(A, 0, 0, &c1);
    MatrixSet(A, 0, 1, &c2);

    Complex d1 = {3,0}, d2 = {0,4};
    MatrixSet(B, 0, 0, &d1);
    MatrixSet(B, 1, 0, &d2);

    Matrix* C = MatrixMul(A, B, &st);
    assert(C && st == MATRIX_OK);

    Complex out;
    MatrixGet(C, 0, 0, &out);
    Complex exp = {3,11};
    assert(complex_eq(&out, &exp, 1e-9));

    MatrixDestroy(A); MatrixDestroy(B); MatrixDestroy(C);
}

static void test_type_mismatch(void) {
    MatrixStatus st;
    Matrix* A = MatrixCreate(2, 2, GetIntFieldInfo(), &st);
    Matrix* B = MatrixCreate(2, 2, GetComplexFieldInfo(), &st);
    assert(A && B);

    Matrix* C = MatrixAdd(A, B, &st);
    assert(C == NULL);
    assert(st == MATRIX_ERR_TYPE_MISMATCH);

    MatrixDestroy(A); MatrixDestroy(B);
}

void RunAllTests(void) {
    test_int_add();
    test_int_mul();
    test_transpose_int();
    test_complex_mul_small();
    test_type_mismatch();
    printf("ALL TESTS PASSED\n");
}