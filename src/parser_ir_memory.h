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
#ifndef _H_PARSR_IR_MEMORY_
#define _H_PARSR_IR_MEMORY_

/*!
 * メモリの種類
 */
enum MEMORY_FLAG{
	MEMORY_FLAG_UNKNWON,	/* 不明なメモリ */
	MEMORY_FLAG_GLOBAL,		/* グローバルメモリ */
	MEMORY_FLAG_LOCAL,		/* ローカルメモリ */
	MEMORY_FLAG_POINTER,	/* ポインター宣言 */
	MEMORY_FLAG_REGISTER,	/* 変数 */
	MEMORY_FLAG_RETURN,		/* 戻り値 */
};

typedef struct memory_tree{
    struct memory_tree	*prev_ptr;
    struct memory_tree	*next_ptr;
    enum MEMORY_FLAG	flag;			// フラグ
    char				*label;			// ラベル
    char				*type;			// タイプ
    char				*verilog_args;	// 引数
    char				*verilog_proc;	// 動作
    char				*verilog_decl;	// 宣言
	int					num;			// 引数の番号
    int 				size;			// サイズ
} MEMORY_TREE;

typedef struct module_tree{
    struct module_tree	*prev_ptr;
    struct module_tree	*next_ptr;
    char				*module_name;		// ラベル
    struct memory_tree	*memory_tree_ptr;	// メモリツリーのポインタ
    struct call_tree	*call_tree_ptr;		// CALLツリーのポインタ
} MODULE_TREE;

typedef struct module_stack{
    struct module_stack	*prev_ptr;
    struct module_stack	*next_ptr;
    char				*module_name;		// ラベル
    struct module_tree	*module_tree_ptr;	// メモリツリーのポインタ
} MODULE_STACK;

extern char *module_name;

extern int __get_array_size(char *type, char *ptr, char *rslt_ptr);
extern int __get_struct_size(char *type, char *ptr, char *rslt_ptr);
extern int clean_memory_tree(MEMORY_TREE *memory_tree_ptr);
extern int clean_module_tree();
extern int create_array_size();
extern int deploy_array_type();
extern int get_memory_size(char *label, char *ptr, char *rslt_ptr);
extern int insert_memory_tree();
extern int insert_module_tree();
extern int new_memory_tree();
extern int output_memory_tree(FILE *fp);
extern int output_memory_tree_decl(FILE *fp);
extern int output_memory_tree_proc(FILE *fp);
extern int parser_memory_tree();
extern int print_array_type();
extern int print_module_tree(FILE *fp);
extern int register_module_tree(char *module_name);
extern int search_module_tree(char *module, char label);
extern int new_module_stack();

extern MEMORY_TREE *memory_tree_top;
extern MEMORY_TREE *memory_tree_current;
extern MEMORY_TREE *memory_tree_prev;

extern MODULE_TREE *module_tree_top;
extern MODULE_TREE *module_tree_current;
extern MODULE_TREE *module_tree_prev;

extern MODULE_STACK *module_stack_top;

#endif
