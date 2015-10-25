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
 * @file	parser_ir_proc.c
 * @brief	Verilog HDLの生成
 * @author	Hidemi Ishiahra
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

/*!
 *
 */
typedef struct proc_tree{
    struct proc_tree	*prev_ptr;
    struct proc_tree	*next_ptr;
    int stage;
	struct seq_req{
		int	ena;
		char *state;
		char *condision;
		char *body;
	} seq_req;
	struct seq_wait{
		int	ena;
		char *state;
		char *condision;
		char *body;
	} seq_wait;
	struct seq_exec{
		int	ena;
		char *state;
		char *condision;
		char *body;
	} seq_exec;
} PROC_TREE;

PROC_TREE *proc_tree_top        = NULL;
PROC_TREE *proc_tree_current    = NULL;
PROC_TREE *proc_tree_prev       = NULL;

char *verilog_wire = NULL;
char *verilog_module = NULL;
char *verilog_state = NULL;
extern char *module_name;

/*!
 * @brief	ツリーの登録
 */
int insert_proc_tree()
{
    if(proc_tree_top == NULL){
        proc_tree_top     = (PROC_TREE *)malloc(sizeof(PROC_TREE));
        memset(proc_tree_top, 0, sizeof(PROC_TREE));
        proc_tree_current = proc_tree_top;
    }else{
        proc_tree_current->next_ptr   = (PROC_TREE *)malloc(sizeof(PROC_TREE));
        memset(proc_tree_current->next_ptr, 0, sizeof(PROC_TREE));
        proc_tree_prev                = proc_tree_current;
        proc_tree_current             = proc_tree_current->next_ptr;
        proc_tree_current->prev_ptr   = proc_tree_prev;
        proc_tree_current->next_ptr   = NULL;
    }
    return 0;
}

/*!
 * @brief	入力されたステージと同じツリーの検索
 */
int recall_proc_tree(int stage)
{
	PROC_TREE *now_proc_tree;

    now_proc_tree = proc_tree_top;
    while(now_proc_tree != NULL){
        if(now_proc_tree->stage == stage){
			proc_tree_current = now_proc_tree;
			return 0;
		}
        now_proc_tree = now_proc_tree->next_ptr;
    }
    return -1;
}

/*!
 * @brief	最後のツリーの呼び出し
 */
int recall_proc_tree_end()
{
	PROC_TREE *now_proc_tree;
	PROC_TREE *old_proc_tree;

    now_proc_tree = proc_tree_top;
    while(now_proc_tree != NULL){
		old_proc_tree = now_proc_tree;
        now_proc_tree = now_proc_tree->next_ptr;
    }
	proc_tree_current = old_proc_tree;
    return 0;
}

/*!
 * @brief	ツリーの削除
 */
int clean_proc_tree()
{
	PROC_TREE *now_proc_tree;
	PROC_TREE *new_proc_tree;

    now_proc_tree = proc_tree_top;
    while(now_proc_tree != NULL){
        new_proc_tree = now_proc_tree->next_ptr;
        if(now_proc_tree->seq_req.state != NULL)		free(now_proc_tree->seq_req.state);
        if(now_proc_tree->seq_req.condision != NULL)	free(now_proc_tree->seq_req.condision);
        if(now_proc_tree->seq_req.body != NULL)		free(now_proc_tree->seq_req.body);
        if(now_proc_tree->seq_wait.state != NULL)		free(now_proc_tree->seq_wait.state);
        if(now_proc_tree->seq_wait.condision != NULL)	free(now_proc_tree->seq_wait.condision);
        if(now_proc_tree->seq_wait.body != NULL)		free(now_proc_tree->seq_wait.body);
        if(now_proc_tree->seq_exec.state != NULL)		free(now_proc_tree->seq_exec.state);
        if(now_proc_tree->seq_exec.condision != NULL)	free(now_proc_tree->seq_exec.condision);
        if(now_proc_tree->seq_exec.body != NULL)		free(now_proc_tree->seq_exec.body);
        free(now_proc_tree);
        now_proc_tree = new_proc_tree;
    }
    proc_tree_top = NULL;
	proc_tree_current = proc_tree_top;

	if(verilog_wire != NULL){
		free(verilog_wire);
		verilog_wire = NULL;
	}
	if(verilog_module != NULL){
		free(verilog_module);
		verilog_module = NULL;
	}
	if(verilog_state != NULL){
		free(verilog_state);
		verilog_state = NULL;
	}

    return 0;
}

char *convname(char *buf)
{
	int i;
	for(i=0;i<strlen(buf);i++){
		if(buf[i] == '.') buf[i] = '_';
	}
	return buf;
}

/*!
 * @brief	ツリーの表示
 *
 */
int print_proc_tree(FILE *fp)
{
	PROC_TREE *now_proc_tree;

    fprintf(fp,"==============================\n");
    fprintf(fp,"Print - Proc Tree\n");
    fprintf(fp,"==============================\n");
    now_proc_tree = proc_tree_top;
    while(1){
        if(now_proc_tree == NULL) break;

		fprintf(fp,"Stage: %d\n", now_proc_tree->stage);

		if(now_proc_tree->seq_req.ena){
			if(now_proc_tree->seq_req.condision != NULL )	fprintf(fp," seq-c  ->\n%s\n", now_proc_tree->seq_req.condision);
			if(now_proc_tree->seq_req.body != NULL )		fprintf(fp," seq-b  ->\n%s\n", now_proc_tree->seq_req.body);
		}
		if(now_proc_tree->seq_wait.ena){
			if(now_proc_tree->seq_wait.condision != NULL )	fprintf(fp," wait-c ->\n%s\n", now_proc_tree->seq_wait.condision);
			if(now_proc_tree->seq_wait.body != NULL )		fprintf(fp," wait-b ->\n%s\n", now_proc_tree->seq_wait.body);
		}
		if(now_proc_tree->seq_exec.ena){
			if(now_proc_tree->seq_exec.condision != NULL )	fprintf(fp," exec-c ->\n%s\n", now_proc_tree->seq_exec.condision);
			if(now_proc_tree->seq_exec.body != NULL )		fprintf(fp," exec-b ->\n%s\n", now_proc_tree->seq_exec.body);
		}

        now_proc_tree = now_proc_tree->next_ptr;
    }

	fprintf(fp,"verilog_wire:\n%s\n", verilog_wire);

	fprintf(fp,"memory:\n");
	output_memory_tree(fp);

    fprintf(fp,"==============================\n");

    return 0;
}

/*!
 * @brief	Verilog HDLの出力
 *
 */
int output_proc_tree(FILE *fp)
{
	PROC_TREE *now_proc_tree;
	PROC_TREE *next_proc_tree;

	printf(" -> Output Verilog HDL: module = %s\n", sep_p(module_name));

	// モジュールヘッダの生成
	fprintf(fp,"/*\n");
	fprintf(fp," * Copyright (C)2005-2015 AQUAXIS TECHNOLOGY.\n");
	fprintf(fp," *  Don't remove this header.\n");
	fprintf(fp," * When you use this source, there is a need to inherit this header.\n");
	fprintf(fp," *\n");
	fprintf(fp," * This software is released under the MIT License.\n");
	fprintf(fp," * http://opensource.org/licenses/mit-license.php\n");
	fprintf(fp," *\n");
	fprintf(fp," * For further information please contact.\n");
	fprintf(fp," *  URI:    http://www.aquaxis.com/\n");
	fprintf(fp," *  E-Mail: info(at)aquaxis.com\n");
	fprintf(fp," */\n");

	fprintf(fp,"module %s(\n", sep_p(module_name));
	fprintf(fp,"\tinput __func_clock,\n");
	fprintf(fp,"\tinput __func_reset,\n");
	fprintf(fp,"\tinput __func_start,\n");
	fprintf(fp,"\toutput reg __func_done,\n");
//	fprintf(fp,"\toutput reg __func_idle,\n");
	fprintf(fp,"\toutput reg __func_ready,\n\n");

	fprintf(fp,"\toutput reg __gm_req,\n");
	fprintf(fp,"\toutput reg __gm_rnw,\n");
	fprintf(fp,"\tinput __gm_done,\n");
	fprintf(fp,"\toutput reg [31:0] __gm_adrs,\n");
	fprintf(fp,"\toutput reg [1:0] __gm_leng,\n");
	fprintf(fp,"\tinput [31:0] __gm_di,\n");
	fprintf(fp,"\toutput reg [31:0] __gm_do,\n\n");

	fprintf(fp,"\t// Memory Singal\n");
	output_memory_tree(fp);	// 引数

	fprintf(fp,"\t// Call Singal\n");
	output_call_signal_tree(fp);

	fprintf(fp,"\n");
	fprintf(fp,"\toutput reg __dummy\n");
	fprintf(fp,");\n\n");

	// reg,wire宣言
	output_signal_tree(fp);	// 信号定義の出力

	// WIRE接続(assign文)の出力
	if(verilog_wire != NULL){
		fprintf(fp,"%s", verilog_wire);
	}
	output_memory_tree_decl(fp);

	// ステートマシンの宣言出力
	if(verilog_state != NULL){
		fprintf(fp,"%s", verilog_state);
	}

	output_verilog_sdiv(fp);

    now_proc_tree = proc_tree_top;

	// ステートマシンの生成
	fprintf(fp,"always @(posedge __func_clock or negedge __func_reset) begin\n");
	fprintf(fp,"\tif(!__func_reset) begin\n");
	fprintf(fp,"\t\t__state <= __state_start_req;\n");
	fprintf(fp,"\t\t__func_ready <= 0;\n");
	fprintf(fp,"\t\t__func_done <= 0;\n");
	// 初期化を入れる
	fprintf(fp,"\tend else begin\n");
	fprintf(fp,"\tcase(__state)\n");

	fprintf(fp,"\t\t__state_start_req: begin\n");
	fprintf(fp,"\t\t\t__state <= __state_start_wait;\n");
	fprintf(fp,"\t\tend\n");

	fprintf(fp,"\t\t__state_start_wait: begin\n");
	fprintf(fp,"\t\t\tif(__func_start) begin\n");
	fprintf(fp,"\t\t\t\t__state <= __state_start_exec;\n");
	fprintf(fp,"\t\t\t\t__func_ready <= 0;\n");
	fprintf(fp,"\t\t\t\t__func_done <= 0;\n");
	output_memory_tree_proc(fp);
	fprintf(fp,"\t\t\tend\n");
	fprintf(fp,"\t\tend\n");

	fprintf(fp,"\t\t__state_start_exec: begin\n");
	if(now_proc_tree->seq_req.ena){
		fprintf(fp,"\t\t\t__state <= __state_1_req;\n");
	}else if(now_proc_tree->seq_wait.ena){
		fprintf(fp,"\t\t\t__state <= __state_1_wait;\n");
	}else{
		fprintf(fp,"\t\t\t__state <= __state_1_exec;\n");
	}
	fprintf(fp,"\t\tend\n");

    while(now_proc_tree != NULL){
		if(now_proc_tree->seq_req.ena){
			fprintf(fp, "\t\t__state_%d_req: begin\n", now_proc_tree->stage);
			fprintf(fp, "%s", now_proc_tree->seq_req.state);
			if(now_proc_tree->seq_req.body != NULL )		fprintf(fp, "%s", now_proc_tree->seq_req.body);
			fprintf(fp, "\t\tend\n");
		}

		if(now_proc_tree->seq_wait.ena){
			fprintf(fp, "\t\t__state_%d_wait: begin\n", now_proc_tree->stage);
			fprintf(fp, "%s", now_proc_tree->seq_wait.state);
			if(now_proc_tree->seq_wait.body != NULL )		fprintf(fp, "%s", now_proc_tree->seq_wait.body);
			fprintf(fp, "\t\tend\n");
		}

		{
			fprintf(fp, "\t\t__state_%d_exec: begin\n", now_proc_tree->stage);
			if(now_proc_tree->seq_exec.state != NULL){
				if(strlen(now_proc_tree->seq_exec.state) > 0){
					fprintf(fp, "%s\n", now_proc_tree->seq_exec.state);
				}
			}else{
		        next_proc_tree = now_proc_tree->next_ptr;
		        if(next_proc_tree != NULL){
					if(next_proc_tree->seq_req.ena){
						fprintf(fp, "\t\t\t__state <= __state_%d_req;\n", now_proc_tree->stage + 1);
					}else if(next_proc_tree->seq_wait.ena){
						fprintf(fp, "\t\t\t__state <= __state_%d_wait;\n", now_proc_tree->stage + 1);
					}else{
						fprintf(fp, "\t\t\t__state <= __state_%d_exec;\n", now_proc_tree->stage + 1);
					}
				}
			}
			if(now_proc_tree->seq_exec.body != NULL )	fprintf(fp, "%s", now_proc_tree->seq_exec.body);
			fprintf(fp, "\t\tend\n");
		}
        now_proc_tree = now_proc_tree->next_ptr;
    }

	fprintf(fp,"\t\t__state_fin_exec: begin\n");
	fprintf(fp,"\t\t\t__state <= __state_start_req;\n");
	fprintf(fp,"\t\t\t__func_ready <= 1;\n");
	fprintf(fp,"\t\t\t__func_done <= 1;\n");
	fprintf(fp,"\t\tend\n");

	fprintf(fp,"\tendcase\n");
	fprintf(fp,"\tend\n");
	fprintf(fp,"end\n");

	fprintf(fp,"endmodule\n");

    return 0;
}

/*!
 * @brief	レジスタ名の変換
 *
 * @note
 * %xxxxの場合、__sig_xxxxに変換、
 * xxxxの場合、そのまま、xxxxとする
 */
char *regalloc(char *name)
{
	char *buf;
	int i;
	int ret;

	// 確保している容量は暫定です
	buf=calloc(strlen(name)+20,1);
	if(!strcmp(name, "false")){
		sprintf(buf, "(0)");
	}else if(!strcmp(name, "true")){
		sprintf(buf, "(1)");
	}else if(name[0] == '%'){
		ret = is_memmap_tree(module_name, name);
		if(ret){
			// ポインターで固定アドレスを解けた場合
			ret = get_adrs_memmap(module_name, name);
			sprintf(buf, "(%d)", ret);
		}else{
			// 変数
			sprintf(buf, "__sig_%s", &name[1]);
		}
	}else if(name[0] == '@'){
		// メモリ
		ret = is_memmap_tree(module_name, name);
		if(ret){
			// メモリで固定アドレスを解けた場合
			ret = get_adrs_memmap(module_name, name);
			sprintf(buf, "(%d)", ret);
		}else{
			sprintf(buf, "__sig_%s", &name[1]);
		}
	}else{
		sprintf(buf, "(%s)", name);
	}

	for(i=0;i<strlen(buf);i++){
		if(buf[i] == '.') buf[i] = '_';
	}

	return buf;
}

char *labelalloc(char *name)
{
	char *buf;
	int i;

	if(name[0] == '%'){
		buf=calloc(strlen(name)+1+8,1);
		sprintf(buf, "__label_%s", &name[1]);
	}else{
		buf=calloc(strlen(name)+1+8,1);
		sprintf(buf, "__label_%s", name);
	}

	for(i=0;i<strlen(buf);i++){
		if(buf[i] == '.') buf[i] = '_';
	}

	return buf;
}

/*!
 * @brief	CALL命令の引数を生成
 *
 * @note
 * argsからアクセスするためのコードとI/Fを生成する
 */
int create_verilog_call_args(char *name, char *args, char *buf, int is_call)
{
	char *token;
	char *line;
	char *temp;
	char *type;
	int args_num = 0;
	char *str;
	int width;

	token = calloc(STR_MAX, 1);
	temp = calloc(STR_MAX, 1);
	type = calloc(STR_MAX, 1);
	line = charalloc(args);
	strcpy(buf, "");

	get_token_llvm(line, token);	// "("

	/* ToDo:
	 * Verilog HDLを作成しつつ、外部信号も生成すること
	 */
	while(strlen(line) > 0){
		// タイプ
		/*
		 * タイプが整数の場合は、Writeのみ
		 * タイプがポインタの場合はRead/Write有り
		 * タイプがメモリの場合はRead/Write有り
		 */
		get_irtype_llvm(line, token);
		if(!strcmp(token, "") || !strcmp(token,")")) break;
		strcpy(type, token);

		{
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
		}

		if(!strcmp(token, "getelementptr")){
			// ポインタの場合はアドレス値を算出するように復元
			sprintf(temp, "%s%s", token, line);
			free(line);
			line = calloc(strlen(temp)+1, 1);
			strcpy(line, temp);
			get_irtype_llvm(line, token);
		}

		create_adrs_getelementptr(token, token);
		str = regalloc(token);
		sprintf(temp, "\t\t\t__call_%s_args_%d <= %s;\n", name, args_num, str);
		free(str);
		strcat(buf, temp);

		// インターフェースの作成
		if(!strcmp(type, "i8")){
			width = 8;
		}else if(!strcmp(type, "i16")){
			width = 16;
		}else if(
			!strcmp(type, "i32") ||
			!strcmp(type, "i8*") || !strcmp(type, "i16*") || !strcmp(type, "i32*")
		){
			width = 32;
		}else{
			// ポインタの場合はここに入る
			width = 32;
		}
		// call_sianal_treeへ登録
		if(!is_call) register_call_signal_tree(name, "args", args_num, width, 1);
		args_num++;

		// 次の","か")"まで読み飛ばし
		do{
			get_token_llvm(line, token);
		}while(strcmp(token, ",") && strcmp(token, ")"));
	}

	free(token);
	free(type);
	free(temp);
	free(line);

	return 0;
}

/*!
 * @brief	getelementptrのアドレス位置を生成
 *
 * フォーマット
 * <result> = getelementptr <ty>, <ty>* <ptrval>{, <ty> <idx>}*
 * <result> = getelementptr inbounds <ty>, <ty>* <ptrval>{, <ty> <idx>}*
 * <result> = getelementptr <ty>, <ptr vector> <ptrval>, <vector index type> <idx>
 *
 * サンプル
 * %1 = getelementptr inbounds [64 x i8]* %DU, i32 0, i32 0
 * %16 = getelementptr inbounds [256 x i8]* @DU_Y, i32 0, i32 %15
 */
int create_adrs_getelementptr(char *line, char *buf)
{
	char *token = NULL;
	char *line_org = NULL;
	char *name = NULL;
	char *type = NULL;
	int base_adrs = 0;
	char *str1, *str2;

	token = calloc(STR_MAX, 1);
	line_org = calloc(strlen(line)+1,1);
	strcpy(line_org, line);
	strcpy(buf, "");

	// getelementptr
	get_token_llvm(line_org, token);
	if(strcmp(token, "getelementptr")){
		/*
		 * getelementptrでなはい場合はメモリマップからアドレスを取得する
		 */
		if(token[0] == '@'){
			base_adrs = get_adrs_memmap("GLOBAL", token);
			// ToDo:
			// サイズがない場合の処理を考えておこう
			if(base_adrs == -1){
				// アドレスが取得できない場合は変数をそのまま代入する
				str2 = regalloc(token);
				sprintf(buf, "%s", str2);
				free(str2);
			}else{
				sprintf(buf, "%d", base_adrs);
			}
//			sprintf(buf, "%d", base_adrs);
		}else if(token[0] == '%'){
			base_adrs = get_adrs_memmap(module_name, token);
			if(base_adrs == -1){
				// アドレスが取得できない場合は変数をそのまま代入する
				str2 = regalloc(token);
				sprintf(buf, "%s", str2);
				free(str2);
			}else{
				sprintf(buf, "%d", base_adrs);
			}
		}else{
			strcpy(buf, token);
		}
		free(token);
		free(line_org);
		return 0;
	}

	get_irtype_llvm(line_org, token);	// inbounds

	if(!strcmp(token, "inbounds")) get_irtype_llvm(line_org, token);
	if(!strcmp(token, "(")) get_irtype_llvm(line_org, token);
	// ポインタのタイプ
	type = calloc(strlen(token)+1,1);
	strcpy(type, token);

	// ポインタの名前
	get_token_llvm(line_org, token);
	name = calloc(strlen(token)+1,1);
	strcpy(name, token);

	get_token_llvm(line_org, token);	// ,

	if(
		!strcmp(type, "i8*") ||
		!strcmp(type, "i16*") ||
		!strcmp(type, "i32*")
	){
		// シングルタイプの場合
		// タイプ
		get_token_llvm(line_org, token);
		// 数値
		get_token_llvm(line_org, token);

		str1 = regalloc(name);
		str2 = regalloc(token);
		sprintf(buf, "%s + %s", str1, str2);
		free(str1);
		free(str2);
	}else{
		// arrayタイプの場合
		if(is_pointer_type(name)) name[strlen(name) -1] = '\0';
		base_adrs = get_memory_size(name, line_org, buf);
	}

	if(type != NULL) free(type);
	if(line_org != NULL) free(line_org);
	if(token != NULL)	free(token);

	return 0;
}

/*!
 * @brief	getelementptrからtypeを取得する
 */
int get_getelement_type(char *line, char *result)
{
	char *token;
	char *buf;

	token = calloc(STR_MAX, 1);
	buf = charalloc(line);

	get_token_llvm(buf, token);		// getelementptr
	get_token_llvm(buf, token);		// inbounds
	get_irtype_llvm(buf, token);	// Type
	strcpy(result, token);

	free(token);

	return 0;
}

/*!
 * @brief	Verilog HDLの登録
 * @note
 * bodyがNULLであれば、charallocでbufを取得して、登録する
 * bodyがある場合は、追加なので一旦、freeしてから追加登録する
 */
char * register_verilog(char *body, char *buf)
{
    char *temp = NULL;

	// Verilogコード登録
	if(body != NULL){
		temp = charalloc(body);
		free(body);
		body = calloc(strlen(temp) + strlen(buf) + 1, 1);
		sprintf(body, "%s%s", temp, buf);
		free(temp);
	}else{
		body = charalloc(buf);
	}

	return body;
}

/*!
 * @brief	プロセス部のVerilog HDLの生成
 * @note
 * プロセス部とは実行部分のところを指している。
 * ただし、モジュール宣言の部分は既に生成しているのでここではDon't careとする
 */
int create_verilog_proc_tree()
{
	PARSER_TREE_IR *now_parser_tree_ir;
    char *token;
    char *buf;
    char *args_buf;
    int leng;
    char *label;
    char *value;

    char *str1, *str2, *str3, *str4;
	char *line_buf;
	char *call_name;

	int is_call;

	token = calloc(STR_MAX, 1);
	buf = calloc(STR_MAX, 1);
	args_buf = calloc(STR_MAX, 1);
	label = calloc(STR_MAX, 1);
	value = calloc(STR_MAX, 1);

    printf(" -> Create Verilog HDL for Proc Tree\n");
    now_parser_tree_ir = parser_tree_ir_top;
    while(now_parser_tree_ir != NULL){
		if( now_parser_tree_ir->stage != 0 ){
			if(proc_tree_current == NULL){
				// ツリーがなければ新規生成
				insert_proc_tree();
			}else{
				if(proc_tree_current->stage < now_parser_tree_ir->stage){
					// ステージ番号が大きくなっていれば新規生成
					insert_proc_tree();
				}else if(proc_tree_current->stage != now_parser_tree_ir->stage){
					// ステージ番号が小さい場合は呼び出しをする
					recall_proc_tree(now_parser_tree_ir->stage);
				}
			}
			proc_tree_current->stage = now_parser_tree_ir->stage;

			switch(now_parser_tree_ir->flag){
				case PARSER_IR_FLAG_REGISTER:
					proc_tree_current->seq_exec.ena = 1;

					if(!strcmp(now_parser_tree_ir->reg.name, "add")){
						strcpy(label, "+");
					}else if(!strcmp(now_parser_tree_ir->reg.name, "sub")){
						strcpy(label, "-");
					}else if(!strcmp(now_parser_tree_ir->reg.name, "mul")){
						strcpy(label, "*");
					}else if(!strcmp(now_parser_tree_ir->reg.name, "and")){
						strcpy(label, "&");
					}else if(!strcmp(now_parser_tree_ir->reg.name, "or")){
						strcpy(label, "|");
					}else if(!strcmp(now_parser_tree_ir->reg.name, "xor")){
						strcpy(label, "^");
					}else{
						strcpy(label, "+");
						printf("[ERROR] REGISTER - unkown process(%s)\n", now_parser_tree_ir->reg.name);
					}

					str1 = regalloc(now_parser_tree_ir->label);
					str2 = regalloc(now_parser_tree_ir->reg.input_left);
					str3 = regalloc(now_parser_tree_ir->reg.input_right);
					sprintf(buf, "\t\t\t%s <= %s %s %s;\n",
						str1,
						str2,
						label,
						str3
					);
					free(str1);
					free(str2);
					free(str3);
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);

					break;
				case PARSER_IR_FLAG_FLOAT:
					/*
					 * FLOATは未実装
					 */
					printf("[ABORT] Process for float\n");
					exit(0);
					break;
				case PARSER_IR_FLAG_WIRE:
					/*
					 * WIREはassign文で形成する
					 */
					str1 = regalloc(now_parser_tree_ir->label);
					str2 = regalloc(now_parser_tree_ir->reg.input_left);
					sprintf(buf, "assign %s = %s",
						str1,
						str2
					);
					free(str1);
					free(str2);
					strcpy(label, "");
					str2 = regalloc(now_parser_tree_ir->reg.input_right);
					if(!strcmp(now_parser_tree_ir->reg.name, "shl")){
						sprintf(label, " << %s", str2);
					}else if(!strcmp(now_parser_tree_ir->reg.name, "lshl")){
						sprintf(label, " << %s", str2);
					}else if(!strcmp(now_parser_tree_ir->reg.name, "lshr")){
						sprintf(label, " >> %s", str2);
					}else if(!strcmp(now_parser_tree_ir->reg.name, "ashr")){
						sprintf(label, " >> %s", str2);
					}
					sprintf(buf, "%s%s;\n", buf, label);
					verilog_wire = register_verilog(verilog_wire, buf);
					break;
				case PARSER_IR_FLAG_CALL:
					/*
					 * CALLは外部モジュールへのアクセスである。
					 */
					/*
					 * lifetimeの場合、何もしない。
					 * lifetimeはメモリのロック期間を示している。
					 */
					if(
						!strcmp(now_parser_tree_ir->call.name, "@llvm.lifetime.start") ||
						!strcmp(now_parser_tree_ir->call.name, "@llvm.lifetime.end")
					){
						proc_tree_current->seq_exec.ena = 1;
						break;
					}

					/*
					 * 名前の変換
					 * @llvm.memcpy.p0i8.p0i8.i32 -> @llvm_memcpy
					 */
					if(!strcmp(now_parser_tree_ir->call.name, "@llvm.memcpy.p0i8.p0i8.i32")){
						strcpy(now_parser_tree_ir->call.name, "@llvm_memcpy");
//						printf("[WARINIG] find a \"@llvm.memcpy.p0i8.p0i8.i32\"\n");
					}

					proc_tree_current->seq_req.ena = 1;
					proc_tree_current->seq_wait.ena = 1;
					proc_tree_current->seq_exec.ena = 1;

					str1 = calloc(strlen(now_parser_tree_ir->call.name)+1,1);
					strcpy(str1, now_parser_tree_ir->call.name);

					str2 = convname(sep_p(str1));

					// CALLツリーへの登録
					is_call = register_call_tree(now_parser_tree_ir->call.name);
					if(!is_call){
						register_call_signal_tree(now_parser_tree_ir->call.name, "req", 0, 0, 1);
						register_call_signal_tree(now_parser_tree_ir->call.name, "ready", 0, 0, 0);
						register_call_signal_tree(now_parser_tree_ir->call.name, "done", 0, 0, 0);
					}

					// reqプロセス
					sprintf(buf, "\t\t\tif(__call_%s_ready) begin\n\t\t\t\t__call_%s_req <= 1;\n\t\t\tend\n",
						str2,
						str2
					);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);

					/*
					 * 引数を出力する
					 */
					create_verilog_call_args(&now_parser_tree_ir->call.name[1], now_parser_tree_ir->call.argument, args_buf, is_call);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, args_buf);
					// waitプロセス
					// 条件
					sprintf(buf, "(__call_%s_done == 1)",
						str2
					);
					proc_tree_current->seq_wait.condision = register_verilog(proc_tree_current->seq_wait.condision, buf);

					sprintf(buf, "\t\t\t__call_%s_req <= 0;\n",
						str2
					);
					proc_tree_current->seq_wait.body = register_verilog(proc_tree_current->seq_wait.body, buf);

					// execプロセス
					/*
					 * CALLは戻り値を格納するケースがある。
					 * 戻り値は未実装
					 */
					free(str1);

					break;
				case PARSER_IR_FLAG_BRANCH:
					/*
					 * BRANCHはジャンプするだけ
					 * ジャンブ場所はステージ番号である。
					 */
					proc_tree_current->seq_exec.ena = 1;

					// execプロセス
					if(!strcmp(now_parser_tree_ir->branch.condition_value, "none")){
						// 無条件ジャンプ
						sprintf(buf, "\t\t\t__state <= __state_%d_exec;",
							get_stage_label(now_parser_tree_ir->branch.branch_true)
						);
						proc_tree_current->seq_exec.state = register_verilog(proc_tree_current->seq_exec.state, buf);
					}else{
						// 条件ジャンプ
						str1=regalloc(now_parser_tree_ir->branch.condition_value);
						sprintf(buf, "\t\t\t__state <= (%s)?__state_%d_exec:__state_%d_exec;",
							str1,
							get_stage_label(now_parser_tree_ir->branch.branch_true),
							get_stage_label(now_parser_tree_ir->branch.branch_false)
						);
						free(str1);
						proc_tree_current->seq_exec.state = register_verilog(proc_tree_current->seq_exec.state, buf);
					}
					break;
				case PARSER_IR_FLAG_LABEL:
					/*
					 * LABELは通過するだけです。
					 */
					proc_tree_current->seq_exec.ena = 1;

					str1=labelalloc(now_parser_tree_ir->label);
					sprintf(buf, "\t\t\t__label <= %s;\n",
						convname(str1)
					);
					free(str1);
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);
					break;
				case PARSER_IR_FLAG_REGISTER_LOOP:
					/*
					 * LOOPはcase文で対応する
					 */
					proc_tree_current->seq_exec.ena = 1;

					sprintf(buf, "\t\t\tcase(__label)\n");
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);

					line_buf = charalloc(now_parser_tree_ir->phi.argument);

					do{
						//
						get_token_llvm(line_buf, token);
						// value
						get_token_llvm(line_buf, token);
						strcpy(value, token);
						str3 = regalloc(token);
						// ,
						get_token_llvm(line_buf, token);
						// name
						get_token_llvm(line_buf, token);
						str1 = labelalloc(token);
						strcpy(label, token);
						// ]
						get_token_llvm(line_buf, token);
						// ,
						get_token_llvm(line_buf, token);
						str2 = regalloc(now_parser_tree_ir->label);
						sprintf(buf, "\t\t\t\t%s: %s <= %s;\n",
							str1,
							str2,
							str3
						);
						free(str1);
						free(str2);
						proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);
					}while(strlen(line_buf) > 0);
					free(line_buf);
					sprintf(buf, "\t\t\tendcase\n");
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);
					break;
				case PARSER_IR_FLAG_TYPE:
					/*
					 * TYPEは対象外
					 */
					break;
				case PARSER_IR_FLAG_STORE:
					proc_tree_current->seq_req.ena = 1;
					proc_tree_current->seq_wait.ena = 1;
					proc_tree_current->seq_exec.ena = 1;

					// reqプロセス
					sprintf(buf, "\t\t\t__gm_req <= 1;\n\t\t\t__gm_rnw <= 0;\n");
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);
					// アドレス
					create_adrs_getelementptr(now_parser_tree_ir->reg.input_right, args_buf);
					str1 = regalloc(args_buf);
					sprintf(buf, "\t\t\t__gm_adrs <= %s;\n", str1);
					free(str1);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);
					// レングス
					if(!strcmp(now_parser_tree_ir->reg.input_right_type, "i8") || !strcmp(now_parser_tree_ir->reg.input_right_type, "i8*") || !strcmp(now_parser_tree_ir->reg.input_right_type, "i8**")){
						leng = 0;
					}else if(!strcmp(now_parser_tree_ir->reg.input_right_type, "i16") || !strcmp(now_parser_tree_ir->reg.input_right_type, "i16*") || !strcmp(now_parser_tree_ir->reg.input_right_type, "i16**")){
						leng = 1;
					}else if(!strcmp(now_parser_tree_ir->reg.input_right_type, "i32") || !strcmp(now_parser_tree_ir->reg.input_right_type, "i32*") || !strcmp(now_parser_tree_ir->reg.input_right_type, "i32**")){
						leng = 3;
					}else{
						/*
						 * ToDo:
						 * ちゃんと処理すること。いまのところ暫定だよ。
						 */
						leng = 3;
						printf("[WRANING] LENGTH: %s\n", now_parser_tree_ir->reg.input_right_type);
					}
					sprintf(buf, "\t\t\t__gm_leng <= %d;\n",
						leng
					);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);
					str1 = regalloc(now_parser_tree_ir->reg.input_left);
					sprintf(buf, "\t\t\t__gm_do <= %s;\n",
						str1
					);
					free(str1);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);

					// waitプロセス
					sprintf(buf, "(__gm_done == 1)");
					proc_tree_current->seq_wait.condision = register_verilog(proc_tree_current->seq_wait.condision, buf);
					sprintf(buf, "\t\t\t__gm_req <= 0;\n");
					proc_tree_current->seq_wait.body = register_verilog(proc_tree_current->seq_wait.body, buf);

					// execプロセス
					//STOREは領域への書き込みなのでexecプロセスは存在しない。
					break;
				case PARSER_IR_FLAG_ARRAY:
					/*
					 * ARRAYは対象外のはず。
					 * 後日検討
					 */
					break;
				case PARSER_IR_FLAG_COMPARE:
					// 比較演算
					proc_tree_current->seq_exec.ena = 1;

					// execプロセス
					if(!strcmp(now_parser_tree_ir->comp.value, "sgt")){
						strcpy(label, ">");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "sge")){
						strcpy(label, ">=");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "slt")){
						strcpy(label, "<");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "sle")){
						strcpy(label, "<=");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "ugt")){
						strcpy(label, ">");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "uge")){
						strcpy(label, ">=");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "ult")){
						strcpy(label, "<");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "ule")){
						strcpy(label, "<=");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "eq")){
						strcpy(label, "==");
					}else if(!strcmp(now_parser_tree_ir->comp.value, "ne")){
						strcpy(label, "!=");
					}else{
						printf("[ERROR] COMPARE - unknown value(%s)\n", now_parser_tree_ir->comp.value);
					}
					str1 = regalloc(now_parser_tree_ir->label);
					str2 = regalloc(now_parser_tree_ir->comp.input_left);
					str3 = regalloc(now_parser_tree_ir->comp.input_right);
					sprintf(buf, "\t\t\t%s <= ( %s %s %s );\n",
						str1,
						str2,
						label,
						str3
					);
					free(str1);
					free(str2);
					free(str3);
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);

					break;
				case PARSER_IR_FLAG_RETURN:
					/*
					 * RETURNはfinへジャンプするだけ
					 */
					proc_tree_current->seq_exec.ena = 1;

					// execプロセス
					sprintf(buf, "__state <= __state_fin;\n");
					proc_tree_current->seq_exec.condision = register_verilog(proc_tree_current->seq_exec.condision, buf);
					break;
				case PARSER_IR_FLAG_POINTER:
					/*
					 * POINTERはポインタの位置を計算し、wireとする。
					 * なので、複雑なポインタ計算があると、処理速度に影響を与える恐れがある。
					 */
					create_adrs_getelementptr(now_parser_tree_ir->pointer.str, args_buf);
					str1 = regalloc(now_parser_tree_ir->label);
					str2 = regalloc(args_buf);
					sprintf(buf, "assign %s = %s;\n",
						str1,
						str2
					);
					free(str1);
					free(str2);

					verilog_wire = register_verilog(verilog_wire, buf);
					break;
				case PARSER_IR_FLAG_LOAD:
					/*
					 * LOADはグローバル変数、グローバル構造体、グローバルメモリの
					 * いずれからレジスタにデータが格納される。
					 */

					proc_tree_current->seq_req.ena = 1;
					proc_tree_current->seq_wait.ena = 1;
					proc_tree_current->seq_exec.ena = 1;

					// reqプロセス
					sprintf(buf, "\t\t\t__gm_req <= 1;\n\t\t\t__gm_rnw <= 1;\n");
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);
					// アドレス
					create_adrs_getelementptr(now_parser_tree_ir->load.name, args_buf);
					str1 = regalloc(args_buf);
					sprintf(buf, "\t\t\t__gm_adrs <= %s;\n", str1);
					free(str1);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);
					// レングス
					if(!strcmp(now_parser_tree_ir->load.type, "i8") || !strcmp(now_parser_tree_ir->load.type, "i8*") || !strcmp(now_parser_tree_ir->load.type, "i8**")){
						leng = 0;
					}else if(!strcmp(now_parser_tree_ir->load.type, "i16") || !strcmp(now_parser_tree_ir->load.type, "i16*") || !strcmp(now_parser_tree_ir->load.type, "i16**")){
						leng = 1;
					}else if(!strcmp(now_parser_tree_ir->load.type, "i32") || !strcmp(now_parser_tree_ir->load.type, "i32*") || !strcmp(now_parser_tree_ir->load.type, "i32**")){
						leng = 3;
					}else{
						/*
						 * ToDo:
						 * ちゃんと処理すること。いまのところ暫定だよ。
						 */
						leng = 3;
						printf("[WARNING] LENGTH: %s\n", now_parser_tree_ir->load.type);
					}
					sprintf(buf, "\t\t\t__gm_leng <= %d;\n",
						leng
					);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);

					// waitプロセス
					sprintf(buf, "(__gm_done == 1)");
					proc_tree_current->seq_wait.condision = register_verilog(proc_tree_current->seq_wait.condision, buf);
					sprintf(buf, "\t\t\t__gm_req <= 0;\n");
					proc_tree_current->seq_wait.body = register_verilog(proc_tree_current->seq_wait.body, buf);

					// execプロセス
					if(now_parser_tree_ir->label[0] != '%') printf("known error\n");
					str1 = regalloc(now_parser_tree_ir->label);
					sprintf(buf, "\t\t\t%s <= __gm_di;\n",
						str1
					);
					free(str1);
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);
					break;
				case PARSER_IR_FLAG_SELECT:
					proc_tree_current->seq_exec.ena = 1;

					str1 = regalloc(now_parser_tree_ir->label);
					str2 = regalloc(now_parser_tree_ir->select.condition_value);
					str3 = regalloc(now_parser_tree_ir->select.select_true);
					str4 = regalloc(now_parser_tree_ir->select.select_false);
					sprintf(buf, "\t\t\t%s <= (%s)?%s:%s;\n",
						str1,
						str2,
						str3,
						str4
					);
					free(str1);
					free(str2);
					free(str3);
					free(str4);
					proc_tree_current->seq_exec.body = register_verilog(proc_tree_current->seq_exec.body, buf);
					break;
				case PARSER_IR_FLAG_ALLOCA:
					/*
					 * ALLOCはVerilog HDLに展開しない。
					 * そういう訳にはいかなかった
					 */
					break;
				case PARSER_IR_FLAG_SCALL:
					/*
					 * SCALLは除算回路である。
					 */
					proc_tree_current->seq_req.ena = 1;
					proc_tree_current->seq_wait.ena = 1;
					proc_tree_current->seq_exec.ena = 1;

					call_name = calloc(strlen("@sdiv")+1, 1);
					strcpy(call_name, "@sdiv");

					str1 = calloc(strlen(call_name)+1,1);
					strcpy(str1, call_name);

					str2 = convname(sep_p(str1));

					// CALLツリーへの登録
					// sdivは登録しなくてもいいや
/*
					is_call = register_call_tree(call_name);
					if(!is_call){
						register_call_signal_tree(call_name, "req", 0, 0, 1);
						register_call_signal_tree(call_name, "ready", 0, 0, 0);
						register_call_signal_tree(call_name, "done", 0, 0, 0);
					}
*/
					// reqプロセス
					sprintf(buf, "\t\t\tif(__call_%s_ready) begin\n\t\t\t\t__call_%s_req <= 1;\n\t\t\tend\n",
						str2,
						str2
					);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);

					/*
					 * 引数を出力する
					 */
//					create_verilog_call_args(call_name, now_parser_tree_ir->call.argument, args_buf, is_call);
//					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, args_buf);
					str3 = regalloc(now_parser_tree_ir->reg.input_left);
					str4 = regalloc(now_parser_tree_ir->reg.input_right);
					sprintf(buf, "\t\t\t\t__call_%s_args_0 <= %s;\n\t\t\t\t__call_%s_args_1 <= %s;\n",
						str2,
						str3,
						str2,
						str4
					);
					proc_tree_current->seq_req.body = register_verilog(proc_tree_current->seq_req.body, buf);
					// waitプロセス
					// 条件
					sprintf(buf, "(__call_%s_done == 1)",
						str2
					);
					proc_tree_current->seq_wait.condision = register_verilog(proc_tree_current->seq_wait.condision, buf);

					str3 = regalloc(now_parser_tree_ir->label);
					sprintf(buf, "\t\t\t__call_%s_req <= 0;\n\t\t\t%s <= __call_%s_result;\n",
						str2,
						str3,
						str2
					);
					proc_tree_current->seq_wait.body = register_verilog(proc_tree_current->seq_wait.body, buf);

					// execプロセス
					/*
					 * CALLは戻り値を格納するケースがある。
					 * 戻り値は未実装
					 */
					free(str1);

					break;
				case PARSER_IR_FLAG_CLOSE:
					/*
					 * CLOSEは未処理
					 * ToDo:
					 *   finプロセスにする
					 */
					proc_tree_current->seq_exec.ena = 1;

					sprintf(buf, "__state <= __state_start_req;\n");
					proc_tree_current->seq_exec.condision = register_verilog(proc_tree_current->seq_exec.condision, buf);
					break;
				case PARSER_IR_FLAG_NONE:
					break;
				case PARSER_IR_FLAG_MODULE:
					break;
				case PARSER_IR_FLAG_UNKNOWN:
					break;
				default:
					printf("[ERROR]Unknown(can't parser: %d)\n", now_parser_tree_ir->flag);
					break;
			}

			/*
			 * ステートマシンのVerilog HDLコードを生成
			 */
			if(proc_tree_current->seq_req.ena){
				sprintf(buf, "\t\t\t__state <= __state_%d_wait;\n", proc_tree_current->stage);
				proc_tree_current->seq_req.state = charalloc(buf);
			}

			if(proc_tree_current->seq_wait.ena){
				sprintf(buf, "\t\t\tif(%s) begin\n\t\t\t\t__state <= __state_%d_exec;\n\t\t\tend\n", proc_tree_current->seq_wait.condision, proc_tree_current->stage);
				proc_tree_current->seq_wait.state = charalloc(buf);
			}

			if(proc_tree_current->seq_exec.ena){
				switch(now_parser_tree_ir->flag){
					case PARSER_IR_FLAG_CLOSE:
						sprintf(buf, "\t\t\t__state <= __state_fin_exec;");
						proc_tree_current->seq_exec.state = charalloc(buf);
						break;
					case PARSER_IR_FLAG_RETURN:
						sprintf(buf, "\t\t\t__state <= __state_fin_exec;");
						proc_tree_current->seq_exec.state = charalloc(buf);
						break;
					default:
						break;
				}
			}

			recall_proc_tree_end();
		}else{
		}

        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
    }

	free(token);
	free(buf);
	free(args_buf);
	free(label);
	free(value);

    return 0;
}

/*!
 * @brief	ステートマシンのパラメータ生成
 */
int create_verilog_state()
{
	PROC_TREE *now_proc_tree = proc_tree_top;
    char *buf;
	int count = 4;

	buf = calloc(STR_MAX, 1);

	sprintf(buf, "localparam __state_fin_exec = 0;\n");
	verilog_state = register_verilog(verilog_state, buf);
	sprintf(buf, "localparam __state_start_req = 1;\n");
	verilog_state = register_verilog(verilog_state, buf);
	sprintf(buf, "localparam __state_start_wait = 2;\n");
	verilog_state = register_verilog(verilog_state, buf);
	sprintf(buf, "localparam __state_start_exec = 3;\n");
	verilog_state = register_verilog(verilog_state, buf);

    while(now_proc_tree != NULL){
		if(now_proc_tree->seq_req.ena){
			sprintf(buf, "localparam __state_%d_req = %d;\n", now_proc_tree->stage, count);
			verilog_state = register_verilog(verilog_state, buf);
			count++;
		}
		if(now_proc_tree->seq_wait.ena){
			sprintf(buf, "localparam __state_%d_wait = %d;\n", now_proc_tree->stage, count);
			verilog_state = register_verilog(verilog_state, buf);
			count++;
		}
		sprintf(buf, "localparam __state_%d_exec = %d;\n", now_proc_tree->stage, count);
		verilog_state = register_verilog(verilog_state, buf);
		count++;
        now_proc_tree = now_proc_tree->next_ptr;
	}

	sprintf(buf, "integer __state;\n");
	verilog_state = register_verilog(verilog_state, buf);

	free(buf);

	return 0;
}

/*!
 * @brief	ラベルのパラーメータ生成
 */
int create_verilog_label()
{
	PARSER_TREE_IR *now_parser_tree_ir = parser_tree_ir_top;
    char *buf;
    char *str;
	int count = 1;

	buf = calloc(STR_MAX, 1);

	sprintf(buf, "localparam __label_0 = 0;\n");
	verilog_state = register_verilog(verilog_state, buf);

    while(now_parser_tree_ir != NULL){
		if(now_parser_tree_ir->flag == PARSER_IR_FLAG_LABEL){
			str = labelalloc(now_parser_tree_ir->label);
			sprintf(buf, "localparam %s = %d;\n", str, now_parser_tree_ir->stage);
			verilog_state = register_verilog(verilog_state, buf);
			free(str);
			count++;
		}
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
	}

	sprintf(buf, "integer __label;\n");
	verilog_state = register_verilog(verilog_state, buf);

	free(buf);

	return 0;
}

/*!
 * @brief	sdiv Verilog　HDL出力
 */
int output_verilog_sdiv(FILE *fp)
{
	PARSER_TREE_IR *now_parser_tree_ir = parser_tree_ir_top;
	int is_sdiv = 0;

    while(now_parser_tree_ir != NULL){
		if(now_parser_tree_ir->flag == PARSER_IR_FLAG_SCALL){
			is_sdiv = 1;
		}
        now_parser_tree_ir = now_parser_tree_ir->next_ptr;
	}

	if(!is_sdiv){
		return 0;
	}

	fprintf(fp, "// sdiv module;\n");
	fprintf(fp, "reg __call_sdiv_req;\n");
	fprintf(fp, "wire __call_sdiv_ready;\n");
	fprintf(fp, "wire __call_sdiv_done;\n");
	fprintf(fp, "reg [31:0] __call_sdiv_args_0;\n");
	fprintf(fp, "reg [31:0] __call_sdiv_args_1;\n");
	fprintf(fp, "wire [31:0] __call_sdiv_result;\n");

	fprintf(fp, "synverll_sdiv_32x32 u_synverll_sdiv_32x32(\n");
	fprintf(fp, "\t.system_clock(__func_clock),\n");
	fprintf(fp, "\t.system_reset(__func_reset),\n");

	fprintf(fp, "\t.__call_sdiv_req(__call_sdiv_req),\n");
	fprintf(fp, "\t.__call_sdiv_ready(__call_sdiv_ready),\n");
	fprintf(fp, "\t.__call_sdiv_done(__call_sdiv_done),\n");
	fprintf(fp, "\t.__call_sdiv_args_0(__call_sdiv_args_0),\n");
	fprintf(fp, "\t.__call_sdiv_args_1(__call_sdiv_args_1),\n");
	fprintf(fp, "\t.__call_sdiv_result(__call_sdiv_result)\n");

	fprintf(fp, ");\n\n");

	return 0;
}
