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
#ifndef _H_PARSER_IR_
#define _H_PARSER_IR_

typedef struct inst_stage{
	int parallel;
	int stage;
} INST_STAGE;

/*!
 * LLVM-IRの命令種類
 */
enum PARSER_IR_FLAG{
	PARSER_IR_FLAG_UNKNOWN,
	PARSER_IR_FLAG_REGISTER,
	PARSER_IR_FLAG_REGISTER_LOOP,
	PARSER_IR_FLAG_WIRE,
	PARSER_IR_FLAG_TYPE,
	PARSER_IR_FLAG_STORE,
	PARSER_IR_FLAG_ARRAY,
	PARSER_IR_FLAG_POINTER,
	PARSER_IR_FLAG_LOAD,
	PARSER_IR_FLAG_ALLOCA,
	PARSER_IR_FLAG_SELECT,
	PARSER_IR_FLAG_FLOAT,
	PARSER_IR_FLAG_CLOSE,
	PARSER_IR_FLAG_COMPARE,
	PARSER_IR_FLAG_LABEL,
	PARSER_IR_FLAG_MODULE,
	PARSER_IR_FLAG_CALL,
	PARSER_IR_FLAG_BRANCH,
	PARSER_IR_FLAG_RETURN,
	PARSER_IR_FLAG_SCALL,
	PARSER_IR_FLAG_NONE,
};

typedef struct parser_tree_ir{
    struct parser_tree_ir *prev_ptr;
    struct parser_tree_ir *next_ptr;
    enum PARSER_IR_FLAG			flag;
    int             level;
    int				parallel;		// パラレルステージ
    int				stage;			// ステージ
    int				prev_parallel;	// 元のパラレルステージ
    int				prev_stage;		// 元のステージ
    char            *str;
    char			label[STR_MAX];	// ラベル
    char			label_type[STR_MAX];
    char			verilog[STR_MAX];

	// モジュール宣言
	struct module {
		char		name[STR_MAX];
		char		result_type[STR_MAX];
		char		argument[STR_MAX];
	} module;
	struct type {
		char		name[STR_MAX];
	} type;
	// モジュール宣言
	struct call {
		char		name[STR_MAX];
		char		result_type[STR_MAX];
		char		argument[STR_MAX];
	} call;
	// レジスター
	struct reg {
		char		name[STR_MAX];
		int			instruction;
		char		input_left_type[STR_MAX];
		char		input_left[STR_MAX];
		char		input_right_type[STR_MAX];
		char		input_right[STR_MAX];
	} reg;
	// 外部変数
	struct array {
		char		type[STR_MAX];
		char		name[STR_MAX];
	} array;
	// ブランチ
	struct branch {
		char		condition_type[STR_MAX];
		char		condition_value[STR_MAX];
		char		branch_true[STR_MAX];
		char		branch_false[STR_MAX];
	} branch;
	// セレクト
	struct select {
		char		condition_type[STR_MAX];
		char		condition_value[STR_MAX];
		char		select_true[STR_MAX];
		char		select_false[STR_MAX];
	} select;
	// リターン
	struct ret {
		char		type[STR_MAX];
		char		name[STR_MAX];
	} ret;
	struct alloca {
		char		type[STR_MAX];
		char		init[STR_MAX];
	} alloca;
	struct load {
		char		type[STR_MAX];
		char		name[STR_MAX];
	} load;
	struct pointer {
		char		str[STR_MAX];
		char		type[STR_MAX];
		char		name[STR_MAX];
		char		init_type[STR_MAX];
		char		init_name[STR_MAX];
		char		add_type[STR_MAX];
		char		add_name[STR_MAX];
	} pointer;
	struct comp {
		char		name[STR_MAX];
		char		value[STR_MAX];
		char		input_left_type[STR_MAX];
		char		input_left[STR_MAX];
		char		input_right_type[STR_MAX];
		char		input_right[STR_MAX];
	} comp;
	struct phi {
		char		type[STR_MAX];
		char		argument[STR_MAX];
	} phi;
} PARSER_TREE_IR;

/*!
 * 信号の種類
 */
enum SIGNAL_FLAG{
	SIGNAL_FLAG_UNKNWON,	/* 不明な信号 */
	SIGNAL_FLAG_REG,		/* レジスタ */
	SIGNAL_FLAG_WIRE,		/* ワイヤ */
	SIGNAL_FLAG_RAM,		/* グローバルメモリ*/
	SIGNAL_FLAG_ALLOCA,		/* ALLOCA */
	SIGNAL_FLAG_RETURN,		/* RETURN(戻り値) */
};

#define sep_p(x)	(x[0]=='%' || x[0]=='@')?&x[1]:x

// parser_ir.c
extern int clean_parser_tree_ir();
extern int create_stage_parser_tree();
extern int get_irtype_llvm(char *line, char *token);
extern PARSER_TREE_IR * get_parser_tree_ir_current();
extern int get_pointer_llvm(char *line, char *token);
extern int get_stage_label(char *label);
extern int insert_parser_tree_ir(char *str);
extern int is_pointer_type(char *name);
extern int is_register_name(char *name);
extern int parser_ir_source(char *buf);
extern int parser_tree_ir_current_top();
extern int print_parser_tree_ir(FILE *fp);
extern INST_STAGE search_args_llvm(PARSER_TREE_IR *now_parser_tree_ir, char *name, char *args);
extern INST_STAGE search_maxstage_llvm(PARSER_TREE_IR *now_parser_tree_ir);
extern int search_module_llvm(char *name, char *args);
extern INST_STAGE search_proc_llvm(PARSER_TREE_IR *now_parser_tree_ir, int flag);
extern INST_STAGE search_reg_llvm(PARSER_TREE_IR *now_parser_tree_ir, char *name, char *left_name, char *right_name);

extern PARSER_TREE_IR *parser_tree_ir_top;
extern PARSER_TREE_IR *parser_tree_ir_current;
extern PARSER_TREE_IR *parser_tree_ir_prev;

#endif
