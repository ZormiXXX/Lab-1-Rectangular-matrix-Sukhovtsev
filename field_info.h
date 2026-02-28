#ifndef FIELD_INFO_H
#define FIELD_INFO_H

#include <stddef.h>
#include <stdio.h>

typedef struct FieldInfo {
    size_t elem_size;

    void (*set_zero)(void* out);                
    void (*copy)(void* dst, const void* src);   

    int  (*add)(void* out, const void* a, const void* b);
    int  (*mul)(void* out, const void* a, const void* b);

    void (*print)(const void* x, FILE* out);
    int  (*read)(void* x, FILE* in);             

    void (*destroy)(void* x);                    
} FieldInfo;

typedef struct Complex {
    double re;
    double im;
} Complex;

const FieldInfo* GetIntFieldInfo(void);
const FieldInfo* GetComplexFieldInfo(void);

#endif