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
/*!
 * @file	parser_ir_struct.c
 * @brief	structの解析と保存
 * @author	Hidemi Ishihara
 *
 * @note
 * structを解析し、struct_treeに格納します。
 *
 * @todo
 * structの初期化を実装する
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "synverll.h"
#include "token.h"
#include "parser.h"
#include "parser_ir.h"
#include "parser_ir_call.h"
#include "parser_ir_memmap.h"
#include "parser_ir_memory.h"
#include "parser_ir_proc.h"
#include "parser_ir_signal.h"
#include "parser_ir_struct.h"
#include "parser_ir_top.h"

extern PARSER_TREE_IR *parser_tree_ir_top;
extern PARSER_TREE_IR *parser_tree_ir_current;
extern PARSER_TREE_IR *parser_tree_ir_prev;

typedef struct struct_tree{
    struct struct_tree *prev_ptr;
    struct struct_tree *next_ptr;
    char			*label;
    char			*argument;
    int				size;
} STRUCT_TREE;

STRUCT_TREE *struct_tree_top        = NULL;

/*!
 * @brief	structの開放
 */
int clean_struct_tree()
{
	STRUCT_TREE *now_struct_tree;
	STRUCT_TREE *new_struct_tree;

    now_struct_tree = struct_tree_top;
    while(now_struct_tree != NULL){
        new_struct_tree = now_struct_tree->next_ptr;
        if(now_struct_tree->label != NULL) free(now_struct_tree->label);
        if(now_struct_tree->argument != NULL) free(now_struct_tree->argument);
        free(now_struct_tree);
        now_struct_tree = new_struct_tree;
    }
    struct_tree_top = NULL;
    return 0;
}

/*!
 * @brief	structを登録する
 */
int register_struct_tree(char *label, char *argument)
{
	STRUCT_TREE *now_struct_tree = NULL;
	STRUCT_TREE *old_struct_tree = NULL;

	now_struct_tree = struct_tree_top;
	while(now_struct_tree != NULL){
		old_struct_tree = now_struct_tree;
		now_struct_tree = now_struct_tree->next_ptr;
	}

	now_struct_tree				= (STRUCT_TREE *)calloc(sizeof(STRUCT_TREE),1);
	now_struct_tree->prev_ptr	= old_struct_tree;
	now_struct_tree->next_ptr	= NULL;

	if(struct_tree_top == NULL){
		struct_tree_top				= now_struct_tree;
	}else{
		old_struct_tree->next_ptr	= now_struct_tree;
	}


	now_struct_tree->label = charalloc(label);
	now_struct_tree->argument = charalloc(argument);
	return 0;
}

/*!
 * @brief	ファイル内全てのstructの登録
 *
 * @note
 * structの解析に使用する
 */
int parser_struct_tree()
{
	PARSER_TREE_IR *now_parser_tree_ir;

    printf("[Parsing struct type for LLCM-IR]\n");

    now_parser_tree_ir = parser_tree_ir_top;
    while(now_parser_tree_ir != NULL){
		if(now_parser_tree_ir->flag == PARSER_IR_FLAG_TYPE){
			register_struct_tree(now_parser_tree_ir->label, now_parser_tree_ir->type.name);
		}
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
    }

    return 0;
}

/*!
 * @brief	structの引数を返す
 */
int get_struct_argument(char *label, char *argument)
{
	STRUCT_TREE *now_struct_tree;

	strcpy(argument, "");

    now_struct_tree = struct_tree_top;
    while(now_struct_tree != NULL){
		if(!strcmp(now_struct_tree->label, label)){
			strcpy(argument, now_struct_tree->argument);
			break;
		}
		now_struct_tree = now_struct_tree->next_ptr;
    }

	return 0;
}

/*!
 * @brief	struct名か判定する
 */
int is_struct_name(char *label)
{
	STRUCT_TREE *now_struct_tree;

    now_struct_tree = struct_tree_top;
    while(now_struct_tree != NULL){
		if(!strcmp(now_struct_tree->label, label)) break;
		now_struct_tree = now_struct_tree->next_ptr;
    }
    if(now_struct_tree == NULL) return 0;

	return 1;
}
