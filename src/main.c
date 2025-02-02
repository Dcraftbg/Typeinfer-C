#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "darray.h"
typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT32,
    TYPE_INT16
} Type;
typedef struct SymTabNode SymTabNode;
typedef struct Symbol Symbol;
typedef struct AST AST;
typedef struct {
    AST **items;
    size_t len, cap;
} ASTs;
struct Symbol {
    const char* name;
    Type type;
    ASTs infer_asts;
};
struct SymTabNode {
    SymTabNode* parent;
    struct {
        Symbol** items;
        size_t len, cap;
    } symbols;
};
typedef struct AST AST;
struct AST {
    AST* parent;
    enum {
        AST_INT,
        AST_SYM,
        AST_BINOP
    } kind;
    Type type;
    union {
        struct {
            const char* name;
            Symbol* symbol;
        } sym;
        int integer;
        struct {
            int op;
            AST *lhs, *rhs;
        } binop;
    } as;
};
Symbol* sym_new(void) {
    Symbol* me = malloc(sizeof(*me));
    assert(me && "Ran out of memory");
    memset(me, 0, sizeof(*me));
    return me;
}
void stl_insert(SymTabNode* node, const char* name, Symbol* symbol) {
    symbol->name = name;
    da_push(&node->symbols, symbol);
}
Symbol* stl_lookup(SymTabNode* node, const char* name) {
    while(node) {
        for(size_t i = 0; i < node->symbols.len; ++i) {
            Symbol* s = node->symbols.items[i];
            if(strcmp(s->name, name) == 0) return s;
        }
        node = node->parent;
    }
    return NULL;
}
AST* ast_new(void) {
    AST* me = malloc(sizeof(*me));
    assert(me && "Ran out of memory");
    memset(me, 0, sizeof(*me));
    return me;
}
AST* ast_sym(SymTabNode* node, const char* sym) {
    AST* me = ast_new();
    me->kind = AST_SYM;
    me->as.sym.name = sym;
    me->as.sym.symbol = stl_lookup(node, sym);
    return me;
}
AST* ast_int(Type type, int value) {
    AST* me = ast_new();
    me->kind = AST_INT;
    me->as.integer = value;
    me->type = type;
    return me;
}
AST* ast_binop_new(int op, AST* lhs, AST* rhs) {
    AST* me = ast_new();
    lhs->parent = me;
    rhs->parent = me;
    me->kind = AST_BINOP;
    me->as.binop.op = op;
    me->as.binop.lhs = lhs;
    me->as.binop.rhs = rhs;
    return me;
}
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eprintfln(...) (eprintf(__VA_ARGS__), fputs("\n", stderr))
void type_dump(Type type) {
    switch(type) {
    case TYPE_UNKNOWN:
        printf("<unknown>");
        break;
    case TYPE_INT16:
        printf("int16");
        break;
    case TYPE_INT32:
        printf("int32");
        break;
    default:
        printf("(type unknown %d)", type);
        break;
    }
}
void ast_dump(AST* ast) {
    switch(ast->kind) {
    case AST_SYM:
        printf("%s", ast->as.sym.name);
        break;
    case AST_INT:
        printf("int(%d)", ast->as.integer);
        break;
    case AST_BINOP:
        printf("(");
        ast_dump(ast->as.binop.lhs);
        printf(" %c ", ast->as.binop.op);
        ast_dump(ast->as.binop.rhs);
        printf(")");
        break;
    default:
        printf("(ast_dump unknown kind %d)", ast->kind);
        break;
    }
    if(ast->type) {
        printf("[");
        type_dump(ast->type);
        printf("]");
    }
}

void infer_down_ast(AST* ast, Type type);
void infer_up_ast(AST* ast, Type type) {
    while(ast) {
        if(ast->type) break;
        switch(ast->kind) {
        case AST_BINOP:
            infer_down_ast(ast->as.binop.lhs, type);
            infer_down_ast(ast->as.binop.rhs, type);
            break;
        default:
            break;
        }
        ast->type = type;
        ast = ast->parent;
    }
}
void infer_down_ast(AST* ast, Type type) {
    if(ast->type) return;
    ast->type = type;
    switch(ast->kind) {
    case AST_SYM: {
        Symbol* s = ast->as.sym.symbol;
        s->type = type;
        for(size_t i = 0; i < s->infer_asts.len; ++i) {
            s->infer_asts.items[i]->type = type;
            infer_up_ast(s->infer_asts.items[i]->parent, type);
        }
        free(s->infer_asts.items);
        memset(&s->infer_asts, 0, sizeof(s->infer_asts));
    } break;
    case AST_BINOP: {
        infer_down_ast(ast->as.binop.lhs, type);
        infer_down_ast(ast->as.binop.rhs, type);
    } break;
    default:
        break;
    }
}
bool try_infer_ast(AST* ast) {
    if(ast->type) return true;
    switch(ast->kind) {
    case AST_SYM: {
        Symbol* s = ast->as.sym.symbol;
        assert(s);
        if(s->type) {
            ast->type = s->type;
            return true;
        }
        da_push(&s->infer_asts, ast);
    } break;
    case AST_BINOP: {
        Type type = TYPE_UNKNOWN;
        if(try_infer_ast(ast->as.binop.lhs)) {
            type = ast->as.binop.lhs->type;
            infer_down_ast(ast->as.binop.rhs, type);
        }
        else if(try_infer_ast(ast->as.binop.rhs)) {
            type = ast->as.binop.rhs->type;
            infer_down_ast(ast->as.binop.lhs, type);
        }
        if(type) {
            ast->type = type;
            return true;
        }
    } break;
    default:
    }
    return false;
}
int main(void) {
    SymTabNode symroot = {0};
    SymTabNode* symnode = &symroot;
    Symbol* a = sym_new();
    stl_insert(symnode, "a", a);
    AST* expr = 
        ast_binop_new(
            '+',
            ast_int(TYPE_UNKNOWN, 3),
            ast_binop_new(
                '*',
                ast_int(TYPE_UNKNOWN, 4),
                ast_sym(symnode, "a") 
            )
        );
    AST* expr2 = 
        ast_binop_new(
            '+',
            ast_int(TYPE_INT32, 2),
            ast_binop_new(
                '*',
                ast_int(TYPE_UNKNOWN, 3),
                ast_sym(symnode, "a") 
            )
        );
    printf("expr1: "); ast_dump(
        expr
    ); printf("\n");
    printf("expr2: "); ast_dump(
        expr2
    ); printf("\n");
    printf("a : "); type_dump(a->type); printf(";\n");
    printf("-- Inferring types --\n");
    try_infer_ast(expr);
    try_infer_ast(expr2);
    printf("expr1: "); ast_dump(
        expr
    ); printf("\n");
    printf("expr2: "); ast_dump(
        expr2
    ); printf("\n");
    printf("a : "); type_dump(a->type); printf(";\n");
}
