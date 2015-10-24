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
 * @file	parser_ir.c
 * @brief	構文解析 - LLVM-IR
 * @author	Hidemi Ishiahra
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

PARSER_TREE_IR *parser_tree_ir_top        = NULL;
PARSER_TREE_IR *parser_tree_ir_current    = NULL;
PARSER_TREE_IR *parser_tree_ir_prev       = NULL;

char current_ir_label[STR_MAX];	// 現在処理中のラベル名
char *module_name = NULL;

int insert_parser_tree_ir(char *str)
{
    // パーサーツリーの新規登録
    if(parser_tree_ir_top == NULL){
        parser_tree_ir_top     = (PARSER_TREE_IR *)malloc(sizeof(PARSER_TREE_IR));
        memset(parser_tree_ir_top, 0, sizeof(PARSER_TREE_IR));
        parser_tree_ir_current = parser_tree_ir_top;
    }else{
        parser_tree_ir_current->next_ptr   = (PARSER_TREE_IR *)malloc(sizeof(PARSER_TREE_IR));
        memset(parser_tree_ir_current->next_ptr, 0, sizeof(PARSER_TREE_IR));
        parser_tree_ir_prev                = parser_tree_ir_current;
        parser_tree_ir_current             = parser_tree_ir_current->next_ptr;
        parser_tree_ir_current->prev_ptr   = parser_tree_ir_prev;
        parser_tree_ir_current->next_ptr   = NULL;
    }

    // 原文の保存
    parser_tree_ir_current->str = calloc(strlen(str)+1,1);
    strcpy(parser_tree_ir_current->str, str);

    return 0;
}

int clean_parser_tree_ir()
{
	PARSER_TREE_IR *now_parser_tree_ir;
	PARSER_TREE_IR *new_parser_tree_ir;

    now_parser_tree_ir = parser_tree_ir_top;
    while(1){
        if(now_parser_tree_ir == NULL) break;
        new_parser_tree_ir = now_parser_tree_ir->next_ptr;
		if(now_parser_tree_ir->str != NULL) free(now_parser_tree_ir->str);
        free(now_parser_tree_ir);
        now_parser_tree_ir = new_parser_tree_ir;
    }
    parser_tree_ir_top = NULL;

	if(module_name != NULL){
		free(module_name);
		module_name = NULL;
	}

    return 0;
}

/*!
 * @brief 現在の構文解析木の位置を返す
 */
PARSER_TREE_IR * get_parser_tree_ir_current()
{
    return parser_tree_ir_current;
}

/*!
 * @brief 現在の構文解析木の位置を返す
 */
int parser_tree_ir_current_top()
{
    parser_tree_ir_current = parser_tree_ir_top;
    return 0;
}

/*!
 * LLVM-IRのtypeの取得
 *
 */
int get_irtype_llvm(char *line, char *token)
{
	char *result;
	int level = 0;

	result = calloc(STR_MAX, 1);

	get_token_llvm(line, token);
	strcpy(result, "");

	/*
	 * add/mul演算のみ次の宣言が付く場合がある
	 * nsw: No Signed Wrap
	 * nuw: No UnSigned Wrap
	 */
	if(!strcmp(token, "nsw") || !strcmp(token, "nuw")){
		// nsw, nusについては必要性がわからないため保留
		get_token_llvm(line, token);
	}
	if(!strcmp(token, "nsw") || !strcmp(token, "nuw")){
		// nsw, nusについては必要性がわからないため保留
		get_token_llvm(line, token);
	}

	if(!strcmp(token, "<")){
		// 構造体型
		strcat(result, token);
		level = 1;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strlen(result) > 0)strcat(result, " ");
			strcat(result, token);
			if(!strcmp(token,"<")) level++;
			if(!strcmp(token,">")) level--;
			if(!strcmp(token,">*")) level--;
		}while((strcmp(token,">") && strcmp(token,">*")) || (level != 0));
		strcpy(token, result);
	}else if(!strcmp(token, "[")){
		// 構造体型
		strcat(result, token);
		level = 1;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strlen(result) > 0)strcat(result, " ");
			strcat(result, token);
			if(!strcmp(token,"[")) level++;
			if(!strcmp(token,"]")) level--;
			if(!strcmp(token,"]*")) level--;
		}while((strcmp(token,"]") && strcmp(token,"]*")) || (level != 0));
		strcpy(token, result);
	}else if(!strcmp(token, "{")){
		// 構造体型
		strcat(result, token);
		level = 1;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strlen(result) > 0)strcat(result, " ");
			strcat(result, token);
			if(!strcmp(token,"{")) level++;
			if(!strcmp(token,"}")) level--;
		}while(strcmp(token,"}") || (level != 0));
		strcpy(token, result);
	}else if(!strcmp(token, "getelementptr")){
		// ポインタ
		/*
		 * ToDo:
		 * ポインタはinboundsされていないケースもあるかも
		 */
		strcat(result, token);
		if(strlen(result) > 0)strcat(result, " ");
		// inbounds
		get_token_llvm(line, token);
		if(!strcmp(token, "inbounds")){
			strcat(result, token);
			// type
			get_token_llvm(line, token);
		}
		if(!strcmp(token, "[")){
			strcat(result, token);
			level = 1;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				if(strlen(result) > 0)strcat(result, " ");
				strcat(result, token);
				if(!strcmp(token, "[")) level++;
				if(!strcmp(token, "]")) level--;
			}while(strcmp(token,"]") && strcmp(token,"]*") && (level != 0));
		}else if(!strcmp(token, "(")){
			strcat(result, token);
			level = 1;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				if(strlen(result) > 0)strcat(result, " ");
				strcat(result, token);
				if(!strcmp(token, "(")) level++;
				if(!strcmp(token, ")")) level--;
			}while(strcmp(token,")") && strcmp(token,")*") && (level != 0));
		}else{
			if(strlen(result) > 0)strcat(result, " ");
			strcat(result, token);
		}
		strcpy(token, result);
	}else{
		// 単独のタイプ
		strcat(result, token);
		strcpy(token, result);
	}

	free(result);
	return 0;
}

/*!
 * グローバル構造体からポインタ名を取得する
 */
int get_pointer_llvm(char *line, char *token)
{
	char *result;
	char *line_org;

	result = calloc(STR_MAX,1 );

	line_org = charalloc(line);

	// getelementptr
	get_token_llvm(line_org, token);
	strcpy(result, "");

	// inbounds
	get_token_llvm(line_org, token);
	strcpy(result, "");

	if(!strcmp(token, "inbounds")){
		// inboundsは無視して良い
		// "("
		get_token_llvm(line_org, token);
	}else{
		/*
		 * %x,@xの場合
		 */
		strcpy(result, token);
		strcpy(token, result);

		free(line_org);
		return 0;
	}

	// Type
	get_irtype_llvm(line_org, token);
	// Name
	get_token_llvm(line_org, token);
	strcpy(result, token);
	strcpy(token, result);

	free(line_org);
	free(result);

	return 0;
}

/*!
 * @brief	LLVM-IRの解析
 */
int parser_ir_source(char *buf)
{
    char *token;
    char *line_buf;
    char *line;
    int level = 0;

	token = calloc(STR_MAX, 1);
	line = calloc(strlen(buf)+1,1);
	strcpy(line, buf);

	get_token_llvm(line, token);
	// 何もなければ何もしない
	if(!strcmp(token, "") || !strcmp(token, "\n") || !strcmp(token, "\r")) return 0;

    insert_parser_tree_ir(token);

	if(!strcmp(token, ";")){
		// 宣言
		get_token_llvm(line, token);

		if(!strcmp(token, "<")){
			// ラベル名
			get_token_llvm(line, token);	// "label"
			if(strcmp(token, "label")){
				// "label"でなければいけない
				printf("[ERROR] unknown label\n");
				parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
				free(line);
				return 0;
			}

			get_token_llvm(line, token);	// ">"
			if(strcmp(token, ">")){
				// ">"でなければいけない
				printf("[ERROR] unknown label\n");
				parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
				free(line);
				return 0;
			}

			get_token_llvm(line, token);	// ":"
			if(strcmp(token, ":")){
				// ":"でなければいけない
				printf("[ERROR] unknown label\n");
				parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
				free(line);
				return 0;
			}

			get_token_llvm(line, token);	// ラベル名

			// ラベル名の保存
			strcpy(current_ir_label, token);
			strcpy(parser_tree_ir_current->label, token);
			parser_tree_ir_current->flag = PARSER_IR_FLAG_LABEL;

			// これ以降は無視
		}else if(!strcmp(token, "Function")){
			// Function
			// これは無視
			parser_tree_ir_current->flag = PARSER_IR_FLAG_NONE;
		}else if(!strcmp(token, "ModuleID")){
			// Function
			// これは無視
			parser_tree_ir_current->flag = PARSER_IR_FLAG_NONE;
		}else{
			printf("[ERROR] unknown label\n");
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
		}
	}else if(!strcmp(token, "define")){
		// モジュール宣言
		/*
		 * ToDo:
		 * エラー処理を入れること
		 */
		parser_tree_ir_current->flag = PARSER_IR_FLAG_MODULE;
		get_irtype_llvm(line, token);	// 戻り値タイプ
		strcpy(parser_tree_ir_current->module.result_type, token);
		get_token_llvm(line, token);	// モジュール名
		strcpy(parser_tree_ir_current->module.name, token);

		module_name = calloc(strlen(token)+1,1);
		strcpy(module_name, token);

		// 引数取得
		level = 0;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strcmp(token,")") || level != 0){
				strcat(parser_tree_ir_current->module.argument, token);
				strcat(parser_tree_ir_current->module.argument, " ");
				if(!strcmp(token, "(")) level++;
				if(!strcmp(token, ")")) level--;
			}
		}while(strcmp(token,")") || level != 0);

		// 無視する部分
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strcmp(token,"{")){
				// この部分は無視する
			}
		}while(strcmp(token,"{"));

		if(!strcmp(token, "{")){
			// レベルを1つ上げる
		}
	}else if(!strcmp(token, "store")){
		// store命令
		strcat(parser_tree_ir_current->label, token);
		parser_tree_ir_current->flag = PARSER_IR_FLAG_STORE;

		// 保存元タイプ
		get_irtype_llvm(line, token);
		strcat(parser_tree_ir_current->reg.input_left_type, token);

		// 保存元名
		level = 0;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strcmp(token,",") || level != 0){
				if(strlen(parser_tree_ir_current->reg.input_left) > 0)strcat(parser_tree_ir_current->reg.input_left, " ");
				strcat(parser_tree_ir_current->reg.input_left, token);
				if(!strcmp(token, "(")) level++;
				if(!strcmp(token, ")")) level--;
			}
		}while(strcmp(token,",") || level != 0);

		// 保存先タイプ
		get_irtype_llvm(line, token);
		strcat(parser_tree_ir_current->reg.input_right_type, token);

		// 保存先名
		level = 0;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strcmp(token,",") || level != 0){
				if(strlen(parser_tree_ir_current->reg.input_right) > 0)strcat(parser_tree_ir_current->reg.input_right, " ");
				strcat(parser_tree_ir_current->reg.input_right, token);
				if(!strcmp(token, "(")) level++;
				if(!strcmp(token, ")")) level--;
			}
		}while(strcmp(token,",") || level != 0);

		// storeのラベルを保存しておく
		strcpy(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_right);

		// align
		get_token_llvm(line, token);
		// align数
		get_token_llvm(line, token);
		// これ以降は無視
	}else if(!strcmp(token, "br")){
		// ブランチ
		parser_tree_ir_current->flag = PARSER_IR_FLAG_BRANCH;

		get_token_llvm(line, token);
		if(!strcmp(token, "label")){
			// labelがあれば、無条件ブランチ
			strcpy(parser_tree_ir_current->branch.condition_type, "none");
			strcpy(parser_tree_ir_current->branch.condition_value, "none");
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->branch.branch_true, token);
			free(line);
			return 0;
		}

		// 無条件ブランチでなければ
		// タイプ
		strcpy(parser_tree_ir_current->branch.condition_type, token);
		// 変数
		get_token_llvm(line, token);
		strcpy(parser_tree_ir_current->branch.condition_value, token);
		// ,
		get_token_llvm(line, token);
		// label
		get_token_llvm(line, token);
		// ジャンプ先
		get_token_llvm(line, token);
		strcpy(parser_tree_ir_current->branch.branch_true, token);
		// ,
		get_token_llvm(line, token);
		// label
		get_token_llvm(line, token);
		// ジャンプ先
		get_token_llvm(line, token);
		strcpy(parser_tree_ir_current->branch.branch_false, token);
	}else if(!strcmp(token, "ret")){
		// リターン
		parser_tree_ir_current->flag = PARSER_IR_FLAG_RETURN;

		// 戻り値
		get_token_llvm(line, token);
		if(!strcmp(token, "void")){
			strcat(parser_tree_ir_current->ret.name, token);
		}
	}else if(!strcmp(token, "tail") || !strcmp(token, "call")){
		// 外部モジュール呼び出し
		// void関数のcallはレジスタ代入がない
		parser_tree_ir_current->flag = PARSER_IR_FLAG_CALL;
		if(!strcmp(token, "tail")){
			get_token_llvm(line, token);
			if(strcmp(token, "call")){
				// tailの後にcallが無ければ未知の命令と判断する
				printf("[ERROR] unknown call\n");
				parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
				free(line);
				return 0;
			}
		}

		// 戻り値検出
		get_irtype_llvm(line, token);

		// 呼び出し名
		get_irtype_llvm(line, token);
		if(!strcmp(token, "bitcast")){
			level = 0;
			do{
				get_irtype_llvm(line, token);
				if(token[0] == '@') strcpy(parser_tree_ir_current->call.name, token);
				if(!strcmp(token, "(")) level++;
				if(!strcmp(token, ")")) level--;
			}while(strcmp(token,")") || level != 0);
			strcpy(token, parser_tree_ir_current->call.name);
		}

		strcpy(parser_tree_ir_current->call.name, token);	// 呼び出し名

		// 引数取得
		level = 0;
		do{
			get_token_llvm(line, token);
			if(!strcmp(token, "")) break;
			if(strcmp(token,")") || level != 0){
				strcat(parser_tree_ir_current->call.argument, token);
				strcat(parser_tree_ir_current->call.argument, " ");
				if(!strcmp(token, "(")) level++;
				if(!strcmp(token, ")")) level--;
			}
		}while(strcmp(token,")") || level != 0);
	}else if(token[0] == '%'){
		// レジスタ
		strcpy(parser_tree_ir_current->label, token);

		get_token_llvm(line, token);
		if(strcmp(token, "=")){
			printf("[ERROR] unknown pointer\n");
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
			return 0;
		}

		// 命令
		get_token_llvm(line, token);

		if(
			!strcmp(token, "add") ||
			!strcmp(token, "sub") ||
			!strcmp(token, "mul") ||
			!strcmp(token, "and") ||
			!strcmp(token, "or") ||
			!strcmp(token, "xor") ||
			!strcmp(token, "srem")
		){
			// 算術演算/論理演算
			parser_tree_ir_current->flag = PARSER_IR_FLAG_REGISTER;
			strcpy(parser_tree_ir_current->reg.name, token);

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left_type, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left, token);
			// ,
			get_token_llvm(line, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_right, token);

			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_left_type, SIGNAL_FLAG_REG);
		}else if(
			!strcmp(token, "sdiv")
		){
			// 除算命令
			/*
			 * 除算はスペシャル処理
			 */
			parser_tree_ir_current->flag = PARSER_IR_FLAG_SCALL;
			strcpy(parser_tree_ir_current->reg.name, token);

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left_type, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left, token);
			// ,
			get_token_llvm(line, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_right, token);

			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_left_type, SIGNAL_FLAG_REG);
		}else if(
			!strcmp(token, "shl") ||
			!strcmp(token, "lshl") ||
			!strcmp(token, "lshr") ||
			!strcmp(token, "ashr")
		){
			// 算術演算/論理演算
			parser_tree_ir_current->flag = PARSER_IR_FLAG_WIRE;
			strcpy(parser_tree_ir_current->reg.name, token);

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left_type, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left, token);
			// ,
			get_token_llvm(line, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_right, token);

			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_left_type, SIGNAL_FLAG_WIRE);
		}else if(
			!strcmp(token, "fadd") ||
			!strcmp(token, "fsub") ||
			!strcmp(token, "fmul")
		){
			// 浮動小数点演算
			parser_tree_ir_current->flag = PARSER_IR_FLAG_FLOAT;
			strcpy(parser_tree_ir_current->reg.name, token);

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left_type, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left, token);
			// ,
			get_token_llvm(line, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_right, token);
		}else if(!strcmp(token, "icmp")){
			// 比較
			parser_tree_ir_current->flag = PARSER_IR_FLAG_COMPARE;
			strcpy(parser_tree_ir_current->comp.name, token);

			// 条件
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->comp.value, token);

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->comp.input_left_type, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->comp.input_left, token);
			// ,
			get_token_llvm(line, token);
			// 変数/値
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->comp.input_right, token);

			register_signal_tree(parser_tree_ir_current->label, "i1", SIGNAL_FLAG_REG);
		}else if(
			!strcmp(token, "bitcast") ||
			!strcmp(token, "sext") ||
			!strcmp(token, "trunc") ||
			!strcmp(token, "zext") ||
			!strcmp(token, "sitofp") ||
			!strcmp(token, "fptoui")
		){
			// 型変換
			/*
			 * 型変換はワイヤで対応する
			 * sext:  ビット拡張 -> $signed()
			 * trunc: ビット削減
			 */
//			parser_tree_ir_current->flag = PARSER_IR_FLAG_WIRE;
			parser_tree_ir_current->flag = PARSER_IR_FLAG_POINTER;
			strcpy(parser_tree_ir_current->reg.name, token);

			// 変換元タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left_type, token);
			// 変換元
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_left, token);
			strcpy(parser_tree_ir_current->pointer.str, token);
			// to
			get_token_llvm(line, token);
			if(strcmp(token, "to")){
				printf("[ERROR] unknown bitcast\n");
				parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
				return 0;
			}
			// 変換先タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->reg.input_right_type, token);

//			printf("[%s] ", parser_tree_ir_current->reg.name);

			// ポインター登録
/*
			if(is_pointer_type(parser_tree_ir_current->reg.input_left_type)){
				// 代入するのはポインターなのでポインター登録を行う
				register_pointer_tree(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_left);
			}
*/
//			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_left_type, SIGNAL_FLAG_REG);
			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->reg.input_left_type, SIGNAL_FLAG_WIRE);
		}else if(!strcmp(token, "type")){
			// 構造体宣言
			/*
			 * 構造体宣言は現時点では無視している。
			 * その理由は、ポインタ宣言では相対アドレスが示されているから。
			 */
			parser_tree_ir_current->flag = PARSER_IR_FLAG_TYPE;

			// 引数取得
			level = 0;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				strcat(parser_tree_ir_current->type.name, token);
				strcat(parser_tree_ir_current->type.name, " ");
				if(!strcmp(token, "{")) level++;
				if(!strcmp(token, "}")) level--;
			}while(strcmp(token,"}") || level != 0);

		}else if(!strcmp(token, "load")){
			// ロード
			parser_tree_ir_current->flag = PARSER_IR_FLAG_LOAD;

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->load.type, token);
			// ポインタ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->load.name, token);
			// ,
			get_token_llvm(line, token);
			// align
			get_token_llvm(line, token);
			// align値
			get_token_llvm(line, token);

			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->load.type, SIGNAL_FLAG_REG);

			if(!strcmp(&parser_tree_ir_current->load.type[strlen(parser_tree_ir_current->load.type) -2], "**")){
				/*
				 * LOADしたデータはポインターである可能性が非常に大きいのでポインターとして登録する
				 */
//				 register_pointer_tree(parser_tree_ir_current->label, parser_tree_ir_current->load.name);
			}
		}else if(!strcmp(token, "phi")){
			// ループ系
			/*
			 * これが意外と厄介
			 */
			parser_tree_ir_current->flag = PARSER_IR_FLAG_REGISTER_LOOP;
			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->phi.type, token);
			// 引数を登録
			strcpy(parser_tree_ir_current->phi.argument, line);
			/*
			 * ポインターの登録
			 * ToDo:
			 *   暫定処理：ラベル名そのものを登録している
			 *   ポインターの格納領域はレジスタとしたいが、LLVM-IRの時点でポインターになるケースがある。
			 */
			if(is_pointer_type(parser_tree_ir_current->phi.type)){
//				register_pointer_tree(parser_tree_ir_current->label, parser_tree_ir_current->label);
			}

			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->phi.type, SIGNAL_FLAG_REG);
		}else if(!strcmp(token, "getelementptr")){
			// ポインタ
			//   %49 = getelementptr inbounds %struct.bitstring* %agg.result, i32 0, i32 0
			parser_tree_ir_current->flag = PARSER_IR_FLAG_POINTER;
			// 一旦、復元してからタイプの取得をする

			line_buf = calloc(strlen(token)+strlen(line)+1,1);
			sprintf(line_buf, "%s%s", token, line);
			strcpy(parser_tree_ir_current->pointer.str, line_buf);
			// タイプ
			get_irtype_llvm(line_buf, token);
			strcpy(parser_tree_ir_current->pointer.type, token);
			// 変数
			get_irtype_llvm(line_buf, token);
			strcpy(parser_tree_ir_current->pointer.name, token);

			// ,
			get_token_llvm(line_buf, token);
			// タイプ(基数)
			get_irtype_llvm(line_buf, token);
			strcpy(parser_tree_ir_current->pointer.init_type, token);
			// 変数(基数)
			get_irtype_llvm(line_buf, token);
			strcpy(parser_tree_ir_current->pointer.init_name, token);
			// ,
			get_token_llvm(line_buf, token);
			// ポインタの位置が階層になっている場合は検出する
			do{
				// タイプ
				get_irtype_llvm(line_buf, token);
				strcat(parser_tree_ir_current->pointer.add_name, token);
				strcat(parser_tree_ir_current->pointer.add_name, " ");
				// 変数
				get_irtype_llvm(line_buf, token);
				strcat(parser_tree_ir_current->pointer.add_name, token);
				// ,
				get_token_llvm(line_buf, token);
				if(!strcmp(token, ",")){
					strcat(parser_tree_ir_current->pointer.add_name, token);
				}
			}while(!strcmp(token, ",") && (strlen(line_buf) > 0));
			/*
			 * どのように処理するか？
			 */

			 free(line_buf);

//			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->pointer.init_type, SIGNAL_FLAG_REG);
			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->pointer.init_type, SIGNAL_FLAG_WIRE);
		}else if(!strcmp(token, "alloca")){
			/*
			 * 領域取得
			 */
			parser_tree_ir_current->flag = PARSER_IR_FLAG_ALLOCA;
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->alloca.type, token);
			/*
			 * ポインター登録
			 */
			strcpy(token, parser_tree_ir_current->label);

//			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->alloca.type, SIGNAL_FLAG_RAM);
			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->alloca.type, SIGNAL_FLAG_ALLOCA);
		}else if(!strcmp(token, "select")){
			// セレクト
			parser_tree_ir_current->flag = PARSER_IR_FLAG_SELECT;

			// タイプ
			get_irtype_llvm(line, token);
			strcpy(parser_tree_ir_current->select.condition_type, token);
			// 変数
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->select.condition_value, token);
			// ,
			get_token_llvm(line, token);
			// タイプ
			get_irtype_llvm(line, token);
			// True時の入力変数
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->select.select_true, token);
			// ,
			get_token_llvm(line, token);
			// タイプ
			get_irtype_llvm(line, token);
			// False時の入力変数
			get_token_llvm(line, token);
			strcpy(parser_tree_ir_current->select.select_false, token);

			register_signal_tree(parser_tree_ir_current->label, parser_tree_ir_current->label, SIGNAL_FLAG_REG);
		}else if(!strcmp(token, "tail") || !strcmp(token, "call")){
			// 外部モジュール呼び出し
			// 戻り値があるcallはここに入る
			parser_tree_ir_current->flag = PARSER_IR_FLAG_CALL;
			if(!strcmp(token, "tail")){
				get_token_llvm(line, token);
				if(strcmp(token, "call")){
					// tailの後にcallが無ければ未知の命令と判断する
					printf("[ERROR] unknown call\n");
					parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
					free(line);
					return 0;
				}
			}

			// 戻り値検出
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				if(token[0] != '@'){
					strcat(parser_tree_ir_current->call.result_type, token);
					strcat(parser_tree_ir_current->call.result_type, " ");
				}
			}while(token[0] != '@');

			strcpy(parser_tree_ir_current->call.name, token);	// 呼び出し名

			// 引数取得
			level = 0;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				if(strcmp(token,")") || level != 0){
					strcat(parser_tree_ir_current->call.argument, token);
					strcat(parser_tree_ir_current->call.argument, " ");
					if(!strcmp(token, "(")) level++;
					if(!strcmp(token, ")")) level--;
				}
			}while(strcmp(token,")") || level != 0);
		}else{
			printf("[ERROR] unknown call\n");
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
			free(line);
			return 0;
		}

		/*
		 * ToDo: 命令セットごとの構文解析を入れる
		 */

	}else if(token[0] == '@'){
		// 外部変数又はcall宣言
		parser_tree_ir_current->flag = PARSER_IR_FLAG_ARRAY;
		strcat(parser_tree_ir_current->label, token);

		get_token_llvm(line, token);
		if(strcmp(token, "=")){
			printf("[ERROR] unknown instruction\n");
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
			free(line);
			return 0;
		}

		get_token_llvm(line, token);
		if(strcmp(token, "external") && strcmp(token, "internal")){
			printf("[ERROR] unknown instruction\n");
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
			free(line);
			return 0;
		}

		get_token_llvm(line, token);
		if(strcmp(token, "global")){
			printf("[ERROR] unknown instruction\n");
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
			free(line);
			return 0;
		}

		// 外部変数名
		get_irtype_llvm(line, token);
		strcat(parser_tree_ir_current->array.type, token);
		if(!strcmp(token, "[")){
			level = 1;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				strcat(parser_tree_ir_current->call.argument, token);
				strcat(parser_tree_ir_current->call.argument, " ");
				if(!strcmp(token, "[")) level++;
				if(!strcmp(token, "]")) level--;
			}while(strcmp(token,"]") || level != 0);
		}

		// internalの場合は初期値がある
		get_irtype_llvm(line, token);
		strcpy(parser_tree_ir_current->array.name, token);
		if(!strcmp(token, "c")){
			// "\""
			get_token_llvm(line, token);
			strcat(parser_tree_ir_current->array.name, token);
			strcat(parser_tree_ir_current->array.name, " ");
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				strcat(parser_tree_ir_current->array.name, token);
				strcat(parser_tree_ir_current->array.name, " ");
			}while(strcmp(token,"\""));
		}else if(!strcmp(token, "[")){
			level = 1;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				strcat(parser_tree_ir_current->array.name, token);
				strcat(parser_tree_ir_current->array.name, " ");
				if(!strcmp(token, "[")) level++;
				if(!strcmp(token, "]")) level--;
			}while(strcmp(token,"]") || level != 0);
		}else if(!strcmp(token, "{")){
			level = 1;
			do{
				get_token_llvm(line, token);
				if(!strcmp(token, "")) break;
				strcat(parser_tree_ir_current->array.name, token);
				strcat(parser_tree_ir_current->array.name, " ");
				if(!strcmp(token, "{")) level++;
				if(!strcmp(token, "}")) level--;
			}while(strcmp(token,"}") || level != 0);
		}
	}else if(!strcmp(token, "}")){
		// モジュールの締め
		/*
		 * ToDo: モジュール締めの処理
		 */
		parser_tree_ir_current->flag = PARSER_IR_FLAG_CLOSE;
	}else if(token[0] == '.'){
		// ラベル
		// ラベル名の保存
		strcat(current_ir_label, token);
		strcat(parser_tree_ir_current->label, token);
		parser_tree_ir_current->flag = PARSER_IR_FLAG_LABEL;
		// これ以降は無視
	}else if(token[0] == '!'){
		// 文字列宣言
		// これは無視
		parser_tree_ir_current->flag = PARSER_IR_FLAG_NONE;
	}else if(!strcmp(token, "declare")){
		// extern宣言のようなもの
		// declareは無視する
		parser_tree_ir_current->flag = PARSER_IR_FLAG_NONE;
	}else if(!strcmp(token, "target")){
		// 宣言
		// これは無視
		parser_tree_ir_current->flag = PARSER_IR_FLAG_NONE;
	}else if(!strcmp(token, "attributes")){
		// 宣言
		// これは無視
		parser_tree_ir_current->flag = PARSER_IR_FLAG_NONE;
	}else{
		// ラベルの可能性
		// ラベル名の保存
		strcat(current_ir_label, token);
		strcat(parser_tree_ir_current->label, token);
		get_token_llvm(line, token);
		if(!strcmp(token, ":")){
			parser_tree_ir_current->flag = PARSER_IR_FLAG_LABEL;
		}else{
			printf("[ERROR] unknown(%s)\n", token);
			parser_tree_ir_current->flag = PARSER_IR_FLAG_UNKNOWN;
		}
	}

	free(token);
	free(line);

    return 0;
}
/*
char *sep_p(char *name){
	if(name[0] == '%'){
printf("%s\n", &name[1]);
		return &name[1];
	}else{
		return name;
	}
}
*/
/*!
 * @brief	ラベル名からステージ番号を取得する
 *
 */
int get_stage_label(char *label)
{
	PARSER_TREE_IR *now_parser_tree_ir;

    now_parser_tree_ir = parser_tree_ir_top;
    while(now_parser_tree_ir != NULL){
		/*
		 * 本当はフラグも一緒に判定しておかなかければいけないかもしれない
		 */
		if(now_parser_tree_ir->flag == PARSER_IR_FLAG_LABEL){
			if(!strcmp(now_parser_tree_ir->label, sep_p(label))){
				return now_parser_tree_ir->stage;
			}
		}
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
	}

	return -1;
}


int print_parser_tree_ir(FILE *fp)
{
	PARSER_TREE_IR *now_parser_tree_ir;

    fprintf(fp,"==============================\n");
    fprintf(fp,"Print - LLVM-IR\n");
    fprintf(fp,"==============================\n");
    now_parser_tree_ir = parser_tree_ir_top;
    while(now_parser_tree_ir != NULL){

		switch(now_parser_tree_ir->flag){
			case PARSER_IR_FLAG_REGISTER:
				fprintf(fp,"REGISTER(%d_%d): ", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"%s ", now_parser_tree_ir->reg.input_left_type);
				fprintf(fp,"%s = ", now_parser_tree_ir->label);
				fprintf(fp,"%s ", now_parser_tree_ir->reg.input_left);
				fprintf(fp,"%s ", now_parser_tree_ir->reg.name);
				fprintf(fp,"%s\n", now_parser_tree_ir->reg.input_right);

				break;
			case PARSER_IR_FLAG_WIRE:
				fprintf(fp,"WIRE(%d_%d):     ", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"%s ", now_parser_tree_ir->reg.input_left_type);
				fprintf(fp,"%s = ", now_parser_tree_ir->label);
				fprintf(fp,"%s ", now_parser_tree_ir->reg.name);
				fprintf(fp,"%s ", now_parser_tree_ir->reg.input_left);
				fprintf(fp,"(%s)\n", now_parser_tree_ir->reg.input_right);

				break;
			case PARSER_IR_FLAG_CALL:
				fprintf(fp,"CALL\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  reg:    %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  name:   %s\n", now_parser_tree_ir->call.name);
				fprintf(fp,"  result: %s\n", now_parser_tree_ir->call.result_type);
				fprintf(fp,"  args:   %s\n", now_parser_tree_ir->call.argument);

				break;
			case PARSER_IR_FLAG_BRANCH:
				fprintf(fp,"BRANCH\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  label:  %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  cond:   %s\n", now_parser_tree_ir->branch.condition_type);
				fprintf(fp,"  value:  %s\n", now_parser_tree_ir->branch.condition_value);
				fprintf(fp,"  true:   %s\n", now_parser_tree_ir->branch.branch_true);
				fprintf(fp,"  false:  %s\n", now_parser_tree_ir->branch.branch_false);

				break;
			case PARSER_IR_FLAG_LABEL:
				fprintf(fp,"LABEL(%d_%d):    ", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"%s\n", now_parser_tree_ir->label);

				break;
			case PARSER_IR_FLAG_MODULE:
				fprintf(fp,"MODULE\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  name:   %s\n", now_parser_tree_ir->module.name);
				fprintf(fp,"  result: %s\n", now_parser_tree_ir->module.result_type);
				fprintf(fp,"  args:   %s\n", now_parser_tree_ir->module.argument);

				break;
			case PARSER_IR_FLAG_REGISTER_LOOP:
				fprintf(fp,"REGISTER LOOP\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  loop:   %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  type:   %s\n", now_parser_tree_ir->phi.type);
				fprintf(fp,"  init:   %s\n", now_parser_tree_ir->phi.argument);
//				fprintf(fp,"  init:   %s\n", now_parser_tree_ir->phi.init);
//				fprintf(fp,"  next1:  %s\n", now_parser_tree_ir->phi.next1);
				break;
			case PARSER_IR_FLAG_FLOAT:
				fprintf(fp,"FLOAT\n");
				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);
				fprintf(fp,"  reg:   %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  inst:  %s\n", now_parser_tree_ir->reg.name);
				fprintf(fp,"  type:  %s\n", now_parser_tree_ir->reg.input_left_type);
				fprintf(fp,"  left:  %s\n", now_parser_tree_ir->reg.input_left);
				fprintf(fp,"  right: %s\n", now_parser_tree_ir->reg.input_right);
				break;
			case PARSER_IR_FLAG_TYPE:
				fprintf(fp,"TYPE\n");
				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);
				fprintf(fp,"  name:  %s\n", now_parser_tree_ir->type.name);
				break;
			case PARSER_IR_FLAG_STORE:
				fprintf(fp,"STORE\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  %s -> %s\n", now_parser_tree_ir->reg.input_left_type, now_parser_tree_ir->reg.input_right_type);
				fprintf(fp,"  In:  %s\n", now_parser_tree_ir->reg.input_left);
				fprintf(fp,"  Out: %s\n", now_parser_tree_ir->reg.input_right);

				break;
			case PARSER_IR_FLAG_ARRAY:
				fprintf(fp,"ARRAY\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  array: %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  type:  %s\n", now_parser_tree_ir->array.type);
				fprintf(fp,"  name:  %s\n", now_parser_tree_ir->array.name);

				break;
			case PARSER_IR_FLAG_COMPARE:
				fprintf(fp,"COMPARE\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  label:  %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  name:   %s\n", now_parser_tree_ir->comp.name);
				fprintf(fp,"  value:  %s\n", now_parser_tree_ir->comp.value);
				fprintf(fp,"  type:   %s\n", now_parser_tree_ir->comp.input_left_type);
				fprintf(fp,"  left:   %s\n", now_parser_tree_ir->comp.input_left);
				fprintf(fp,"  right:  %s\n", now_parser_tree_ir->comp.input_right);

				break;
			case PARSER_IR_FLAG_POINTER:
				fprintf(fp,"POINTER\n");
				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);
				fprintf(fp,"  label:  %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  type:   %s\n", now_parser_tree_ir->pointer.str);
				fprintf(fp,"  type:   %s\n", now_parser_tree_ir->pointer.type);
				fprintf(fp,"  name:   %s\n", now_parser_tree_ir->pointer.name);
				fprintf(fp,"  itype:  %s\n", now_parser_tree_ir->pointer.init_type);
				fprintf(fp,"  iname:  %s\n", now_parser_tree_ir->pointer.init_name);
				fprintf(fp,"  atype:  %s\n", now_parser_tree_ir->pointer.add_type);
				fprintf(fp,"  aname:  %s\n", now_parser_tree_ir->pointer.add_name);
				break;
			case PARSER_IR_FLAG_LOAD:
				fprintf(fp,"LOAD\n");

				fprintf(fp,"  stage: %d_%d\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);

				fprintf(fp,"  name:    %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  type:    %s\n", now_parser_tree_ir->load.type);
				fprintf(fp,"  pointer: %s\n", now_parser_tree_ir->load.name);

				break;
			case PARSER_IR_FLAG_SELECT:
				fprintf(fp,"SELECT\n");
				fprintf(fp,"  name:    %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  cond:    %s\n", now_parser_tree_ir->select.condition_type);
				fprintf(fp,"  value:   %s\n", now_parser_tree_ir->select.condition_value);
				fprintf(fp,"  true:    %s\n", now_parser_tree_ir->select.select_true);
				fprintf(fp,"  false:   %s\n", now_parser_tree_ir->select.select_false);
				break;
			case PARSER_IR_FLAG_ALLOCA:
				fprintf(fp,"ALLOCA\n");
				fprintf(fp,"  name:    %s\n", now_parser_tree_ir->label);
				fprintf(fp,"  type:    %s\n", now_parser_tree_ir->alloca.type);
				break;
			case PARSER_IR_FLAG_RETURN:
				fprintf(fp,"RETURN(%d_%d)\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);
				break;
			case PARSER_IR_FLAG_CLOSE:
				fprintf(fp,"CLOSE(%d_%d)\n", now_parser_tree_ir->parallel, now_parser_tree_ir->stage);
				break;
			case PARSER_IR_FLAG_NONE:
//				fprintf(fp,"None\n");
				break;
			case PARSER_IR_FLAG_UNKNOWN:
				fprintf(fp,"Unknown(error)\n");
				break;
			default:
				fprintf(fp,"Unknown(can't parser: %d)\n", now_parser_tree_ir->flag);
				break;
        }
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
    }
    fprintf(fp,"==============================\n");

    return 0;
}

/*!
 * 関数宣言のargsの中から変数を探す
 */
int search_module_llvm(char *name, char *args)
{
	char *line;
	char *token;

	token = calloc(STR_MAX, 1);
	line = charalloc(args);

	while(strcmp(line, "")){
		get_token_llvm(line, token);

		if(!strcmp(token, name)){
			free(line);
			return 0;
		}
	}
	free(token);
	free(line);

	return -1;
}

/*!
 * レジスタの位置を探す
 */
INST_STAGE search_reg_llvm(PARSER_TREE_IR *now_parser_tree_ir, char *name, char *left_name, char *right_name)
{
    now_parser_tree_ir = now_parser_tree_ir->prev_ptr;
	INST_STAGE inst_stage;

	int left_stage = 0;
	int left_parallel = 0;
	int right_stage = 0;
	int right_parallel = 0;

	inst_stage.parallel = 0;
	inst_stage.stage = 0;

	while(now_parser_tree_ir != parser_tree_ir_top){

		switch(now_parser_tree_ir->flag){
			case PARSER_IR_FLAG_MODULE:
				if(!search_module_llvm(left_name, now_parser_tree_ir->module.argument)){
					left_parallel = now_parser_tree_ir->parallel;
					left_stage = now_parser_tree_ir->stage;
					break;
				}
			case PARSER_IR_FLAG_REGISTER:
			case PARSER_IR_FLAG_LOAD:
			case PARSER_IR_FLAG_FLOAT:
			case PARSER_IR_FLAG_WIRE:
			case PARSER_IR_FLAG_CALL:
			case PARSER_IR_FLAG_ALLOCA:
			case PARSER_IR_FLAG_COMPARE:
			case PARSER_IR_FLAG_POINTER:
			case PARSER_IR_FLAG_SELECT:
			case PARSER_IR_FLAG_TYPE:
			case PARSER_IR_FLAG_REGISTER_LOOP:
				if(!strcmp(now_parser_tree_ir->label, left_name) && (strlen(left_name) > 0) && (left_name[0] == '%')){
					left_parallel = now_parser_tree_ir->parallel;
					left_stage = now_parser_tree_ir->stage;
					break;
				}
				if(!strcmp(now_parser_tree_ir->label, right_name) && (strlen(right_name) > 0) && (right_name[0] == '%')){
					right_parallel = now_parser_tree_ir->parallel;
					right_stage = now_parser_tree_ir->stage;
					break;
				}
			default:
				break;
		}
        now_parser_tree_ir = now_parser_tree_ir->prev_ptr;
	}

	if(left_stage > right_stage){
		inst_stage.parallel = left_parallel;
		inst_stage.stage = left_stage;
	}else if(right_stage > left_stage){
		inst_stage.parallel = right_parallel;
		inst_stage.stage = right_stage;
	}else{
		inst_stage.parallel = left_parallel;
		inst_stage.stage = left_stage;
	}

	return inst_stage;
}

/*!
 * 関数の引数から変数を検索する
 */
INST_STAGE search_args_llvm(PARSER_TREE_IR *now_parser_tree_ir, char *name, char *args)
{
	char *line;
	char *token;
	int max_parallel = 0;
	int max_stage = 0;
	INST_STAGE inst_stage;

	token = calloc(STR_MAX, 1);
	line = charalloc(args);

	// "("
	get_token_llvm(line, token);

	while(strlen(line) > 0){
		// タイプ
		get_irtype_llvm(line, token);
		if(!strcmp(token, "")) break;
		// 変数名
		get_token_llvm(line, token);
		if(!strcmp(token, "zeroext")) get_token_llvm (line, token);
		inst_stage = search_reg_llvm(now_parser_tree_ir, name ,token , "");
		if((inst_stage.parallel > max_parallel) || (inst_stage.stage > max_stage)){
			max_parallel = inst_stage.parallel;
			max_stage = inst_stage.stage;
		}
		// ","
		get_token_llvm(line, token);
	}

	free(line);

	inst_stage.stage = max_stage;
	inst_stage.parallel = max_parallel;

	return inst_stage;
}

/*!
 * ステージが一番進んでいるプロセスを探す
 */
INST_STAGE search_proc_llvm(PARSER_TREE_IR *now_parser_tree_ir, int flag)
{
    now_parser_tree_ir = now_parser_tree_ir->prev_ptr;

	INST_STAGE inst_stage;

	inst_stage.parallel = 0;
	inst_stage.stage = 0;

	while(now_parser_tree_ir != parser_tree_ir_top){
		if(
			(now_parser_tree_ir->flag == PARSER_IR_FLAG_CALL) ||
			(now_parser_tree_ir->flag == PARSER_IR_FLAG_SCALL) ||
			(now_parser_tree_ir->flag == PARSER_IR_FLAG_STORE) ||
			(now_parser_tree_ir->flag == PARSER_IR_FLAG_LOAD) ||
			(now_parser_tree_ir->flag == PARSER_IR_FLAG_LABEL)
		){
			inst_stage.parallel = now_parser_tree_ir->parallel;
			inst_stage.stage = now_parser_tree_ir->stage;
			break;
		}

        now_parser_tree_ir = now_parser_tree_ir->prev_ptr;
	}

	return inst_stage;
}

/*!
 * 現在、一番最後のステージを探す
 */
INST_STAGE search_maxstage_llvm(PARSER_TREE_IR *now_parser_tree_ir)
{
    now_parser_tree_ir = now_parser_tree_ir->prev_ptr;

	INST_STAGE inst_stage;

	inst_stage.parallel = 0;
	inst_stage.stage = 0;

	while(now_parser_tree_ir != parser_tree_ir_top){
		if(inst_stage.stage < now_parser_tree_ir->stage){
//			inst_stage.parallel = now_parser_tree_ir->parallel;
			inst_stage.stage = now_parser_tree_ir->stage;
			break;
		}
        now_parser_tree_ir = now_parser_tree_ir->prev_ptr;
	}

	return inst_stage;
}


/*!
 * LLVM-IR 構文解析(ファースト・ステージ)
 *
 * このステージでは、モジュールのステージング解析を行う。
 */

int create_stage_parser_tree()
{
	PARSER_TREE_IR *now_parser_tree_ir;
    char *token;
	int current_parallel = 0;
	int current_stage = 0;
	int new_stage = 0;
	INST_STAGE inst_stage;
	INST_STAGE proc_stage;
    char *line_buf;
    char *init_name, *next_name;

	token = calloc(STR_MAX, 1);
	init_name = calloc(STR_MAX, 1);
	next_name = calloc(STR_MAX, 1);

    printf(" -> Create Stage for LLVM-IR\n");
    now_parser_tree_ir = parser_tree_ir_top;
    while(now_parser_tree_ir != NULL){

		if( current_stage == 0 ){
			if( now_parser_tree_ir->flag == PARSER_IR_FLAG_MODULE ){
				/*
				 * モジュール宣言に入った時にステージが初めてスタートします。
				 */
				current_stage = 1;
				current_parallel = 0;
			}
			now_parser_tree_ir->parallel = current_parallel;
			now_parser_tree_ir->stage = current_stage;
		}else{
			switch(now_parser_tree_ir->flag){
				case PARSER_IR_FLAG_MODULE:
//					printf("[ERROR] Stage Level for Module Start : parser_tree_1st_stage().\n");
					break;
				case PARSER_IR_FLAG_REGISTER:
				case PARSER_IR_FLAG_FLOAT:
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->reg.input_left, now_parser_tree_ir->reg.input_right);
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}
					current_stage = inst_stage.stage;
					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_WIRE:
					/* WIREはstageに左右されない */
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->reg.input_left, now_parser_tree_ir->reg.input_right);
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}
					current_stage = inst_stage.stage;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_CALL:
				case PARSER_IR_FLAG_SCALL:
					inst_stage = search_args_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->call.argument);
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}

					proc_stage = search_proc_llvm(now_parser_tree_ir, now_parser_tree_ir->flag);

					if(inst_stage.stage >= proc_stage.stage){
							current_stage = inst_stage.stage;
					}else{
							current_stage = proc_stage.stage;
					}
					current_parallel = proc_stage.parallel;

					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_REGISTER_LOOP:
					line_buf = charalloc(now_parser_tree_ir->phi.argument);

					do{
						//
						get_token_llvm(line_buf, token);
						// value
						get_token_llvm(line_buf, token);
//						printf("[LOOP]: value: %s\n", token);

						if(is_register_name(token)){
							inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, token, "");
							if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
								current_parallel++;
							}
						}
						proc_stage = search_proc_llvm(now_parser_tree_ir, now_parser_tree_ir->flag);

						if(inst_stage.stage >= proc_stage.stage){
								current_stage = inst_stage.stage;
						}else{
								current_stage = proc_stage.stage;
						}
						current_parallel = proc_stage.parallel;

						// ,
						get_token_llvm(line_buf, token);
						// name
						get_token_llvm(line_buf, token);
//						printf("[LOOP]: name: %s\n", token);
						// ]
						get_token_llvm(line_buf, token);
						// ,
						get_token_llvm(line_buf, token);
					}while(strlen(line_buf) > 0);
					free(line_buf);
					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_STORE:
					/*  */
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->reg.input_left, now_parser_tree_ir->reg.input_right);
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}
					proc_stage = search_proc_llvm(now_parser_tree_ir, now_parser_tree_ir->flag);

					if(inst_stage.stage >= proc_stage.stage){
							current_stage = inst_stage.stage;
					}else{
							current_stage = proc_stage.stage;
					}
					current_parallel = proc_stage.parallel;

					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_COMPARE:
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->comp.input_left, now_parser_tree_ir->comp.input_right);
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}
					current_stage = inst_stage.stage;
					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_POINTER:
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->pointer.name, now_parser_tree_ir->pointer.init_name);
					// ポインターが階層になっている場合は、全ての位置を把握する必要がある
					line_buf = charalloc(now_parser_tree_ir->pointer.add_name);

					do{
						// タイプ
						get_irtype_llvm(line_buf, token);
						// 変数
						get_irtype_llvm(line_buf, token);
						proc_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, token, "");
						// ,
						get_token_llvm(line_buf, token);
						if(proc_stage.stage > new_stage){
//							new_parallel = proc_stage.parallel;
							new_stage = proc_stage.stage;
						}
					}while(!strcmp(token, ",") && (strlen(line_buf) > 0));

					free(line_buf);

					if(inst_stage.stage >= proc_stage.stage){
							current_stage = inst_stage.stage;
					}else{
							current_stage = proc_stage.stage;
					}
					current_parallel = proc_stage.parallel;

					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_LOAD:
					line_buf = charalloc(now_parser_tree_ir->load.name);

					// getelementptr
					get_token_llvm(line_buf, token);
					if(!strcmp(token, "getelementptr")){
						// inbounds
						get_token_llvm(line_buf, token);

						inst_stage = search_args_llvm(now_parser_tree_ir, now_parser_tree_ir->label, line_buf);
						if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
							current_parallel++;
						}
					}else{
						inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, token, "");
					}
					free(line_buf);
					proc_stage = search_proc_llvm(now_parser_tree_ir, now_parser_tree_ir->flag);

					if(inst_stage.stage >= proc_stage.stage){
							current_stage = inst_stage.stage;
					}else{
							current_stage = proc_stage.stage;
					}
					current_parallel = proc_stage.parallel;

					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_SELECT:
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->select.condition_value, "");
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}
					current_stage = inst_stage.stage;
					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_LABEL:
					inst_stage = search_maxstage_llvm(now_parser_tree_ir);
					current_stage = inst_stage.stage;
					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_BRANCH:
					inst_stage = search_reg_llvm(now_parser_tree_ir, now_parser_tree_ir->label, now_parser_tree_ir->branch.condition_value, "");
					if((current_parallel > inst_stage.parallel) || (current_stage > inst_stage.stage)){
						current_parallel++;
					}
					proc_stage = search_proc_llvm(now_parser_tree_ir, now_parser_tree_ir->flag);

					if(inst_stage.stage >= proc_stage.stage){
							current_stage = inst_stage.stage;
					}else{
							current_stage = proc_stage.stage;
					}
					current_parallel = proc_stage.parallel;

					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					now_parser_tree_ir->prev_parallel = inst_stage.parallel;
					now_parser_tree_ir->prev_stage = inst_stage.stage;
					break;
				case PARSER_IR_FLAG_TYPE:
				case PARSER_IR_FLAG_ARRAY:
				case PARSER_IR_FLAG_RETURN:
				case PARSER_IR_FLAG_ALLOCA:
				case PARSER_IR_FLAG_CLOSE:
					now_parser_tree_ir->prev_parallel = current_parallel;
					now_parser_tree_ir->prev_stage = current_stage;
					current_stage++;
					now_parser_tree_ir->parallel = current_parallel;
					now_parser_tree_ir->stage = current_stage;
					break;
				case PARSER_IR_FLAG_NONE:
				case PARSER_IR_FLAG_UNKNOWN:
					break;
				default:
					printf("Unknown(can't parser: %d)\n", now_parser_tree_ir->flag);
					break;
			}

		}
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
    }

    free(token);
    free(init_name);
    free(next_name);

    return 0;
}

typedef struct pararell_tree{
	int now_stage;
	int last_stage;
} PARARELL_TREE;

/*!
 * @brief	内部レジスタの判定
 */
int is_register_name(char *name)
{
	int ret = 1;
	int i = 0;

	if(name[0] == '@'){
		ret = 0;
		return ret;
	}

	i = 1;

	while(name[i] != '\0'){
		if(isalpha(name[i])){
			ret = 0;
		}
		i++;
	}

	return ret;
}

/*!
 * @brief	ポインターの判定
 */
int is_pointer_type(char *name)
{
	if(name[strlen(name) -1] == '*'){
		return 1;
	}else{
		return 0;
	}
}
