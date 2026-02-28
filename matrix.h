#ifndef MATRIX_H
#define MATRIX_H

#include "field_info.h"
#include <stddef.h>

typedef enum MatrixStatus {
    MATRIX_OK = 0,
    MATRIX_ERR_NULL = 1,
    MATRIX_ERR_ALLOC = 2,
    MATRIX_ERR_BOUNDS = 3,
    MATRIX_ERR_TYPE_MISMATCH = 4,
    MATRIX_ERR_SHAPE = 5
} MatrixStatus;

typedef struct Matrix {
    size_t rows;
    size_t cols;
    const FieldInfo* fi; 
    void* data;          
} Matrix;

Matrix* MatrixCreate(size_t rows, size_t cols, const FieldInfo* fi, MatrixStatus* st);
void    MatrixDestroy(Matrix* m);

MatrixStatus MatrixGet(const Matrix* m, size_t r, size_t c, void* out_elem);
MatrixStatus MatrixSet(Matrix* m, size_t r, size_t c, const void* elem);

void MatrixPrint(const Matrix* m);
MatrixStatus MatrixRead(Matrix* m); 

Matrix* MatrixAdd(const Matrix* a, const Matrix* b, MatrixStatus* st);
Matrix* MatrixMul(const Matrix* a, const Matrix* b, MatrixStatus* st);
Matrix* MatrixTranspose(const Matrix* a, MatrixStatus* st);

const char* MatrixStatusToString(MatrixStatus st);

#endif 