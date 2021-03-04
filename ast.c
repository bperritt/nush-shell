#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
// Credit to ferd @ github.ccs.neu.edu/fvesely for the ast implementation
ast *make_ast_value(char** value, long size) {
    ast *new = calloc(1, sizeof(ast));
    new->value = malloc((size + 1) * sizeof(char*));
    for (int i = 0; i < size; i++) {
	    new->value[i] = strdup(value[i]);
    }	 
    new->valSize = size;
    new->value[size] = 0;   
    new->op = "=";

    return new;
}

ast *make_ast_op(char* op, ast *left, ast *right) {
    ast *new = calloc(1, sizeof(ast));
    new->op = strdup(op);
    new->valSize = 0;
    new->left = left;
    new->right = right;
    
    return new;
}

int is_value(ast *a) {
    if (a == NULL)
        return 0;

    return strcmp(a->op, "=") == 0;
}
int is_operator(ast *a) {
    if (a == NULL)
        return 0;

    return strcmp(a->op, "=") != 0;
}

void free_ast(ast *a) {
    if (a == NULL)
        return;
    if (strcmp(a->op, "=") == 0) {
	for(int i = 0; i < a->valSize; i++) {
		free(a->value[i]);
	}
	free(a->value);
    } else {
	free(a->op);
    }		
    free_ast(a->left);
    free_ast(a->right);
    free(a);
}

void print_ast_aux(ast *a) {
    if (a == NULL)
        return;

    if (is_value(a)) {
	char** phrase = a->value;   
        int i = 0;	
	while(phrase[i] != 0) {
		printf("%s ", phrase[i]);
		i++;
	}	
        return;
    }

    // ortherwise, we have an op
    printf("(%s ", a->op);
    print_ast_aux(a->left);
    putchar(' ');
    print_ast_aux(a->right);
    printf(")");
}

void print_ast(ast *a) {
    print_ast_aux(a);
    putchar('\n');
}
