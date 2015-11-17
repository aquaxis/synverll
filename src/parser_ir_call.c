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
 * @file	parser_ir_call.c
 * @brief	CALL命令の管理
 * @author	Hidemi Ishiahra
 *
 * @note
 * CALL命令とCALL命令に付随する信号を管理します。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

CALL_TREE *call_tree_top				= NULL;
CALL_SIGNAL_TREE *call_signal_tree_top	= NULL;

/*!
 * @brief	CALL命令の削除
 */
int clean_call_tree()
{
	CALL_TREE *now_call_tree;
	CALL_TREE *new_call_tree;

    now_call_tree = call_tree_top;
    while(now_call_tree != NULL){
        new_call_tree = now_call_tree->next_ptr;
        if(now_call_tree->call_name != NULL)	free(now_call_tree->call_name);
        if(now_call_tree->verilog != NULL)	free(now_call_tree->verilog);
        free(now_call_tree);
        now_call_tree = new_call_tree;
    }
    call_tree_top = NULL;
    return 0;
}

/*!
 * @brief	CALL命令の検索
 *
 * @param	call_name	CALL命令の名前
 * @return	0	該当なし
 * @return	1	該当有り

 * @note
 * call_nameで示されたCALL命令がCALLツリーに存在するか検索します。
 *
 */
int is_call_tree(char *call_name)
{
	CALL_TREE *now_call_tree = NULL;
	now_call_tree = call_tree_top;
	while(now_call_tree != NULL){
		if(!strcmp(now_call_tree->call_name, call_name)) return 1;
		now_call_tree = now_call_tree->next_ptr;
	}
    return 0;
}

/*
 * @brief	CALL名を検索しツリーを返す
 */
CALL_TREE * search_call_tree(char *call_name)
{
	CALL_TREE *now_call_tree = NULL;
	now_call_tree = call_tree_top;
	while(now_call_tree != NULL){
		if(!strcmp(now_call_tree->call_name, call_name)) return now_call_tree;
		now_call_tree = now_call_tree->next_ptr;
	}
    return NULL;
}

/*!
 * @brief	CALL命令の登録
 *
 * @return	0	登録
 * @return	-1	未登録(既に存在)
 *
 * @note
 * CALL命令を登録します。
 * 既にCALL命令が登録されている場合は登録しません。
 */
int register_call_tree(char *call_name)
{
	CALL_TREE *now_call_tree		= NULL;
	CALL_TREE *old_call_tree		= NULL;

	if(!is_call_tree(call_name)){
		now_call_tree = call_tree_top;
		while(now_call_tree != NULL){
			old_call_tree = now_call_tree;
			now_call_tree = now_call_tree->next_ptr;
		}

		now_call_tree				= (CALL_TREE *)calloc(sizeof(CALL_TREE),1);
		now_call_tree->prev_ptr	= old_call_tree;
		now_call_tree->next_ptr	= NULL;

		if(call_tree_top == NULL){
			call_tree_top				= now_call_tree;
		}else{
			old_call_tree->next_ptr	= now_call_tree;
		}
		now_call_tree->signal_top_ptr = NULL;

		now_call_tree->call_name = calloc(strlen(call_name)+1,1);
		strcpy(now_call_tree->call_name, call_name);
	}else{
		// 既に生成されている
		return -1;
	}

	return 0;
}

/*!
 * @brief	CALL命令の信号登録
 *
 * @param	call_name	CALL名
 * @param	signal_name	信号名
 * @param	num			引数の番号
 * @param	inout		信号の入出力(0:IN、1:OUT)
 */
int register_call_signal_tree(char *call_name, char *signal_name, int num, int size, int inout)
{
	CALL_TREE *now_call_tree;
	CALL_SIGNAL_TREE *now_call_signal_tree = NULL;
	CALL_SIGNAL_TREE *old_call_signal_tree = NULL;

	now_call_tree = search_call_tree(call_name);
	
	now_call_signal_tree = now_call_tree->signal_top_ptr;
	while(now_call_signal_tree != NULL){
		old_call_signal_tree = now_call_signal_tree;
		now_call_signal_tree = now_call_signal_tree->next_ptr;
	}

	now_call_signal_tree				= (CALL_SIGNAL_TREE *)calloc(sizeof(CALL_SIGNAL_TREE),1);
	now_call_signal_tree->prev_ptr	= old_call_signal_tree;
	now_call_signal_tree->next_ptr	= NULL;

	if(now_call_tree->signal_top_ptr == NULL){
		now_call_tree->signal_top_ptr				= now_call_signal_tree;
	}else{
		old_call_signal_tree->next_ptr	= now_call_signal_tree;
	}

	now_call_signal_tree->signal_name = calloc(strlen(signal_name)+1,1);
	strcpy(now_call_signal_tree->signal_name, signal_name);

	now_call_signal_tree->inout	= inout;
	now_call_signal_tree->num	= num;
	now_call_signal_tree->size	= size;

	return 0;
}

/*!
 * @brief	CALL命令の信号を表示する
 */
int print_call_signal_tree(CALL_SIGNAL_TREE *now_call_signal_tree, char *call_name)
{
	printf("    [CALL Signal Tree List]\n");

    while(now_call_signal_tree != NULL){
		if(
			!strcmp(now_call_signal_tree->signal_name, "req") ||
			!strcmp(now_call_signal_tree->signal_name, "ready") ||
			!strcmp(now_call_signal_tree->signal_name, "done")
		){
			printf("    __call_%s_%s\n", call_name, now_call_signal_tree->signal_name);
		}else{
			printf("    __call_%s_%s_%d\n", call_name, now_call_signal_tree->signal_name, now_call_signal_tree->num);
		}
        now_call_signal_tree = now_call_signal_tree->next_ptr;
    }
    return 0;
}

/*!
 * @brief	CALL命令を表示する
 */
int print_call_tree(CALL_TREE *now_call_tree)
{
	printf("  [CALL Tree List]\n");

    while(now_call_tree != NULL){
		printf("  %s\n", now_call_tree->call_name);
		print_call_signal_tree(now_call_tree->signal_top_ptr, now_call_tree->call_name);
        now_call_tree = now_call_tree->next_ptr;
    }
    return 0;
}

/*!
 * @brief	CALL命令 I/FのVerilog HDL表示
 */
int output_call_signal_tree(FILE *fp)
{
	CALL_TREE *now_call_tree;
	CALL_SIGNAL_TREE *now_call_signal_tree;
	char *temp;

	temp = calloc(STR_MAX, 1);

    now_call_tree = call_tree_top;
    while(now_call_tree != NULL){
		now_call_signal_tree = now_call_tree->signal_top_ptr;
		while(now_call_signal_tree != NULL){
			switch(now_call_signal_tree->inout){
				case 0:	// in
					sprintf(temp, "\tinput ");
					break;
				case 1:	// out
					sprintf(temp, "\toutput reg ");
					break;
			}
			if(now_call_signal_tree->size > 0){
					sprintf(temp, "%s[%d:0] ", temp, now_call_signal_tree->size -1);
			}
			sprintf(temp, "%s__call_%s_%s",
				temp,
				convname(sep_p(now_call_tree->call_name)),
				now_call_signal_tree->signal_name
			);
			if(
				strcmp(now_call_signal_tree->signal_name, "req") &&
				strcmp(now_call_signal_tree->signal_name, "ready") &&
				strcmp(now_call_signal_tree->signal_name, "done") &&
				strcmp(now_call_signal_tree->signal_name, "result")
			){
				sprintf(temp, "%s_%d",
					temp,
					now_call_signal_tree->num
				);
			}
			fprintf(fp, "%s,\n", temp);
			now_call_signal_tree = now_call_signal_tree->next_ptr;
		}
		fprintf(fp, "\n");

        now_call_tree = now_call_tree->next_ptr;
    }

	free(temp);

    return 0;
}
