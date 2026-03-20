#include "matrix.h"

#include <stdlib.h>

static void* elem_ptr(const Matrix* m, size_t r, size_t c) {
  return (char*)m->data + (r * m->cols + c) * m->ring->elem_size;
}

static int same_type(const Matrix* a, const Matrix* b) {
  return a && b && a->ring == b->ring;
}

const char* MatrixStatusToString(MatrixStatus st) {
  switch (st) {
    case MATRIX_OK:
      return "OK";
    case MATRIX_ERR_NULL:
      return "NULL argument";
    case MATRIX_ERR_ALLOC:
      return "Allocation failed";
    case MATRIX_ERR_BOUNDS:
      return "Index out of bounds";
    case MATRIX_ERR_TYPE_MISMATCH:
      return "Type mismatch";
    case MATRIX_ERR_SHAPE:
      return "Incompatible shapes";
    default:
      return "Unknown error";
  }
}

Matrix* MatrixCreate(size_t rows, size_t cols, const RingOps* ring,
                     MatrixStatus* st) {
  if (st) *st = MATRIX_OK;
  if (!ring || rows == 0 || cols == 0) {
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
  m->ring = ring;

  size_t n = rows * cols;
  m->data = calloc(n, ring->elem_size);
  if (!m->data) {
    free(m);
    if (st) *st = MATRIX_ERR_ALLOC;
    return NULL;
  }

  if (ring->zero) {
    for (size_t i = 0; i < n; i++) {
      ring->zero((char*)m->data + i * ring->elem_size);
    }
  }

  return m;
}

void MatrixDestroy(Matrix* m) {
  if (!m) return;

  if (m->ring && m->ring->destroy && m->data) {
    size_t n = m->rows * m->cols;
    for (size_t i = 0; i < n; i++) {
      m->ring->destroy((char*)m->data + i * m->ring->elem_size);
    }
  }

  free(m->data);
  free(m);
}

MatrixStatus MatrixGet(const Matrix* m, size_t r, size_t c, void* out_elem) {
  if (!m || !out_elem) return MATRIX_ERR_NULL;
  if (r >= m->rows || c >= m->cols) return MATRIX_ERR_BOUNDS;
  m->ring->copy(out_elem, elem_ptr(m, r, c));
  return MATRIX_OK;
}

MatrixStatus MatrixSet(Matrix* m, size_t r, size_t c, const void* elem) {
  if (!m || !elem) return MATRIX_ERR_NULL;
  if (r >= m->rows || c >= m->cols) return MATRIX_ERR_BOUNDS;
  m->ring->copy(elem_ptr(m, r, c), elem);
  return MATRIX_OK;
}

Matrix* MatrixAdd(const Matrix* a, const Matrix* b, MatrixStatus* st) {
  if (st) *st = MATRIX_OK;
  if (!a || !b) {
    if (st) *st = MATRIX_ERR_NULL;
    return NULL;
  }
  if (!same_type(a, b)) {
    if (st) *st = MATRIX_ERR_TYPE_MISMATCH;
    return NULL;
  }
  if (a->rows != b->rows || a->cols != b->cols) {
    if (st) *st = MATRIX_ERR_SHAPE;
    return NULL;
  }

  Matrix* r = MatrixCreate(a->rows, a->cols, a->ring, st);
  if (!r) return NULL;

  for (size_t i = 0; i < a->rows; i++) {
    for (size_t j = 0; j < a->cols; j++) {
      void* out = elem_ptr(r, i, j);
      a->ring->add(out, elem_ptr(a, i, j), elem_ptr(b, i, j));
    }
  }
  return r;
}

Matrix* MatrixSub(const Matrix* a, const Matrix* b, MatrixStatus* st) {
  if (st) *st = MATRIX_OK;
  if (!a || !b) {
    if (st) *st = MATRIX_ERR_NULL;
    return NULL;
  }
  if (!same_type(a, b)) {
    if (st) *st = MATRIX_ERR_TYPE_MISMATCH;
    return NULL;
  }
  if (a->rows != b->rows || a->cols != b->cols) {
    if (st) *st = MATRIX_ERR_SHAPE;
    return NULL;
  }

  Matrix* r = MatrixCreate(a->rows, a->cols, a->ring, st);
  if (!r) return NULL;

  for (size_t i = 0; i < a->rows; i++) {
    for (size_t j = 0; j < a->cols; j++) {
      void* out = elem_ptr(r, i, j);
      a->ring->sub(out, elem_ptr(a, i, j), elem_ptr(b, i, j));
    }
  }
  return r;
}

Matrix* MatrixNeg(const Matrix* a, MatrixStatus* st) {
  if (st) *st = MATRIX_OK;
  if (!a) {
    if (st) *st = MATRIX_ERR_NULL;
    return NULL;
  }

  Matrix* r = MatrixCreate(a->rows, a->cols, a->ring, st);
  if (!r) return NULL;

  for (size_t i = 0; i < a->rows; i++) {
    for (size_t j = 0; j < a->cols; j++) {
      void* out = elem_ptr(r, i, j);
      a->ring->neg(out, elem_ptr(a, i, j));
    }
  }
  return r;
}

Matrix* MatrixMul(const Matrix* a, const Matrix* b, MatrixStatus* st) {
  if (st) *st = MATRIX_OK;
  if (!a || !b) {
    if (st) *st = MATRIX_ERR_NULL;
    return NULL;
  }
  if (!same_type(a, b)) {
    if (st) *st = MATRIX_ERR_TYPE_MISMATCH;
    return NULL;
  }
  if (a->cols != b->rows) {
    if (st) *st = MATRIX_ERR_SHAPE;
    return NULL;
  }

  Matrix* r = MatrixCreate(a->rows, b->cols, a->ring, st);
  if (!r) return NULL;

  void* prod = malloc(a->ring->elem_size);
  void* tmp = malloc(a->ring->elem_size);
  if (!prod || !tmp) {
    free(prod);
    free(tmp);
    MatrixDestroy(r);
    if (st) *st = MATRIX_ERR_ALLOC;
    return NULL;
  }

  for (size_t i = 0; i < a->rows; i++) {
    for (size_t j = 0; j < b->cols; j++) {
      void* sum = elem_ptr(r, i, j);
      a->ring->zero(sum);

      for (size_t k = 0; k < a->cols; k++) {
        a->ring->mul(prod, elem_ptr(a, i, k), elem_ptr(b, k, j));

        a->ring->add(tmp, sum, prod);
        a->ring->copy(sum, tmp);
      }
    }
  }

  free(prod);
  free(tmp);
  return r;
}

Matrix* MatrixTranspose(const Matrix* a, MatrixStatus* st) {
  if (st) *st = MATRIX_OK;
  if (!a) {
    if (st) *st = MATRIX_ERR_NULL;
    return NULL;
  }

  Matrix* r = MatrixCreate(a->cols, a->rows, a->ring, st);
  if (!r) return NULL;

  for (size_t i = 0; i < a->rows; i++) {
    for (size_t j = 0; j < a->cols; j++) {
      a->ring->copy(elem_ptr(r, j, i), elem_ptr(a, i, j));
    }
  }

  return r;
}