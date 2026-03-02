#ifndef PL0_RUNTIME_H
#define PL0_RUNTIME_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <setjmp.h>


#define ERROR_LOG_PATH "lexeme_table.txt"


static jmp_buf _env;
static char _err_msg[512];
static char _var_context[64];


static void _throw_error(const char *msg, const char *var_name) {
    snprintf(_err_msg, sizeof(_err_msg), "%s", msg);
    if (var_name) snprintf(_var_context, sizeof(_var_context), "%s", var_name);
    else _var_context[0] = '\0';
    longjmp(_env, 1);
}

static int32_t _check_zero(int32_t val) {
    if (val == 0) {
        _throw_error("Division by zero", NULL);
    }
    return val;
}



static void _safe_input(int32_t *var, const char *var_name) {
    char _tmp_buf[64];
    char *_endptr;
    long long _val;
    errno = 0;
    
    if (scanf("%63s", _tmp_buf) != 1) {
        if (!feof(stdin)) _throw_error("Input failed (scanf error)", var_name);
        _tmp_buf[0] = '\0';
    }
    
    _val = strtoll(_tmp_buf, &_endptr, 10);
    
    if ((_tmp_buf == _endptr) || (*_endptr != '\0')) {
        _throw_error("Invalid input format (not a number)", var_name);
    }
    
    if ((errno == ERANGE) || (_val < -2147483648LL) || (_val > 2147483647LL)) {
        _throw_error("Number out of int32 range", var_name);
    }
    
    *var = (int32_t)_val;
}


static int _handle_runtime_error() {
    FILE *fp = fopen(ERROR_LOG_PATH, "a");
    if (fp) {
        fprintf(fp, "Runtime Error: %s", _err_msg);
        if (_var_context[0] != '\0') fprintf(fp, " (Variable: %s)", _var_context);
        if (errno != 0) fprintf(fp, ". System info: %s", strerror(errno));
        fprintf(fp, "\n");
        fclose(fp);
    }
    fprintf(stderr, "Runtime Error: %s\nCheck lexeme_table.txt\n", _err_msg);
    return 1;
}

 int K = 0x56987018;


#endif
