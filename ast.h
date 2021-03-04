#ifndef AST_H
#define AST_H
// Credit to ferd @ github.ccs.neu.edu/fvesely for the ast implementation
typedef struct ast {
    char* op; // + - * / ... and = stands for value
    char** value;
    long valSize;
    struct ast *left;
    struct ast *right;
} ast;

// Construct an AST value.
ast *make_ast_value(char** value, long size);

// Construct an operator expression
ast *make_ast_op(char* op, ast *left, ast *right);

// Is the given AST a value?
int is_value(ast *a);

// Is the given AST an operator expression?
int is_operator(ast *a);

// Free the given AST
void free_ast(ast *a);

// Print the AST as an s-expression.
void print_ast(ast *a);

#endif // !AST_H
