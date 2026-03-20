#include "algebra.h"

#include <stdlib.h>

static void int_zero(void* out) { *(int*)out = 0; }
static void int_one(void* out) { *(int*)out = 1; }
static void int_copy(void* dst, const void* src) {
  *(int*)dst = *(const int*)src;
}

static int int_add(void* out, const void* a, const void* b) {
  *(int*)out = *(const int*)a + *(const int*)b;
  return 0;
}
static int int_neg(void* out, const void* a) {
  *(int*)out = -(*(const int*)a);
  return 0;
}
static int int_sub(void* out, const void* a, const void* b) {
  *(int*)out = *(const int*)a - *(const int*)b;
  return 0;
}
static int int_mul(void* out, const void* a, const void* b) {
  *(int*)out = (*(const int*)a) * (*(const int*)b);
  return 0;
}
static int int_eq(const void* a, const void* b) {
  return (*(const int*)a == *(const int*)b) ? 1 : 0;
}

static void int_print(const void* x, FILE* out) {
  fprintf(out, "%d", *(const int*)x);
}
static int int_read(void* x, FILE* in) {
  return (fscanf(in, "%d", (int*)x) == 1) ? 0 : 1;
}

static void c_zero(void* out) {
  ((Complex*)out)->re = 0.0;
  ((Complex*)out)->im = 0.0;
}
static void c_one(void* out) {
  ((Complex*)out)->re = 1.0;
  ((Complex*)out)->im = 0.0;
}
static void c_copy(void* dst, const void* src) {
  *(Complex*)dst = *(const Complex*)src;
}

static int c_add(void* out, const void* a, const void* b) {
  const Complex* x = (const Complex*)a;
  const Complex* y = (const Complex*)b;
  Complex* r = (Complex*)out;
  r->re = x->re + y->re;
  r->im = x->im + y->im;
  return 0;
}
static int c_neg(void* out, const void* a) {
  const Complex* x = (const Complex*)a;
  Complex* r = (Complex*)out;
  r->re = -x->re;
  r->im = -x->im;
  return 0;
}
static int c_sub(void* out, const void* a, const void* b) {
  const Complex* x = (const Complex*)a;
  const Complex* y = (const Complex*)b;
  Complex* r = (Complex*)out;
  r->re = x->re - y->re;
  r->im = x->im - y->im;
  return 0;
}
static int c_mul(void* out, const void* a, const void* b) {
  const Complex* x = (const Complex*)a;
  const Complex* y = (const Complex*)b;
  Complex* r = (Complex*)out;
  r->re = x->re * y->re - x->im * y->im;
  r->im = x->re * y->im + x->im * y->re;
  return 0;
}
static int c_eq(const void* a, const void* b) {
  const Complex* x = (const Complex*)a;
  const Complex* y = (const Complex*)b;
  return (x->re == y->re && x->im == y->im) ? 1 : 0;
}

static void c_print(const void* x, FILE* out) {
  const Complex* c = (const Complex*)x;
  if (c->im >= 0)
    fprintf(out, "%.6g+%.6gi", c->re, c->im);
  else
    fprintf(out, "%.6g%.6gi", c->re, c->im);
}
static int c_read(void* x, FILE* in) {
  Complex* c = (Complex*)x;
  return (fscanf(in, "%lf %lf", &c->re, &c->im) == 2) ? 0 : 1;
}

static const RingOps* INT_RING = NULL;
static const RingOps* COMPLEX_RING = NULL;

const RingOps* GetIntRing(void) {
  if (!INT_RING) {
    RingOps* r = (RingOps*)malloc(sizeof(RingOps));
    if (!r) return NULL;

    r->elem_size = sizeof(int);
    r->zero = int_zero;
    r->one = int_one;
    r->copy = int_copy;

    r->add = int_add;
    r->neg = int_neg;
    r->sub = int_sub;
    r->mul = int_mul;

    r->eq = int_eq;

    r->print = int_print;
    r->read = int_read;

    r->destroy = NULL;

    INT_RING = r;
  }
  return INT_RING;
}

const RingOps* GetComplexRing(void) {
  if (!COMPLEX_RING) {
    RingOps* r = (RingOps*)malloc(sizeof(RingOps));
    if (!r) return NULL;

    r->elem_size = sizeof(Complex);
    r->zero = c_zero;
    r->one = c_one;
    r->copy = c_copy;

    r->add = c_add;
    r->neg = c_neg;
    r->sub = c_sub;
    r->mul = c_mul;

    r->eq = c_eq;

    r->print = c_print;
    r->read = c_read;

    r->destroy = NULL;

    COMPLEX_RING = r;
  }
  return COMPLEX_RING;
}