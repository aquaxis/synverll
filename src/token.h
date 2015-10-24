/*
 * Copyright (C)2005-2015 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * For further information please contact.
 *  URI:    http://www.aquaxis.com/
 *  E-Mail: info(at)aquaxis.com
 */
#ifndef _H_TOKEN_
#define _H_TOKEN_
//
typedef struct token_order{
    struct token_order  *next_ptr;
    unsigned int        num;
    char                *str;
} TOKEN_ORDER;

TOKEN_ORDER * get_token_order_top();
TOKEN_ORDER * get_token_order_current();
TOKEN_ORDER * get_token_order_prev();

// token.c

int clean_token_order();
int create_token_order();
int get_chara_llvm(char *line, char *token);
int get_line(char *line, FILE *fp);
int get_token(char *line, char *token);
int get_token_llvm(char *line, char *token);
TOKEN_ORDER * get_token_order_current();
TOKEN_ORDER * get_token_order_prev();
TOKEN_ORDER * get_token_order_top();
int insert_token_order(char *line, char *token, FILE *fp);
int iskanji(char *code);
int read_file(char *filename);
int read_token(char *token);
int set_token_order_current(TOKEN_ORDER * token_order);
int set_token_order_current_next(TOKEN_ORDER * token_order);
int set_token_order_prev_next(TOKEN_ORDER * token_order);

#endif
