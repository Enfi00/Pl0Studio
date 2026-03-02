#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h> 

#include "strtonum.h"

#define PL0C_VERSION    "3.8.0-Refactored"
#define OUTPUT_TABLE_FILE "lexeme_table.txt"
#define ERROR_FILE "errors.txt"

/* Типи токенів */
enum TokenType {
    TOK_EOF = 0,
    TOK_UNKNOWN = '?',    
    TOK_IDENT = 'I',
    TOK_NUMBER = 'N',
    TOK_STRING = 'S',     
    TOK_PROGRAM_KW = 'P', 
    TOK_VARIABLE = 'V',   
    TOK_START = 'B',      
    TOK_STOP = 'E',       
    TOK_IF = 'i',
    TOK_THEN = 't',
    TOK_ELSE = 'e',
    TOK_FOR = 'f',
    TOK_DOWNTO = 'd',
    TOK_DO = 'D',
    TOK_INPUT = 'R',      
    TOK_OUTPUT = 'W',     
    
    TOK_ADD = '+',        
    TOK_SUB = '-',        
    TOK_MUL = '*',
    TOK_DIV = '/',
    TOK_MOD = '%',
    
    TOK_EQ = '=',         
    TOK_NE = '!',         
    TOK_GT = '>',         
    TOK_LT = '<',         
    
    TOK_NOT = 'n',        
    TOK_AND = '&',
    TOK_OR = '|',
    
    TOK_ASSIGN = ':',     
    TOK_SEMICOLON = ';',
    TOK_COMMA = ',',
    TOK_HASH = '#' ,
    
    TOK_LPAREN = '(',
    TOK_RPAREN = ')'
};

/* Структури даних */
typedef struct LexemeEntry {
    size_t line;
    int type;
    char *value; 
    struct LexemeEntry *next;
} LexemeEntry;

typedef struct ErrorEntry {
    char *msg;
    int is_fatal;
    struct ErrorEntry *next;
} ErrorEntry;

struct symtab {
    char *name;
    struct symtab *next;
};

/* Глобальні змінні */
static LexemeEntry *lexeme_head = NULL;
static LexemeEntry *lexeme_tail = NULL;
static ErrorEntry *error_head = NULL;
static ErrorEntry *error_tail = NULL;
static struct symtab *sym_head = NULL;

static char *raw;       
static char *token;     
static int type;        
static size_t line = 1;
static int compilation_errors = 0;

static char *code_buf = NULL;
static size_t code_buf_size = 0;
static size_t code_buf_capacity = 0;

/* Прототипи */
static void fatal_error(const char *fmt, ...);
static void syntax_error(const char *fmt, ...);
static void record_error(const char *fmt, int fatal, ...);
static void synchronize(void);
static void expression(void);
static void condition(void);
static void statement(void);
static void next(void);

/* * --- Утиліти --- */

static void add_lexeme(int type, const char *val) {
    LexemeEntry *node = malloc(sizeof(LexemeEntry));
    if (!node) fatal_error("Memory allocation failed (lexeme)");
    
    node->line = line;
    node->type = type;
    node->value = val ? strdup(val) : strdup("----");
    node->next = NULL;

    if (!lexeme_head) {
        lexeme_head = node;
        lexeme_tail = node;
    } else {
        lexeme_tail->next = node;
        lexeme_tail = node;
    }
}

static void record_error(const char *fmt, int fatal, ...) {
    va_list ap;
    char buffer[512];
    
    va_start(ap, fatal);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    ErrorEntry *node = malloc(sizeof(ErrorEntry));
    if (!node) exit(1); 
    
    node->msg = strdup(buffer);
    node->is_fatal = fatal;
    node->next = NULL;

    if (!error_head) {
        error_head = node;
        error_tail = node;
    } else {
        error_tail->next = node;
        error_tail = node;
    }
}

static const char* get_token_name(int type) {
    switch (type) {
        case TOK_UNKNOWN: return "UNKNOWN";
        case TOK_STRING: return "string";
        case TOK_PROGRAM_KW: return "program";
        case TOK_VARIABLE: return "variable";
        case TOK_START: return "start";
        case TOK_STOP: return "stop";
        case TOK_IF: return "if";
        case TOK_THEN: return "then";
        case TOK_ELSE: return "else";
        case TOK_FOR: return "for";
        case TOK_DOWNTO: return "downto";
        case TOK_DO: return "do";
        case TOK_INPUT: return "input";
        case TOK_OUTPUT: return "output";
        case TOK_ADD: return "add";
        case TOK_SUB: return "sub";
        case TOK_MUL: return "*";
        case TOK_DIV: return "/";
        case TOK_MOD: return "%";
        case TOK_EQ: return "eg";
        case TOK_NE: return "ne";
        case TOK_GT: return ">>";
	case TOK_LT: return "<<";
        case TOK_NOT: return "!";
        case TOK_AND: return "&";
        case TOK_OR: return "|";
        case TOK_ASSIGN: return "->";
        case TOK_SEMICOLON: return ";";
        case TOK_COMMA: return ",";
        case TOK_HASH: return "#";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_IDENT: return "identifier";
        case TOK_NUMBER: return "number";
        case TOK_EOF: return "EOF";
        default: return "unknown";
    }
}


static void dump_errors_file(void) {
    FILE *fp = fopen(ERROR_FILE, "w");
    if (!fp) {
       
        fprintf(stderr, "_Ihor: Warning - cannot write to %s\n", ERROR_FILE);
        return;
    }

    ErrorEntry *err = error_head;
    if (!err) {
       
    } else {
        while (err) {
            fprintf(fp, "%s\n", err->msg);
            err = err->next;
        }
    }
    fclose(fp);
}


static void dump_lexeme_table(void) {
    FILE *fp = fopen(OUTPUT_TABLE_FILE, "w");
    if (!fp) {
        fprintf(stderr, "_Ihor: Critical error - cannot write to %s\n", OUTPUT_TABLE_FILE);
        return;
    }

    fprintf(fp, "Line\tType\t\tValue\n");
    fprintf(fp, "----------------------------------------\n");
    
    LexemeEntry *curr = lexeme_head;
    while (curr) {
        const char *type_name = get_token_name(curr->type);
        char tabs[3] = "\t\t";
        if (strlen(type_name) >= 8) strcpy(tabs, "\t");

        char *val_str = curr->value;
        if (strcmp(val_str, "----") == 0) {
             fprintf(fp, "%lu\t%s%s----\n", curr->line, type_name, tabs);
        } else {
             fprintf(fp, "%lu\t%s%s%s\n", curr->line, type_name, tabs, val_str);
        }
        curr = curr->next;
    }

    fprintf(fp, "\n\n--- Errors ---\n");
    ErrorEntry *err = error_head;
    if (!err) fprintf(fp, "No errors.\n");
    while (err) {
        fprintf(fp, "%s\n", err->msg);
        err = err->next;
    }

    fclose(fp);
    fprintf(stderr, "Lexeme table and errors written to %s\n", OUTPUT_TABLE_FILE);
}

static void fatal_error(const char *fmt, ...) {
    va_list ap;
    char buffer[512];
    char final_msg[1024];
    
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    snprintf(final_msg, sizeof(final_msg), "_Ihor: Fatal Error at line %lu: %s", line, buffer);
    fprintf(stderr, "%s\n", final_msg);
    record_error(final_msg, 1);
    
    dump_lexeme_table();
    dump_errors_file(); 
    exit(1);
}

static void syntax_error(const char *fmt, ...) {
    va_list ap;
    char buffer[512];
    char final_msg[1024];
    
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    snprintf(final_msg, sizeof(final_msg), "_Ihor: Error at line %lu: Syntax Error: %s", line, buffer);
    record_error(final_msg, 0);
    compilation_errors = 1;
}

static void synchronize(void) {
    while (type != TOK_SEMICOLON && type != TOK_STOP && type != TOK_EOF) {
        next();
    }
    if (type == TOK_SEMICOLON) next(); 
}

static void readin(char *filename) {
    int fd;
    struct stat st;

    size_t len = strlen(filename);
    if (len < 4 || strcmp(filename + len - 4, ".O13") != 0) {
        fatal_error("Input file must have .O13 extension");
    }
    if ((fd = open(filename, O_RDONLY)) == -1) fatal_error("couldn't open %s", filename);
    if (fstat(fd, &st) == -1) fatal_error("couldn't get size");
    if ((raw = malloc(st.st_size + 1)) == NULL) fatal_error("malloc failed");
    if (read(fd, raw, st.st_size) != st.st_size) fatal_error("couldn't read %s", filename);
    raw[st.st_size] = '\0';
    close(fd);
}

static void skip_comment(void) {
    raw++; 
    while (*raw != '\0') {
        if (*raw == '$' && *(raw + 1) == '/') {
            raw += 2; return;
        }
        if (*raw == '\n') ++line;
        raw++;
    }
    record_error("Ihor_Lexical Error: Unterminated comment", 0);
}

static int is_stop_char(char c) {
    if (isspace(c)) return 1;
    if (c == ';' || c == ',' || c == '\0' || c == '"' || c == '(' || c == ')') return 1;
    return 0;
}

static int ident(void) {
    char *p = raw;
    size_t len;
    int is_valid_low4 = 1;
    int has_bad_chars = 0;
    
    while (!is_stop_char(*raw)) {
        if (!isalnum(*raw) && *raw != '_') has_bad_chars = 1;
        ++raw;
    }
    len = raw - p;

    free(token);
    token = malloc(len + 1);
    if (!token) fatal_error("malloc failed");
    strncpy(token, p, len);
    token[len] = '\0';

    if (!strcmp(token, "add")) { add_lexeme(TOK_ADD, "----"); return TOK_ADD; }
    if (!strcmp(token, "sub")) { add_lexeme(TOK_SUB, "----"); return TOK_SUB; }
    if (!strcmp(token, "eg"))  { add_lexeme(TOK_EQ, "----"); return TOK_EQ; }
    if (!strcmp(token, "ne"))  { add_lexeme(TOK_NE, "----"); return TOK_NE; }
    
    int ret_type = TOK_IDENT;
    if (!strcmp(token, "variable")) ret_type = TOK_VARIABLE;
    else if (!strcmp(token, "program")) ret_type = TOK_PROGRAM_KW;
    else if (!strcmp(token, "start")) ret_type = TOK_START;
    else if (!strcmp(token, "stop")) ret_type = TOK_STOP;
    else if (!strcmp(token, "if")) ret_type = TOK_IF;
    else if (!strcmp(token, "then")) ret_type = TOK_THEN;
    else if (!strcmp(token, "else")) ret_type = TOK_ELSE;
    else if (!strcmp(token, "for")) ret_type = TOK_FOR;
    else if (!strcmp(token, "downto")) ret_type = TOK_DOWNTO;
    else if (!strcmp(token, "do")) ret_type = TOK_DO;
    else if (!strcmp(token, "input")) ret_type = TOK_INPUT;
    else if (!strcmp(token, "output")) ret_type = TOK_OUTPUT;

    if (ret_type == TOK_IDENT) {
        if (has_bad_chars) {
             record_error("Ihor_Lexical Error: Invalid character in identifier '%s'", 0, token);
             add_lexeme(TOK_UNKNOWN, token);
             return TOK_UNKNOWN;
        }
        if (len > 4) is_valid_low4 = 0;
        for (size_t i = 0; i < len; i++) {
            if (!islower(token[i])) is_valid_low4 = 0;
        }
        if (!is_valid_low4) {
            record_error("Ihor_Lexical Error: Identifier '%s' violates Low4", 0, token);
            add_lexeme(TOK_UNKNOWN, token);
            return TOK_UNKNOWN;
        }
        add_lexeme(TOK_IDENT, token);
    } else {
        add_lexeme(ret_type, "----");
    }
    return ret_type;
}

static int number(void) {
    char *p = raw;
    size_t len;
    int has_bad_chars = 0;
    const char *errstr;

    while (!is_stop_char(*raw)) {
        if (*raw == '-' && raw == p) {
        } else if (!isdigit(*raw)) {
             has_bad_chars = 1;
        }
        ++raw;
    }
    len = raw - p;
    free(token);
    token = malloc(len + 1);
    if (!token) fatal_error("malloc failed");
    strncpy(token, p, len);
    token[len] = '\0';

    if (has_bad_chars) {
        record_error("Ihor_Lexical Error: Invalid number format '%s'", 0, token);
        add_lexeme(TOK_UNKNOWN, token);
        return TOK_UNKNOWN;
    }
    
    (void) strtonum(token, INT_MIN, INT_MAX, &errstr);
    
    if (errstr != NULL) {
        record_error("Ihor_Lexical Error: invalid number value '%s' (%s)", 0, token, errstr);
        add_lexeme(TOK_UNKNOWN, token);
        return TOK_UNKNOWN;
    }
    add_lexeme(TOK_NUMBER, token);
    return TOK_NUMBER;
}

static int string_literal(void) {
    char *p = raw;
    size_t len;
    char *start = p;

    raw++; 
    while (*raw != '"') {
        if (*raw == '\0' || *raw == '\n') {
            record_error("Ihor_Lexical Error: Unterminated string literal", 0);
            add_lexeme(TOK_UNKNOWN, "unterminated string");
            return TOK_UNKNOWN;
        }
        raw++;
    }
    raw++; 
    len = raw - start;
    free(token);
    token = malloc(len + 1);
    if (!token) fatal_error("malloc failed");
    strncpy(token, start, len);
    token[len] = '\0';
    add_lexeme(TOK_STRING, token);
    return TOK_STRING;
}

static int lex(void) {
again:
    while (isspace(*raw)) {
        if (*raw++ == '\n') ++line;
    }
    if (islower(*raw)) return ident();
    if (isalpha(*raw) || *raw == '_') return ident();
    if (isdigit(*raw)) return number();

    switch (*raw) {
    case '"': return string_literal();
    case '#': add_lexeme(TOK_HASH, "----"); raw++; return TOK_HASH;
    case '/':
        if (*(raw + 1) == '$') { 
            raw += 2; 
            while (*raw != '\0') {
                if (*raw == '$' && *(raw + 1) == '/') { raw += 2; goto again; }
                if (*raw == '\n') ++line;
                raw++;
            }
            record_error("Ihor_Lexical Error: Unterminated comment", 0);
            return TOK_EOF;
        }
        add_lexeme(TOK_DIV, "----"); raw++; return TOK_DIV;
    case '-':
        if (*(raw + 1) == '>') { 
            raw += 2; 
            add_lexeme(TOK_ASSIGN, "----"); 
            return TOK_ASSIGN; 
        }
        if (isdigit(*(raw + 1))) {
            return number();
        }
        
        record_error("Ihor_Lexical Error: Unexpected character '-' (use 'sub')", 0);
        add_lexeme(TOK_UNKNOWN, "-"); raw++; return TOK_UNKNOWN;

    case '+':
        record_error("Ihor_Lexical Error: Unexpected character '+' (use 'add')", 0);
        add_lexeme(TOK_UNKNOWN, "+"); raw++; return TOK_UNKNOWN;
    case '*': add_lexeme(TOK_MUL, "----"); raw++; return TOK_MUL;
    case '%': add_lexeme(TOK_MOD, "----"); raw++; return TOK_MOD;
    case '>':
        if (*(raw + 1) == '>') { raw += 2; add_lexeme(TOK_GT, "----"); return TOK_GT; }
        record_error("Ihor_Lexical Error: Unknown '>' (mean '>>'?)", 0);
        add_lexeme(TOK_UNKNOWN, ">"); raw++; return TOK_UNKNOWN;
    case '<':
        if (*(raw + 1) == '<') { raw += 2; add_lexeme(TOK_LT, "----"); return TOK_LT; }
        record_error("Ihor_Lexical Error: Unknown '<' (mean '<<'?)", 0);
        add_lexeme(TOK_UNKNOWN, "<"); raw++; return TOK_UNKNOWN;
    case '!': add_lexeme(TOK_NOT, "----"); raw++; return TOK_NOT;
    case '&': add_lexeme(TOK_AND, "----"); raw++; return TOK_AND;
    case '|': add_lexeme(TOK_OR, "----"); raw++; return TOK_OR;
    case ';': add_lexeme(TOK_SEMICOLON, "----"); raw++; return TOK_SEMICOLON;
    case ',': add_lexeme(TOK_COMMA, "----"); raw++; return TOK_COMMA;
    case '(': add_lexeme(TOK_LPAREN, "("); raw++; return TOK_LPAREN;
    case ')': add_lexeme(TOK_RPAREN, ")"); raw++; return TOK_RPAREN;
    case '\0': return TOK_EOF;
    default:
        {
            char bad[2] = {*raw, 0};
            record_error("Ihor_Lexical Error: Unknown token '%c'", 0, *raw);
            add_lexeme(TOK_UNKNOWN, bad); raw++; return TOK_UNKNOWN;
        }
    }
}

static void next(void) {
    type = lex();
}

static void aout(const char *fmt, ...) {
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (len < 0) return;

    if (code_buf_size + len + 1 > code_buf_capacity) {
        size_t new_cap = code_buf_capacity == 0 ? 1024 : code_buf_capacity * 2;
        while (new_cap < code_buf_size + len + 1) new_cap *= 2;
        
        char *new_buf = realloc(code_buf, new_cap);
        if (!new_buf) fatal_error("Memory allocation failed (code buffer)");
        code_buf = new_buf;
        code_buf_capacity = new_cap;
        
        if (code_buf_size == 0) code_buf[0] = '\0';
    }

    va_start(ap, fmt);
    vsnprintf(code_buf + code_buf_size, len + 1, fmt, ap);
    va_end(ap);

    code_buf_size += len;
}

static void cg_symbol(void) {
    if (type == TOK_UNKNOWN) return; 
    switch (type) {
    case TOK_IDENT: case TOK_NUMBER: aout("%s", token); break;
    case TOK_ADD: aout("+"); break;
    case TOK_SUB: aout("-"); break;
    case TOK_MUL: aout("*"); break;
    case TOK_DIV: aout("/"); break;
    case TOK_MOD: aout("%%"); break;
    case TOK_EQ:  aout("=="); break;
    case TOK_NE:  aout("!="); break;
    case TOK_GT:  aout(">"); break;
    case TOK_LT:  aout("<"); break;
    case TOK_AND: aout("&&"); break;
    case TOK_OR:  aout("||"); break;
    case TOK_NOT: aout("!"); break;
    case TOK_ASSIGN: aout("="); break;
    case TOK_LPAREN: aout("("); break;
    case TOK_RPAREN: aout(")"); break;
    default: break;
    }
}

static void check_sym(void) {
    struct symtab *curr = sym_head;
    int found = 0;
    while (curr) {
        if (!strcmp(token, curr->name)) { found = 1; break; }
        curr = curr->next;
    }
    if (!found) {
        syntax_error("Ihor_Semantic Error: Undefined symbol: %s", token);
    }
}

static void add_sym(void) {
    struct symtab *curr = sym_head;
    while (curr) {
        if (!strcmp(curr->name, token)) {
            syntax_error("Ihor_Semantic Error: Duplicate symbol: %s", token);
            return;
        }
        curr = curr->next;
    }
    struct symtab *node = malloc(sizeof(struct symtab));
    if (!node) fatal_error("malloc failed");
    node->name = strdup(token);
    node->next = sym_head;
    sym_head = node;
}

static void factor(void) {
    if (type == TOK_IDENT) {
        check_sym(); cg_symbol(); next();
    } else if (type == TOK_NUMBER) {
        cg_symbol(); next();
    } else if (type == TOK_NOT) {
        cg_symbol(); next(); aout("("); factor(); aout(")");
    } else if (type == TOK_LPAREN) {
        cg_symbol(); next();
        expression();
        if (type != TOK_RPAREN) {
            syntax_error("Expected ')'");
            synchronize();
            return;
        }
        cg_symbol(); next();
    } else {
        syntax_error("Ihor_Expected identifier, number, '(' or '!'");
        if (type != TOK_SEMICOLON && type != TOK_STOP) next();
    }
}

static void term(void) {
    factor();
    while (type == TOK_MUL || type == TOK_DIV || type == TOK_MOD || type == TOK_AND) {
        int op = type; 
        
        cg_symbol();  
        next(); 
        
    
        if (op == TOK_DIV || op == TOK_MOD) {
            aout("_check_zero(");
            factor();
            aout(")");
        } else {
            
            factor();
        }
    }
}

static void expression(void) {
    term();
    while (type == TOK_ADD || type == TOK_SUB || type == TOK_OR) {
        cg_symbol(); next(); term();
    }
}

static int is_rel_op(int type) {
    return (type == TOK_EQ || type == TOK_NE || type == TOK_GT || type == TOK_LT);
}

static void bool_factor(void);

static void bool_and(void) {
    bool_factor();
    while (type == TOK_AND) {
        aout(" && ");
        next();
        bool_factor();
    }
}

static void bool_or(void) {
    bool_and();
    while (type == TOK_OR) {
        aout(" || ");
        next();
        bool_and();
    }
}

static void bool_factor(void) {
    if (type == TOK_NOT) {
        aout("!");
        next();
        bool_factor();
    } else if (type == TOK_LPAREN) {
        aout("(");
        next();
        condition(); 
        if (type != TOK_RPAREN) {
            syntax_error("Ihor_Expected ')' in logic expression");
            return;
        }
        aout(")");
        next();
    } else {
        expression();
        if (is_rel_op(type)) {
            cg_symbol(); 
            next();
            expression();
        }
    }
}

static void condition(void) {
    bool_or();
}

static void statement(void) {
    char *loop_var;

    switch (type) {
    case TOK_IDENT:
        check_sym();
        cg_symbol();
        next();
        if (type != TOK_ASSIGN) {
            syntax_error("Ihor_Expected '->' assignment");
            synchronize(); 
            return;
        }
        cg_symbol(); 
        next();
        expression();
        
        if (type != TOK_SEMICOLON) {
            syntax_error("Ihor_Expected ';' after assignment");
        } else {
            next();
        }
        aout(";\n");
        break;

    case TOK_START: 
        aout("\n{\n");
        next();
        
        while (type != TOK_STOP && type != TOK_EOF) {
            statement();
        }
        
        if (type != TOK_STOP) {
            syntax_error("Ihor_Expected 'stop' to close block");
            synchronize();
            return;
        }
        aout("\n}\n");
        next();
        break;

    case TOK_IF:
        aout("if (");
        next();
        condition();
        aout(")\n{\n");
        if (type != TOK_THEN) {
            syntax_error("Ihor_Expected 'then'");
            synchronize();
            return;
        }
        next();
        statement();
        aout("\n}\n");
        if (type == TOK_ELSE) {
            aout("else\n{\n");
            next();
            statement();
            aout("\n}\n");
        }
        break;

    case TOK_FOR:
        next();
        if (type != TOK_IDENT) {
            syntax_error("Ihor_Expected identifier in for loop");
            synchronize(); return;
        }
        check_sym();
        loop_var = strdup(token);
        aout("for (%s", loop_var);
        next();
        
        if (type != TOK_ASSIGN) {
            syntax_error("Ihor_Expected '->' in for loop init");
            synchronize(); return;
        }
        aout(" = ");
        next();
        expression();
        
        aout("; %s >= ", loop_var);
        
        if (type != TOK_DOWNTO) {
            syntax_error("Ihor_Expected 'downto'");
            synchronize(); return;
        }
        next();
        expression();
        
        if (type != TOK_DO) {
            syntax_error("Ihor_Expected 'do' after range in for loop");
            synchronize(); return;
        }
        next(); 

        aout("; %s--) \n", loop_var);
        statement();
        aout("\n");
        free(loop_var);
        break;

  
    case TOK_INPUT:
        next();
        if (type != TOK_IDENT) {
            syntax_error("Ihor_Expected identifier after input");
            synchronize(); return;
        }
        check_sym();
        
       
        aout("_safe_input(&%s, \"%s\");\n", token, token);
        
        next();
        if (type != TOK_SEMICOLON) {
            syntax_error("Ihor_Expected ';' after input");
        } else {
            next();
        }
        break;
  

    case TOK_OUTPUT:
        next();
        if (type == TOK_STRING) {
            aout("printf(\"%%s\\n\", %s);\n", token);
            next();
        } else {
            aout("printf(\"%%d\\n\", (int32_t)(");
            expression();
            aout("));\n");
        }
        if (type != TOK_SEMICOLON) {
            syntax_error("Ihor_Expected ';' after output");
        } else {
            next();
        }
        break;

    case TOK_SEMICOLON:
        next();
        break;

    case TOK_STOP:
    case TOK_EOF:
        break;
        
    default:
        if (type == TOK_UNKNOWN) {
             syntax_error("Ihor_Encountered unknown or invalid token");
        } else {
             syntax_error("Ihor_Unknown statement start or unexpected token '%s'", get_token_name(type));
        }
        synchronize(); 
        break;
    }
}

static void parse(void) {
   
   aout("#include \"pl0_runtime.h\"\n\n");
    

    aout("int main(void)\n{\n");

 
    aout("    if (setjmp(_env) == 0) {\n");

    
    
    next(); 
    
    if (type != TOK_HASH) { 
        fatal_error("Ihor_Structure Error: Program must start with '#' (Header missing)"); 
    }
    next(); 
    
    if (type != TOK_PROGRAM_KW) { 
        fatal_error("Ihor_Structure Error: Expected keyword 'program' after '#'"); 
    }
    next();
    
    if (type != TOK_IDENT) { 
        fatal_error("Ihor_Structure Error: Expected program name identifier"); 
    } else { 
        aout("    /* Program: %s */\n", token); 
        next(); 
    }
    
    if (type != TOK_SEMICOLON) { 
        fatal_error("Ihor_Structure Error: Expected ';' after program name"); 
    } 
    next();

    if (type == TOK_VARIABLE) {
        next();
        if (type == TOK_UNKNOWN) { 
            fatal_error("Ihor_Lexical Error in variable declaration"); 
        }
        else if (type != TOK_IDENT) { 
            fatal_error("Ihor_Syntax Error: Expected identifier after 'variable'"); 
        }
        else {
            add_sym();
            aout("    int32_t %s", token);
            next();
            while (type == TOK_COMMA) {
                aout(", ");
                next();
                if (type != TOK_IDENT) { 
                    fatal_error("Ihor_Syntax Error: Expected identifier after ','"); 
                }
                add_sym();
                aout("%s", token);
                next();
            }
            aout(";\n");
            
            if (type != TOK_SEMICOLON) { 
                fatal_error("Ihor_Syntax Error: Expected ';' after variable declarations"); 
            }
            else next();
        }
    } 
    else if (type != TOK_START) {
        fatal_error("Ihor_Structure Error: Expected 'variable' declaration block or 'start' keyword. Found: '%s'", get_token_name(type));
    }

    if (type != TOK_START) { 
        fatal_error("Ihor_Structure Error: Expected 'start' to begin program body"); 
    }
    
    statement();

    if (type != TOK_EOF) {
        fatal_error("Ihor_Structure Error: Unexpected tokens after program end (expected EOF)");
    }

  
    aout("\n        return 0;\n");
    aout("    }\n"); 
  


  
    aout("    else {\n");
    aout("        return _handle_runtime_error();\n");
    aout("    }\n");
    
    aout("}\n");
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file.O13\n", argv[0]);
        return 1;
    }

    readin(argv[1]);
    parse();
    
    dump_lexeme_table(); 
    dump_errors_file();  

    if (compilation_errors) {
        fprintf(stderr, "\n=== Compilation FAILED with %d errors. Lexeme table written. ===\n", compilation_errors);
        return 1;
    }

    if (code_buf) {
        fprintf(stdout, "%s", code_buf);
        free(code_buf);
    }

    return 0;
}
