#include "matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* elem_ptr(const Matrix* m, size_t r, size_t c) {
    return (char*)m->data + (r * m->cols + c) * m->fi->elem_size;
}

const char* MatrixStatusToString(MatrixStatus st) {
    switch (st) {
        case MATRIX_OK: return "OK";
        case MATRIX_ERR_NULL: return "NULL argument";
        case MATRIX_ERR_ALLOC: return "Allocation failed";
        case MATRIX_ERR_BOUNDS: return "Index out of bounds";
        case MATRIX_ERR_TYPE_MISMATCH: return "Type mismatch";
        case MATRIX_ERR_SHAPE: return "Incompatible shapes";
        default: return "Unknown error";
    }
}

Matrix* MatrixCreate(size_t rows, size_t cols, const FieldInfo* fi, MatrixStatus* st) {
    if (st) *st = MATRIX_OK;
    if (!fi || rows == 0 || cols == 0) {
        if (st) *st = MATRIX_ERR_NULL;
        return NULL;
    }

    Matrix* m = (Matrix*)malloc(sizeof(Matrix));
    if (!m) {
        if (st) *st = MATRIX_ERR_ALLOC;
        return NULL;
    }

    m->rows = rows;
    m->cols = cols;
    m->fi = fi;

    size_t n = rows * cols;
    m->data = calloc(n, fi->elem_size);
    if (!m->data) {
        free(m);
        if (st) *st = MATRIX_ERR_ALLOC;
        return NULL;
    }

    if (fi->set_zero) {
        for (size_t i = 0; i < n; i++) {
            fi->set_zero((char*)m->data + i * fi->elem_size);
        }
    }
    return m;
}

void MatrixDestroy(Matrix* m) {
    if (!m) return;
    if (m->fi && m->fi->destroy && m->data) {
        size_t n = m->rows * m->cols;
        for (size_t i = 0; i < n; i++) {
            m->fi->destroy((char*)m->data + i * m->fi->elem_size);
        }
    }
    free(m->data);
    free(m);
}

MatrixStatus MatrixGet(const Matrix* m, size_t r, size_t c, void* out_elem) {
    if (!m || !out_elem) return MATRIX_ERR_NULL;
    if (r >= m->rows || c >= m->cols) return MATRIX_ERR_BOUNDS;
    m->fi->copy(out_elem, elem_ptr(m, r, c));
    return MATRIX_OK;
}

MatrixStatus MatrixSet(Matrix* m, size_t r, size_t c, const void* elem) {
    if (!m || !elem) return MATRIX_ERR_NULL;
    if (r >= m->rows || c >= m->cols) return MATRIX_ERR_BOUNDS;
    m->fi->copy(elem_ptr(m, r, c), elem);
    return MATRIX_OK;
}

void MatrixPrint(const Matrix* m) {
    if (!m) {
        printf("(null matrix)\n");
        return;
    }
    printf("%zux%zu\n", m->rows, m->cols);
    for (size_t i = 0; i < m->rows; i++) {
        for (size_t j = 0; j < m->cols; j++) {
            m->fi->print(elem_ptr(m, i, j), stdout);
            if (j + 1 != m->cols) printf("  ");
        }
        printf("\n");
    }
}

MatrixStatus MatrixRead(Matrix* m) {
    if (!m) return MATRIX_ERR_NULL;
    printf("Введите элементы матрицы (%zux%zu).\n", m->rows, m->cols);

    if (m->fi == GetIntFieldInfo()) {
        printf("Формат int: одно целое число на элемент.\n");
    } else if (m->fi == GetComplexFieldInfo()) {
        printf("Формат complex: два числа (re im) на элемент.\n");
    } else {
        printf("Формат: согласно FieldInfo->read.\n");
    }

    for (size_t i = 0; i < m->rows; i++) {
        for (size_t j = 0; j < m->cols; j++) {
            printf("[%zu,%zu] = ", i, j);
            if (m->fi->read(elem_ptr(m, i, j), stdin) != 0) {
                return MATRIX_ERR_SHAPE;
            }
        }
    }
    return MATRIX_OK;
}

static int same_type(const Matrix* a, const Matrix* b) {
    return a && b && a->fi == b->fi;
}

Matrix* MatrixAdd(const Matrix* a, const Matrix* b, MatrixStatus* st) {
    if (st) *st = MATRIX_OK;
    if (!a || !b) { if (st) *st = MATRIX_ERR_NULL; return NULL; }
    if (!same_type(a, b)) { if (st) *st = MATRIX_ERR_TYPE_MISMATCH; return NULL; }
    if (a->rows != b->rows || a->cols != b->cols) { if (st) *st = MATRIX_ERR_SHAPE; return NULL; }

    Matrix* r = MatrixCreate(a->rows, a->cols, a->fi, st);
    if (!r) return NULL;

    void* tmp = malloc(a->fi->elem_size);
    if (!tmp) { MatrixDestroy(r); if (st) *st = MATRIX_ERR_ALLOC; return NULL; }

    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < a->cols; j++) {
            a->fi->add(tmp, elem_ptr(a, i, j), elem_ptr(b, i, j));
            a->fi->copy(elem_ptr(r, i, j), tmp);
        }
    }

    free(tmp);
    return r;
}

Matrix* MatrixMul(const Matrix* a, const Matrix* b, MatrixStatus* st) {
    if (st) *st = MATRIX_OK;
    if (!a || !b) { if (st) *st = MATRIX_ERR_NULL; return NULL; }
    if (!same_type(a, b)) { if (st) *st = MATRIX_ERR_TYPE_MISMATCH; return NULL; }
    if (a->cols != b->rows) { if (st) *st = MATRIX_ERR_SHAPE; return NULL; }

    Matrix* r = MatrixCreate(a->rows, b->cols, a->fi, st);
    if (!r) return NULL;

    void* sum  = malloc(a->fi->elem_size);
    void* prod = malloc(a->fi->elem_size);
    void* tmp  = malloc(a->fi->elem_size);
    if (!sum || !prod || !tmp) {
        free(sum); free(prod); free(tmp);
        MatrixDestroy(r);
        if (st) *st = MATRIX_ERR_ALLOC;
        return NULL;
    }

    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < b->cols; j++) {
            a->fi->set_zero(sum);
            for (size_t k = 0; k < a->cols; k++) {
                a->fi->mul(prod, elem_ptr(a, i, k), elem_ptr(b, k, j));
                a->fi->add(tmp, sum, prod);
                a->fi->copy(sum, tmp);
            }
            a->fi->copy(elem_ptr(r, i, j), sum);
        }
    }

    free(sum); free(prod); free(tmp);
    return r;
}

Matrix* MatrixTranspose(const Matrix* a, MatrixStatus* st) {
    if (st) *st = MATRIX_OK;
    if (!a) { if (st) *st = MATRIX_ERR_NULL; return NULL; }

    Matrix* r = MatrixCreate(a->cols, a->rows, a->fi, st);
    if (!r) return NULL;

    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < a->cols; j++) {
            a->fi->copy(elem_ptr(r, j, i), elem_ptr(a, i, j));
        }
    }
    return r;
}