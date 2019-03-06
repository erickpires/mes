#include <stdio.h>
#include <stdlib.h>

#include "expression_rdp.h"

// Grammar
// E => E + T | E - T | T
// T => T * R | T / R | R
// R => ( E ) | num
//
// Removing left recursion
// E  => TE'
// E' => + TE' | - TE' | empty
// T  => RT'
// T' => * RT' | / RT' | empty
// R  => ( E ) | num

uint parse_E(RdpExpression* expr);

static RdpError error = RDP_NO_ERROR;

uint parse_R(RdpExpression* expr) {
    if(expr->count == 0) {
        error = RDP_ERR_EARLY_EOS;
        return 0;
    }

    if(expr->expr[0].type == RDP_TYPE_OPERAND) {
        uint result = expr->expr[0].operand;

        expr->expr++;
        expr->count--;
        return result;
    } else {
        RdpOperator optor = expr->expr[0].operator;
        if(optor != OPEN_P) {
            error = RDP_ERR_NO_OPEN_PAREN;
            return 0;
        }

        expr->expr++;
        expr->count--;

        uint result = parse_E(expr);

        optor = expr->expr[0].operator;
        if(optor != CLOSE_P) {
            error = RDP_ERR_NO_CLOSE_PAREN;
            return 0;
        }

        expr->expr++;
        expr->count--;
        return result;
    }
}

uint parse_T_prime(RdpExpression* expr, uint left_op) {
    if(expr->count == 0) { return left_op; }

    if(expr->expr[0].type == RDP_TYPE_OPERATOR) {
        RdpOperator optor = expr->expr[0].operator;
        if(optor != TIMES && optor != DIVIDE) { // empty
            return left_op;
        }

        expr->expr++;
        expr->count--;


        uint right_op = parse_R(expr);
        uint result = left_op;

        if(optor == TIMES) {
            result *= right_op;
        }

        else if(optor == DIVIDE) {
            result /= right_op;
        }

        return parse_T_prime(expr, result);
    } else { // empty
        return left_op;
    }
}

uint parse_T(RdpExpression* expr) {
    uint r = parse_R(expr);
    return parse_T_prime(expr, r);
}

uint parse_E_prime(RdpExpression* expr, uint left_op) {
    if(expr->count == 0) { return left_op; }

    if(expr->expr[0].type == RDP_TYPE_OPERATOR) {
        RdpOperator optor = expr->expr[0].operator;
        if(optor != PLUS && optor != MINUS) { // empty
            return left_op;
        }

        expr->expr++;
        expr->count--;

        uint right_op = parse_T(expr);
        uint result = left_op;

        if(optor == PLUS) {
            result += right_op;
        }

        else if(optor == MINUS) {
            result -= right_op;
        }

        return parse_E_prime(expr, result);
    } else { // empty
        return left_op;
    }
}

uint parse_E(RdpExpression* expr) {
    uint t = parse_T(expr);
    return parse_E_prime(expr, t);
}

uint parse_expression(RdpToken* expression, usize expression_size, RdpError* err) {
    RdpExpression expr = {expression, expression_size};
    *err = error;
    return parse_E(&expr);
}


#if TEST_RDP
#define sizeof_array(a) (sizeof(a)/sizeof(a[0]))
int main(int args_count, char** args_values) {

    RdpToken plus = {
        .type = RDP_TYPE_OPERATOR,
        .operator = PLUS
    };
    RdpToken times = {
        .type = RDP_TYPE_OPERATOR,
        .operator = TIMES
    };

    RdpToken five = {
        .type = RDP_TYPE_OPERAND,
        .operand = 5
    };
    RdpToken six = {
        .type = RDP_TYPE_OPERAND,
        .operand = 6
    };
    RdpToken seven = {
        .type = RDP_TYPE_OPERAND,
        .operand = 7
    };

    RdpToken open_p = {
        .type = RDP_TYPE_OPERATOR,
        .operator = OPEN_P
    };
    RdpToken close_p = {
        .type = RDP_TYPE_OPERATOR,
        .operator = CLOSE_P
    };


    RdpToken _1 [] = {five, plus, six, plus, seven};
    RdpToken _2 [] = {five, times, six, plus, seven};
    RdpToken _3 [] = {five, plus, six, times, seven};
    RdpToken _4 [] = {five, times, six, times, seven};
    RdpToken _5 [] = {open_p, five, plus, six, close_p, times, seven};
    RdpToken _6 [] = {five, times, open_p, six, plus, seven, close_p};


    RdpError dummy;
    uint eighteen = parse_expression(_1, sizeof_array(_1), &dummy);
    uint thirty_seven = parse_expression(_2, sizeof_array(_2), &dummy);
    uint fourty_seven = parse_expression(_3, sizeof_array(_3), &dummy);
    uint two_ten = parse_expression(_4, sizeof_array(_4), &dummy);
    uint seventy_seven = parse_expression(_5, sizeof_array(_5), &dummy);
    uint sixty_five = parse_expression(_6, sizeof_array(_6), &dummy);

    printf("18 == %d\n", eighteen);
    printf("37 == %d\n", thirty_seven);
    printf("47 == %d\n", fourty_seven);
    printf("210 == %d\n", two_ten);
    printf("77 == %d\n", seventy_seven);
    printf("65 == %d\n", sixty_five);

    return 0;
}

#endif
