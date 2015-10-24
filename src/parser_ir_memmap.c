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
 * @file	parser_ir_memmap.c
 * @brief	メモリマップ
 * @author	Hidemi Ishiahra
 *
 * @note
 * メモリマップ
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct memmap_tree{
    struct memmap_tree	*prev_ptr;
    struct memmap_tree	*next_ptr;
    char				*module;	// モジュール名
    char				*label;		// ラベル
    unsigned int		adrs;
    unsigned int		size;
} MEMMAP_TREE;

MEMMAP_TREE *memmap_tree_top        = NULL;
MEMMAP_TREE *memmap_tree_current    = NULL;
MEMMAP_TREE *memmap_tree_prev       = NULL;

/*!
 * @brief	メモリマップの登録
 */
int insert_memmap_tree()
{
    if(memmap_tree_top == NULL){
        memmap_tree_top     = (MEMMAP_TREE *)malloc(sizeof(MEMMAP_TREE));
        memset(memmap_tree_top, 0, sizeof(MEMMAP_TREE));
        memmap_tree_current = memmap_tree_top;
    }else{
        memmap_tree_current->next_ptr   = (MEMMAP_TREE *)malloc(sizeof(MEMMAP_TREE));
        memset(memmap_tree_current->next_ptr, 0, sizeof(MEMMAP_TREE));
        memmap_tree_prev                = memmap_tree_current;
        memmap_tree_current             = memmap_tree_current->next_ptr;
        memmap_tree_current->prev_ptr   = memmap_tree_prev;
        memmap_tree_current->next_ptr   = NULL;
    }
    return 0;
}

/*!
 * @brief	メモリマップの削除
 */
int clean_memmap_tree()
{
	MEMMAP_TREE *now_memmap_tree;
	MEMMAP_TREE *new_memmap_tree;

    now_memmap_tree = memmap_tree_top;
    while(now_memmap_tree != NULL){
        new_memmap_tree = now_memmap_tree->next_ptr;
        if(now_memmap_tree->label != NULL) free(now_memmap_tree->label);
        if(now_memmap_tree->module != NULL) free(now_memmap_tree->module);
        free(now_memmap_tree);
        now_memmap_tree = new_memmap_tree;
    }
    memmap_tree_top = NULL;
    memmap_tree_current = memmap_tree_top;
    return 0;
}

/*!
 * @brief	メモリマップへ登録
 *
 * @note
 * メモリマップの登録と同時にメモリアドレスを生成します。
 */
int register_memmap_tree(char *module, char *label, int size)
{
	unsigned int new_adrs = 0;

	/*
	 * 新しいアドレスの作成
	 */
	if(memmap_tree_current != NULL){
		new_adrs = (memmap_tree_current->adrs + memmap_tree_current->size + 3) & 0xFFFFFFC;
	}else{
		new_adrs = 0;
	}

	if(!is_memmap_tree(module, label)){
		insert_memmap_tree();

		memmap_tree_current->label = calloc(strlen(label)+1,1);
		strcpy(memmap_tree_current->label, label);
		memmap_tree_current->module = calloc(strlen(module)+1,1);
		strcpy(memmap_tree_current->module, module);
		memmap_tree_current->adrs = new_adrs;		// アドレスの登録
		memmap_tree_current->size = size;
	}

	return 0;
}

/*!
 * @brief	メモリマップの検索
 */
int is_memmap_tree(char *module, char *label)
{
	MEMMAP_TREE *now_memmap_tree;

    now_memmap_tree = memmap_tree_top;
    while(now_memmap_tree != NULL){
		if(!strcmp(now_memmap_tree->module, module) && !strcmp(now_memmap_tree->label, label)){
			return 1;
		}
        now_memmap_tree = now_memmap_tree->next_ptr;
    }

    return 0;
}

/*!
 * @brief	メモリアドレスの取得
 */
unsigned int get_adrs_memmap(char *module, char *label)
{
	MEMMAP_TREE *now_memmap_tree;

    now_memmap_tree = memmap_tree_top;
    while(now_memmap_tree != NULL){
		if(
			!strcmp(now_memmap_tree->module, module) &&
			!strcmp(now_memmap_tree->label, label) &&
			(now_memmap_tree->size > 0)
		){
			return now_memmap_tree->adrs;
		}
        now_memmap_tree = now_memmap_tree->next_ptr;
    }

    return -1;
}

/*!
 * @brief	メモリサイズの取得
 */
unsigned int get_size_memmap(char *module, char *label)
{
	MEMMAP_TREE *now_memmap_tree;

    now_memmap_tree = memmap_tree_top;
    while(now_memmap_tree != NULL){
		if(!strcmp(now_memmap_tree->module, module) && !strcmp(now_memmap_tree->label, label)){
			return now_memmap_tree->size;
		}
        now_memmap_tree = now_memmap_tree->next_ptr;
    }

    return -1;
}

/*!
 * @brief	メモリマップを出力する
 */
int print_memmap_tree(FILE *fp)
{
	MEMMAP_TREE *now_memmap_tree;

	printf(" -> Output Memory Map\n");

	fprintf(fp, "======================================================================\n");
	fprintf(fp, "Memory Map\n");
	fprintf(fp, "======================================================================\n");
	fprintf(fp, "[GLOBAL/LOCAL]   [MEMORY NAME]                    [ADDRESS] [SIZE]\n");

    now_memmap_tree = memmap_tree_top;
    while(now_memmap_tree != NULL){
//		if(now_memmap_tree->size > 0)
		fprintf(fp, "%-16s %-32s %08x  %8d\n", now_memmap_tree->module, now_memmap_tree->label, now_memmap_tree->adrs, now_memmap_tree->size);
        now_memmap_tree = now_memmap_tree->next_ptr;
    }

	fprintf(fp, "======================================================================\n");

    return 0;
}
