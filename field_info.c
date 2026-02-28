#include "field_info.h"
#include <stdlib.h>


static void int_set_zero(void* out) {
    *(int*)out = 0;
}

static void int_copy(void* dst, const void* src) {
    *(int*)dst = *(const int*)src;
}

static int int_add(void* out, const void* a, const void* b) {
    *(int*)out = *(const int*)a + *(const int*)b;
    return 0;
}

static int int_mul(void* out, const void* a, const void* b) {
    *(int*)out = (*(const int*)a) * (*(const int*)b);
    return 0;
}

static void int_print(const void* x, FILE* out) {
    fprintf(out, "%d", *(const int*)x);
}

static int int_read(void* x, FILE* in) {
    return (fscanf(in, "%d", (int*)x) == 1) ? 0 : 1;
}


static void complex_set_zero(void* out) {
    ((Complex*)out)->re = 0.0;
    ((Complex*)out)->im = 0.0;
}

static void complex_copy(void* dst, const void* src) {
    *(Complex*)dst = *(const Complex*)src;
}

static int complex_add(void* out, const void* a, const void* b) {
    const Complex* x = (const Complex*)a;
    const Complex* y = (const Complex*)b;
    Complex* r = (Complex*)out;
    r->re = x->re + y->re;
    r->im = x->im + y->im;
    return 0;
}

static int complex_mul(void* out, const void* a, const void* b) {
    const Complex* x = (const Complex*)a;
    const Complex* y = (const Complex*)b;
    Complex* r = (Complex*)out;
    double re = x->re * y->re - x->im * y->im;
    double im = x->re * y->im + x->im * y->re;
    r->re = re;
    r->im = im;
    return 0;
}

static void complex_print(const void* x, FILE* out) {
    const Complex* c = (const Complex*)x;
    if (c->im >= 0) fprintf(out, "%.6g+%.6gi", c->re, c->im);
    else            fprintf(out, "%.6g%.6gi",  c->re, c->im);
}

static int complex_read(void* x, FILE* in) {
    Complex* c = (Complex*)x;
    return (fscanf(in, "%lf %lf", &c->re, &c->im) == 2) ? 0 : 1;
}


static const FieldInfo* INT_FIELD_INFO = NULL;
static const FieldInfo* COMPLEX_FIELD_INFO = NULL;

const FieldInfo* GetIntFieldInfo(void) {
    if (INT_FIELD_INFO == NULL) {
        FieldInfo* fi = (FieldInfo*)malloc(sizeof(FieldInfo));
        if (!fi) return NULL;

        fi->elem_size = sizeof(int);
        fi->set_zero  = int_set_zero;
        fi->copy      = int_copy;
        fi->add       = int_add;
        fi->mul       = int_mul;
        fi->print     = int_print;
        fi->read      = int_read;
        fi->destroy   = NULL;

        INT_FIELD_INFO = fi;
    }
    return INT_FIELD_INFO;
}

const FieldInfo* GetComplexFieldInfo(void) {
    if (COMPLEX_FIELD_INFO == NULL) {
        FieldInfo* fi = (FieldInfo*)malloc(sizeof(FieldInfo));
        if (!fi) return NULL;

        fi->elem_size = sizeof(Complex);
        fi->set_zero  = complex_set_zero;
        fi->copy      = complex_copy;
        fi->add       = complex_add;
        fi->mul       = complex_mul;
        fi->print     = complex_print;
        fi->read      = complex_read;
        fi->destroy   = NULL;

        COMPLEX_FIELD_INFO = fi;
    }
    return COMPLEX_FIELD_INFO;
}