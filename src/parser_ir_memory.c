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
 * @file	parser_ir_memory.c
 * @brief	メモリツリー
 * @author	Hidemi Ishiahra
 *
 * @note
 * 関数内のグローバル変数、グローバル構造体、グローバルメモリのツリー構築
 * モジュールの引数の登録もここで行う
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

MEMORY_TREE *memory_tree_top        = NULL;
MODULE_TREE *module_tree_top        = NULL;
MODULE_STACK *module_stack_top		= NULL;

extern int is_module_gm_if;

/*!
 * @brief	メモリツリーの領域確保
 */
MEMORY_TREE * alloc_memory_tree(){
	MEMORY_TREE *now_memory_tree		= NULL;
	MEMORY_TREE *old_memory_tree		= NULL;

	// メモリツリーに存在しなければ新規登録
	now_memory_tree = memory_tree_top;
	while(now_memory_tree != NULL){
		old_memory_tree = now_memory_tree;
		now_memory_tree = now_memory_tree->next_ptr;
	}

	now_memory_tree				= (MEMORY_TREE *)calloc(sizeof(MEMORY_TREE),1);
	now_memory_tree->prev_ptr	= old_memory_tree;
	now_memory_tree->next_ptr	= NULL;

	if(memory_tree_top == NULL){
		memory_tree_top				= now_memory_tree;
	}else{
		old_memory_tree->next_ptr	= now_memory_tree;
	}

	return now_memory_tree;
}

/*!
 * @brief	メモリツリーを新しくする？
 */
int new_memory_tree()
{
	memory_tree_top        = NULL;
	return 0;
}

/*!
 * @brief	メモリツリー削除
 */
int clean_memory_tree(MEMORY_TREE *memory_tree_ptr)
{
	MEMORY_TREE *now_memory_tree;
	MEMORY_TREE *new_memory_tree;

    now_memory_tree = memory_tree_ptr;
    while(now_memory_tree != NULL){
        new_memory_tree = now_memory_tree->next_ptr;
        if(now_memory_tree->label != NULL)		free(now_memory_tree->label);
        if(now_memory_tree->verilog_args != NULL)	free(now_memory_tree->verilog_args);
        if(now_memory_tree->type != NULL)			free(now_memory_tree->type);
        free(now_memory_tree);
        now_memory_tree = new_memory_tree;
    }
    memory_tree_top = NULL;

	if(module_name != NULL){
		free(module_name);
		module_name = NULL;
	}

    return 0;
}

/*!
 * @brief	メモリツリーのVerilog HDL表示
 *
 * @param	flag 0 args出力
 * @param	flag 1 return出力
 * @param	flag 2 proc出力
 * @param	flag 3 decl(宣言)出力
 */
int output_memory_tree(FILE *fp, int flag)
{
	MEMORY_TREE *now_memory_tree;

    now_memory_tree = memory_tree_top;
    while(now_memory_tree != NULL){
		switch(flag){
			case 0:
				// 引数の表示
				if(now_memory_tree->verilog_args != NULL){
					if(strlen(now_memory_tree->verilog_args) > 0){
						if(
							(get_adrs_memmap("GLOBAL", now_memory_tree->label) == -1) &&
							(now_memory_tree->flag != MEMORY_FLAG_RETURN)
						){
							fprintf(fp, "%s\n", now_memory_tree->verilog_args);
						}else if(now_memory_tree->flag == MEMORY_FLAG_RETURN){
						}else{
							now_memory_tree->flag = MEMORY_FLAG_LOCAL;
						}
					}
				}
				break;
			case 1:
				// returnの表示
				if(now_memory_tree->verilog_args != NULL){
					if(strlen(now_memory_tree->verilog_args) > 0){
						if(
							(get_adrs_memmap("GLOBAL", now_memory_tree->label) == -1) &&
							(now_memory_tree->flag == MEMORY_FLAG_RETURN)
						){
							fprintf(fp, "%s\n", now_memory_tree->verilog_args);
						}
					}
				}
				break;
			case 2:
				// 宣言の表示
				if(now_memory_tree->verilog_decl != NULL){
					if(strlen(now_memory_tree->verilog_decl) > 0){
						if(get_adrs_memmap("GLOBAL", now_memory_tree->label) == -1){
							fprintf(fp, "%s\n", now_memory_tree->verilog_decl);
						}
					}
				}
				break;
			case 3:
				// procの表示
				if(now_memory_tree->verilog_proc != NULL){
					if(strlen(now_memory_tree->verilog_proc) > 0){
						if(get_adrs_memmap("GLOBAL", now_memory_tree->label) == -1){
							fprintf(fp, "%s\n", now_memory_tree->verilog_proc);
						}
					}
				}
				break;
			default:
				printf("[ERROR] output_memory_tree()\n");
				break;
		}
        now_memory_tree = now_memory_tree->next_ptr;
    }
    return 0;
}

/*!
 * @brief	メモリのタイプを表示する
 */
int print_memory_tree(MEMORY_TREE *now_memory_tree)
{
//	MEMORY_TREE *now_memory_tree;

	printf("  [Memory Tree List]\n");

    while(now_memory_tree != NULL){
		printf("  %s: ", now_memory_tree->label);
		printf("  type = %s, ", now_memory_tree->type);
		printf("  flag = %d, ", now_memory_tree->flag);
		printf("\n");
        now_memory_tree = now_memory_tree->next_ptr;
    }

    return 0;
}

/*!
 * @brief	メモリタイプの展開
 *
 * サンプル
 * %struct.DQTinfotype = type { i16, i16, i8, [64 x i8], i8, [64 x i8] }
 * @DQTinfo = external global %struct.DQTinfotype
 *
 * @DQTinfoのstructを展開してしまう。
 */
int deploy_array_type()
{
	MEMORY_TREE *now_memory_tree;
	char *buf = NULL;
	char *buf_temp = NULL;
	char *token;
	char *argument;
	int convert = 0;

	printf(" -> deploy_array_type\n");

	token = malloc(STR_MAX);
	argument = malloc(STR_MAX);

	buf = calloc(1,1);
	buf_temp = calloc(1,1);

	/*
	 * arryタイプの中にstructが無くなるまで置換し続けます。
	 */
	do{
		convert = 0;

		now_memory_tree = memory_tree_top;
		while(now_memory_tree != NULL){
			if(now_memory_tree->type != NULL){
				do{
					get_token_llvm(now_memory_tree->type, token);

					buf_temp = charalloc(buf);
					free(buf);

					if(is_struct_name(token)){
						// struct名を発見した場合、structの中身に置き換える
						get_struct_argument(token, argument);
						buf = calloc(strlen(buf_temp) + strlen(argument) + 3,1);
						sprintf(buf, "%s %s ", buf_temp, argument);
						convert = 1;
					}else{
						buf = calloc(strlen(buf_temp) + strlen(token) + 3,1);
						sprintf(buf, "%s %s ", buf_temp, token);
					}

					free(buf_temp);
					buf_temp = calloc(1,1);

				}while(strlen(now_memory_tree->type) > 0);
			}
			free(now_memory_tree->type);
			now_memory_tree->type = calloc(strlen(buf)+1,1);
			strcpy(now_memory_tree->type, buf);

			free(buf);
			buf = calloc(1,1);

			now_memory_tree = now_memory_tree->next_ptr;
		}
	}while(convert);

    free(buf);
    free(buf_temp);
    free(token);
    free(argument);

    return 0;
}

/*!
 * @brief	メモリツリー構築
 *
 * メモリツリーは次の宣言を処理対象とする。
 * ステージ0に存在するarray宣言
 * MODULEの引数に含まれるポインタ宣言
 * alloca宣言
 */
int parser_memory_tree()
{
	MEMORY_TREE		*memory_tree_current;
	PARSER_TREE_IR	*now_parser_tree_ir;
	char *line;
	char token[STR_MAX];
	char type[STR_MAX];
	char verilog_args[STR_MAX];
	char verilog_proc[STR_MAX];
	char verilog_decl[STR_MAX];
	char *str;
	int num = 0;
	int width;

    printf(" -> Parser Memory Tree\n");

	strcpy(verilog_args, "");

    now_parser_tree_ir = parser_tree_ir_top;
    while(now_parser_tree_ir != NULL){

		if( now_parser_tree_ir->stage == 0 ){
			// stage = 0のみ対象
			switch(now_parser_tree_ir->flag){
				case PARSER_IR_FLAG_ARRAY:
						memory_tree_current = alloc_memory_tree();
						
						memory_tree_current->label	= charalloc(now_parser_tree_ir->label);
						memory_tree_current->type	= charalloc(now_parser_tree_ir->array.type);
						memory_tree_current->num	= -1;
						memory_tree_current->flag	= MEMORY_FLAG_GLOBAL;
						str = charalloc(memory_tree_current->label);
						sprintf(verilog_args, "\tinput [31:0] __base_%s,", convname(sep_p(str)));
						sprintf(verilog_proc, "\t\t\t__sig_%s <= __base_%s;", convname(sep_p(str)), convname(sep_p(str)));
						sprintf(verilog_decl, "\treg [31:0] __sig_%s;", convname(sep_p(str)));
						free(str);
						memory_tree_current->verilog_args = charalloc(verilog_args);
						memory_tree_current->verilog_proc = charalloc(verilog_proc);
						memory_tree_current->verilog_decl = charalloc(verilog_decl);
					break;
				default:
					// ARRAY以外は処理の対象外
					break;
			}
		}else{
			switch(now_parser_tree_ir->flag){
				case PARSER_IR_FLAG_MODULE:
					/*
					 * 関数の引数で受け渡されているポインタのメモリタイプを抜き出す
					 */
					/*
					 * ToDo:
					 *  ここでモジュールの引数の番号の関連付けを作成しなければいけない
					 *  ここで信号定義が作成できるはず
					 */
					line = charalloc(now_parser_tree_ir->module.argument);

					// "("
					get_token_llvm(line, token);

					while(strlen(line) > 0){
						// タイプ
						get_irtype_llvm(line, token);
						if(!strcmp(token, "") || !strcmp(token, ")")) break;
						strcpy(type, token);
						if(is_pointer_type(token)){
							// ポインタの場合
							memory_tree_current = alloc_memory_tree();
							memory_tree_current->flag = MEMORY_FLAG_REGISTER;
							memory_tree_current->num = num++;

							do{
								get_token_llvm(line, token);
								if(!strcmp(token, "align")){
									get_token_llvm(line, token);
									get_token_llvm(line, token);
								}
							}while(
								!strcmp(token, "zeroext") ||
								!strcmp(token, "signext") ||
								!strcmp(token, "inreg") ||
								!strcmp(token, "byval") ||
								!strcmp(token, "sret") ||
								!strcmp(token, "align") ||
								!strcmp(token, "noalias") ||
								!strcmp(token, "nocapture") ||
								!strcmp(token, "nest") ||
								!strcmp(token, "readonly") ||
								!strcmp(token, "readnone") ||
								!strcmp(token, "returned") ||
								!strcmp(token, "dereferenceable") ||
								!strcmp(token, "dereferenceable_or_null")
							);

							memory_tree_current->label = charalloc(token);
							{
								width = get_width(type);
									str = charalloc(memory_tree_current->label);
									sprintf(verilog_args, "\tinput [%d:0] __args_%s,", (width-1), convname(sep_p(str)));
									sprintf(verilog_proc, "\t\t\t__sig_%s <= __args_%s;", convname(sep_p(str)), convname(sep_p(str)));
									sprintf(verilog_decl, "\treg [%d:0] __sig_%s;", (width-1), convname(sep_p(str)));
									free(str);
								convtype(type);
								memory_tree_current->type = charalloc(type);
								memory_tree_current->size = width/4;
							}
						}else{
							/*
							 * 変数の場合は処理をしません。
							 * タイプで入力サイズを変えなければいけないかもしれない。
							 */
							memory_tree_current = alloc_memory_tree();
							memory_tree_current->num = num++;
							memory_tree_current->flag = MEMORY_FLAG_REGISTER;

							get_irtype_llvm(line, token);
							/*
							 * zeroext
							 * signext
							 * inreg
							 * byval
							 * inalloca
							 * sret
							 * align <h>
							 * noalias
							 * nocapture
							 * nest
							 * returned
							 * nonull
							 * dereferenceable(<n>)
							 * dereferenceable_or_null(<n>)
							 */
							if(!strcmp(token, "zeroext") || !strcmp(token, "signext")){
								get_token_llvm(line, token);
							}

							memory_tree_current->label = charalloc(token);

							width = get_width(type);

							sprintf(verilog_args, "\tinput [%d:0] __args_%s,", (width-1), convname(sep_p(token)));
							sprintf(verilog_proc, "\t\t\t__sig_%s <= __args_%s;", convname(sep_p(token)), convname(sep_p(token)));
							sprintf(verilog_decl, "\treg [%d:0] __sig_%s;", (width-1), convname(sep_p(token)));

							memory_tree_current->type = charalloc(type);
							memory_tree_current->size = width/8;
						}
						memory_tree_current->verilog_args = charalloc(verilog_args);
						memory_tree_current->verilog_proc = charalloc(verilog_proc);
						memory_tree_current->verilog_decl = charalloc(verilog_decl);
						// 次の","か")"まで読み飛ばし
						do{
							get_token_llvm(line, token);
						}while(strcmp(token, ",") && strcmp(token, ")"));
					}

					free(line);

					break;

				case PARSER_IR_FLAG_ALLOCA:
					/*
					 * allocaされているメモリを登録する
					 */
					memory_tree_current = alloc_memory_tree();
					memory_tree_current->label = charalloc(now_parser_tree_ir->label);
					memory_tree_current->type = charalloc(now_parser_tree_ir->alloca.type);
					memory_tree_current->flag = MEMORY_FLAG_LOCAL;

					break;

				case PARSER_IR_FLAG_RETURN:
					memory_tree_current = alloc_memory_tree();
					memory_tree_current->flag = MEMORY_FLAG_RETURN;

					if(strcmp(now_parser_tree_ir->ret.type, "void")){
						width = get_width(now_parser_tree_ir->ret.type);
						memory_tree_current->label = charalloc(now_parser_tree_ir->ret.name);
						memory_tree_current->type = charalloc(now_parser_tree_ir->ret.type);
						memory_tree_current->size = width/8;

						sprintf(verilog_args, "\toutput reg [%d:0] __func_result\n", (width-1));
					}else{
						memory_tree_current->label = charalloc("");
						memory_tree_current->type = charalloc("void");
						memory_tree_current->size = 0;

						sprintf(verilog_args, "\toutput reg __func_result\n");
					}

					memory_tree_current->verilog_args = charalloc(verilog_args);

					break;
				default:
					// 他は何もしない
					break;
			}
		}
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
    }
    return 0;
}

/*!
 * @brief	struct型のサイズを返す
 */
int __get_struct_size(char *type, char *ptr, char *rslt_ptr)
{
	char *buf;
	char *token;
	int size = 0;
	int total = 0;
	char *buf_ptr = NULL;
	int ptr_cnt = 0;
	char *temp = NULL;
	int size_count = 0;

	token = malloc(STR_MAX);
	temp = calloc(STR_MAX,1);

	buf = charalloc(type);

	// {
	get_token_llvm(buf, token);

	if(ptr != NULL){
		buf_ptr = charalloc(ptr);
	}

	/*
	 * ポインタの位置の検出
	 */
	if(buf_ptr != NULL){
		// Type
		get_irtype_llvm(buf_ptr, token);
		// Name
		get_token_llvm(buf_ptr, token);
		ptr_cnt = atoi(token);	// 数字しかありえないはず
		// ,
		get_token_llvm(buf_ptr, token);
	}

	do{
		get_irtype_llvm(buf, token);
		if((strlen(token) == 0) || (!strcmp(token, ")"))) break;

		if(token[0] == '['){
			size = __get_array_size(token, buf_ptr, temp);
		}else if(token[0] == '{'){
			size = __get_struct_size(token, buf_ptr, temp);
		}else if(!strcmp(token, "i8")){
			size = 1;
		}else if(!strcmp(token, "i16")){
			size = 2;
		}else if(!strcmp(token, "i32")){
			size = 4;
		}
		total += size;

		if(ptr_cnt > 0){
			size_count += size;
			ptr_cnt--;
		}

		// ","か"}"
		get_token_llvm(buf, token);
	}while(strlen(buf) > 0);

	if(ptr != NULL){
		if(rslt_ptr != NULL){
			if(strlen(temp) > 0){
				sprintf(rslt_ptr, "%s + (%d)", temp, size_count);
			}else{
				sprintf(rslt_ptr, "(%d)", size_count);
			}
		}
	}

	free(token);

	return total;
}

/*!
 * @brief	array型のサイズを返す
 */
int __get_array_size(char *type, char *ptr, char *rslt_ptr)
{
	char *buf;
	char *token;
	int size = 0;
	int total = 0;
	int num = 0;
	char *buf_ptr = NULL;
	char *ptr_cnt = NULL;
	char *temp = NULL;
	char *str = NULL;

	token = malloc(STR_MAX);
	temp = calloc(STR_MAX,1);

	buf = charalloc(type);

	if(ptr != NULL){
		if(strlen(ptr) > 0){
			buf_ptr = charalloc(ptr);
		}
	}

	/*
	 * ポインタの位置の検出
	 */
	if(buf_ptr != NULL){
		// Type
		get_irtype_llvm(buf_ptr, token);
		// Name
		get_token_llvm(buf_ptr, token);
		str = regalloc(token);
		ptr_cnt = charalloc(str);
		free(str);
		// ,
		get_token_llvm(buf_ptr, token);
	}

	// [ num x type ]
	// [
	get_token_llvm(buf, token);
	// num
	get_token_llvm(buf, token);
	num = atoi(token);
	// x
	get_token_llvm(buf, token);
	// type
	get_irtype_llvm(buf, token);

	if(token[0] == '['){
		size = __get_array_size(token, buf_ptr, temp);
	}else if(token[0] == '{'){
		size = __get_struct_size(token, buf_ptr, temp);
	}else if(!strcmp(token, "i8")){
		size = 1;
	}else if(!strcmp(token, "i16")){
		size = 2;
	}else if(!strcmp(token, "i32")){
		size = 4;
	}
	total += size * num;

	// ]
	get_token_llvm(buf, token);

	if(ptr_cnt != NULL){
		if(rslt_ptr != NULL){
			if(strlen(temp) > 0){
				sprintf(rslt_ptr, "%s + (%d * %s)", temp, size, ptr_cnt);
			}else{
				sprintf(rslt_ptr, "(%d * %s)", size, ptr_cnt);
			}
		}
		free(ptr_cnt);
	}

	return total;
}

/*!
 * @brief	RAMのサイズを返す
 *
 * @note	ptrが入力されている場合は、ptrのポインタ位置を生成して返す。
 */
int get_memory_size(char *label, char *ptr, char *rslt_ptr)
{
	MEMORY_TREE *now_memory_tree;
	int size = 0;
	int total = 0;
	int base_adrs = 0;
	char *buf = NULL;
	char *token = NULL;
	char *buf_ptr = NULL;
	char *ptr_cnt = NULL;
	char *temp = NULL;
	char *str = NULL;

	now_memory_tree = memory_tree_top;
    while(now_memory_tree != NULL){
		if(!strcmp(now_memory_tree->label, label)) break;
        now_memory_tree = now_memory_tree->next_ptr;
    }
	if(now_memory_tree == NULL){
		printf("[ABORT] get_memory_size(): unknown memory = \"%s\"\n", label);
		exit(0);
	}

	token		= calloc(STR_MAX,1);
	temp		= calloc(STR_MAX,1);
	buf			= calloc(strlen(now_memory_tree->type)+1,1);
	strcpy(buf, now_memory_tree->type);

	if(ptr != NULL){
		if(strlen(ptr) > 0){
			buf_ptr = calloc(strlen(ptr)+1,1);
			strcpy(buf_ptr, ptr);
		}
	}

	size = 0;
	total = 0;
	do{
		/*
		 * ptrからアドレスポインタを取得する
		 */
		if(buf_ptr != NULL){
			// Type
			get_irtype_llvm(buf_ptr, token);
			// Name
			get_token_llvm(buf_ptr, token);
			str = regalloc(token);
			ptr_cnt = charalloc(str);
			free(str);
			// ,
			get_token_llvm(buf_ptr, token);
		}

		get_irtype_llvm(buf, token);
		if(strlen(token) == 0) break;

		if(token[0] == '['){
			size = __get_array_size(token, buf_ptr, temp);
		}else if(token[0] == '{'){
			size = __get_struct_size(token, buf_ptr, temp);
		}else if(!strcmp(token, "i8")){
			size = 1;
		}else if(!strcmp(token, "i16")){
			size = 2;
		}else if(!strcmp(token, "i32")){
			size = 4;
		}
		total += size;

		if(ptr_cnt != NULL){
			if(rslt_ptr != NULL){
				if(strlen(temp) > 0){
					sprintf(temp, "%s + (%d * %s)", temp, size, ptr_cnt);
				}else{
					sprintf(temp, "(%d * %s)", size, ptr_cnt);
				}
			}
			free(ptr_cnt);
		}

	}while(strlen(buf) > 0);
	if(token != NULL) free(token);
	if(buf != NULL) free(buf);

	if(rslt_ptr != NULL){
		if(label[0] == '@'){
			// グローバルメモリ
			base_adrs = get_adrs_memmap("GLOBAL", label);
			size = get_size_memmap("GLOBAL", label);
		}else if(label[0] == '%'){
			// ローカルメモリ
			base_adrs = get_adrs_memmap(module_name, label);
			size = get_size_memmap(module_name, label);
		}

		if(size > 0){
			sprintf(rslt_ptr, "__gm_base + %d + %s", base_adrs, temp);
		}else{
			buf = regalloc(label);
			sprintf(rslt_ptr, "%s + %s", buf, temp);
			free(buf);
		}
	}

	return total;
}

/*!
 * @brief	メモリのサイズ取得とメモリマップへ登録
 *
 * @note
 */
int create_array_size()
{
	MEMORY_TREE *now_memory_tree;
	int size = 0;

    now_memory_tree = memory_tree_top;
    while(now_memory_tree != NULL){
		size = get_memory_size(now_memory_tree->label, NULL, NULL);

		if(size <= 0){
			size = 0;
		}

		if(now_memory_tree->flag == MEMORY_FLAG_GLOBAL){
			// グローバルメモリ
			register_memmap_tree("GLOBAL", now_memory_tree->label, size);
		}else if(now_memory_tree->flag == MEMORY_FLAG_LOCAL){
			// ローカルメモリ
			register_memmap_tree(module_name, now_memory_tree->label, size);
		}else if(now_memory_tree->flag == MEMORY_FLAG_POINTER){
			// ポインタ
			register_memmap_tree(module_name, now_memory_tree->label, 0);
		}

        now_memory_tree = now_memory_tree->next_ptr;
    }
	if(now_memory_tree == NULL) return MEMORY_FLAG_UNKNWON;

	return 0;
}

/*!
 * @brief	モジュールツリー削除
 */
int clean_module_tree()
{
	MODULE_TREE *now_module_tree;
	MODULE_TREE *new_module_tree;

    now_module_tree = module_tree_top;
    while(now_module_tree != NULL){
        new_module_tree = now_module_tree->next_ptr;
        if(now_module_tree->module_name != NULL) free(now_module_tree->module_name);
        if(now_module_tree->memory_tree_ptr != NULL) clean_memory_tree(now_module_tree->memory_tree_ptr);
        if(now_module_tree->call_tree_ptr != NULL) clean_call_tree(now_module_tree->memory_tree_ptr);
        free(now_module_tree);
        now_module_tree = new_module_tree;
    }
    module_tree_top = NULL;
    return 0;
}

/*!
 * @brief	モジュールツリーの検索
 */
int search_module_tree(char *module, char label)
{
	MODULE_TREE *now_module_tree;

    now_module_tree = module_tree_top;
    while(now_module_tree != NULL){
        now_module_tree = now_module_tree->next_ptr;
    }
    if(now_module_tree == NULL) return -1;

    return 0;
}

/*!
 * @brief	モジュールとメモリツリー、CALLツリーのポインタを登録する
 */
int register_module_tree(char *module_name)
{
	MODULE_TREE *now_module_tree = NULL;
	MODULE_TREE *old_module_tree = NULL;

	// モジュールツリーに存在しなければ新規登録
	now_module_tree = module_tree_top;
	while(now_module_tree != NULL){
		old_module_tree = now_module_tree;
		now_module_tree = now_module_tree->next_ptr;
	}

	now_module_tree				= (MODULE_TREE *)calloc(sizeof(MODULE_TREE),1);
	now_module_tree->prev_ptr	= old_module_tree;
	now_module_tree->next_ptr	= NULL;

	if(module_tree_top == NULL){
		module_tree_top				= now_module_tree;
	}else{
		old_module_tree->next_ptr	= now_module_tree;
	}

	now_module_tree->module_name = calloc(strlen(module_name)+1,1);
	strcpy(now_module_tree->module_name, module_name);
	now_module_tree->memory_tree_ptr	= memory_tree_top;
	now_module_tree->call_tree_ptr		= call_tree_top;
	now_module_tree->is_module_gm_if	= is_module_gm_if;

	return 0;
}

/*!
 * @brief	モジュールツリーの表示
 */
int print_module_tree(FILE *fp)
{
	MODULE_TREE *now_module_tree;

	printf(" -> Output Module List\n");

	fprintf(fp, "======================================================================\n");
	fprintf(fp, "Module List\n");
	fprintf(fp, "======================================================================\n");

    now_module_tree = module_tree_top;
    while(now_module_tree != NULL){
		fprintf(fp, "%s:\n", now_module_tree->module_name);
        now_module_tree = now_module_tree->next_ptr;
    }
	fprintf(fp, "======================================================================\n");
    return 0;
}

/*!
 * @brief	モジュールスタックの登録
 */
int new_module_tree()
{
	memory_tree_top        = NULL;
	call_tree_top        = NULL;

	return 0;
}
