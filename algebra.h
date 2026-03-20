#ifndef ALGEBRA_H
#define ALGEBRA_H

#include <stddef.h>
#include <stdio.h>

typedef struct Complex {
  double re;
  double im;
} Complex;

typedef struct RingOps {
  size_t elem_size;

  void (*zero)(void* out);
  void (*one)(void* out);
  void (*copy)(void* dst, const void* src);

  int (*add)(void* out, const void* a, const void* b);
  int (*neg)(void* out, const void* a);
  int (*sub)(void* out, const void* a, const void* b);
  int (*mul)(void* out, const void* a, const void* b);

  int (*eq)(const void* a, const void* b);

  void (*print)(const void* x, FILE* out);
  int (*read)(void* x, FILE* in);

  void (*destroy)(void* x);
} RingOps;

const RingOps* GetIntRing(void);
const RingOps* GetComplexRing(void);

#endif