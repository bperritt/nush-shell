#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "list.h"
#include "ast.h"

// Credit for tokenize logic goes to Nat Tuck @ GitHub.com/NatTuck
ast* tokenize(char* text);
char* read_token(const char* text, int ii);
bool is_op(char c);

int is_valid_op(char* s) {
	if ( strcmp(s, ";") == 0) {
		return 4;
	}	
	if ( strcmp(s, "&") == 0) {
		return 4;
	}	
	if ( strcmp(s, "&&") == 0) {
		return 3;
	}	
	if ( strcmp(s, "||") == 0) {
		return 3;
	}	
	if ( strcmp(s, "|") == 0) {
		return 2;
	}	
	if ( strcmp(s, "<") == 0) {
		return 1;
	}	
	if ( strcmp(s, ">") == 0) {
		return 1;
	}
    if ( strcmp(s, "(") == 0) {
		return -5;
	}
    if ( strcmp(s, ")") == 0) {
		return 5;
	}
	return 0;
}

ast* create_ast(char* tokens[], long size) {
	if(size == 0) {
		return 0;
	}	
	char* left[size];
	char* right[size];
    int inparens = 0;
	long op_index = -1;
	int curr = 0;
    	for(int i = size - 1; i >= 0; i--) {
		int prec = is_valid_op(tokens[i]);
        if (prec == -5 || prec == 5) {
            if (inparens == 0 && prec == -5) {
                puts("Invalid parentheses 2");
                exit(1);
            }
            inparens += prec;
        }
		if (prec > curr && inparens == 0) {
			curr = prec;
			op_index = i;
		}	
	}
	if (op_index != -1) {
		for(int c = 0; c < op_index; c++) {
			left[c] = tokens[c];
		}
		left[op_index] = 0;
		for(int c = op_index + 1; c < size; c++) {
			right[c - op_index - 1] = tokens[c];
		}
		right[size - op_index - 1] = 0;	
		return make_ast_op(tokens[op_index], create_ast(left, op_index), create_ast(right, size - op_index - 1));
	}
    if (is_valid_op(tokens[0]) == -5 && is_valid_op(tokens[size-1]) == 5) {
        if (size <= 2) {
            puts("Invalid parentheses 1");
            exit(1);
        } 
        return make_ast_op("()", create_ast(&tokens[1], size-2), 0);
    }
	return make_ast_value(tokens, size);	
}

ast* tokenize(char* text) {
    list* xs = 0;

    int nn = strlen(text);
    int ii = 0;
    while (ii < nn) {
	    if (!isspace(text[ii]) && text[ii] != '\n') {
        	 	char* token = read_token(text, ii);
	     	xs = cons(token, xs);
	     	ii += strlen(token);
		    free(token);
	    } else {				
            	ii++;
	    }
    }
    xs = rev_free(xs);
    char* tokens[length(xs) + 1];
    list* temp = xs;
    for(int i = 0; i < length(xs); i++) {
	tokens[i] = temp->head;
	temp = temp->tail;
    }
    tokens[length(xs)] = 0;
    long l = length(xs);
    ast* tree = create_ast(tokens, l);	
    free_list(xs);
    return tree;	    
}

bool is_duplicate_op(char c) {
	return c == '|' || c == '&';
}

char* read_token(const char* text, int ii) {
	int nn = 0;
    int inquotes = 0;
	if (is_op(text[ii])) {    
        nn++;   
		if(text[ii] == '&' && text[ii+1] == '&') {
			nn++;
		} 
        else if(text[ii] == '|' && text[ii+1] == '|') {
			nn++;
		}       
	} else {
		while(inquotes || (!is_op(text[ii+nn]) && !isspace(text[ii+nn]) && (text[ii + nn] != '$' || nn == 0))) {
            if (text[ii+nn] == '"') {
                if (inquotes) {
                    inquotes = 0;
                } else {
                    inquotes = 1;
                }
            }    
			nn++;
		}
	}
	char* token = malloc(nn + 1);
	memcpy(token, text + ii, nn);
	token[nn] = 0;
	return token;
}

bool is_op(char c) {
	return c == '<' || c == '>' || c == '|' || c == '&' || c == ';' || c == ')' || c == '(';
}
