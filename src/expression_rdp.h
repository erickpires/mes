// Expressions Recursive Descent Parser
// Copyright Erick Pires - 2018

#ifndef EXPRESSION_RDP_H
#define EXPRESSION_RDP_H 1

#include <stdint.h>
#include <stdbool.h>
typedef intptr_t isize;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

typedef size_t       usize;
typedef uint8_t      uint8;
typedef uint16_t     uint16;
typedef uint32_t     uint32;
typedef uint64_t     uint64;
typedef unsigned int uint;


typedef enum {
    RDP_TYPE_OPERATOR,
    RDP_TYPE_OPERAND,
} RdpTokenType;

typedef enum {
    PLUS    = '+',
    MINUS   = '-',
    TIMES   = '*',
    DIVIDE  = '/',
    OPEN_P  = '(',
    CLOSE_P = ')',
} RdpOperator;

typedef enum {
    RDP_NO_ERROR = 0,
    RDP_ERR_EARLY_EOS,
    RDP_ERR_NO_OPEN_PAREN,
    RDP_ERR_NO_CLOSE_PAREN,
} RdpError;

typedef struct {
    RdpTokenType type;
    union {
        uint operand;
        RdpOperator operator;
    };
} RdpToken;

typedef struct {
    RdpToken* expr;
    usize count;
} RdpExpression;

uint parse_expression(RdpExpression* expr, RdpError* err);

#endif
