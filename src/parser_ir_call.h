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
#ifndef _H_PARSR_IR_CALL_
#define _H_PARSR_IR_CALL_

typedef struct call_signal_tree{
	struct call_signal_tree	*prev_ptr;
	struct call_signal_tree	*next_ptr;

    char				*signal_name;		// 信号名(引数名)
    char				*verilog;
    int					inout;				// 0:in,1:out
    int					size;				// 変数サイズ
    int					num;				// 引数番号
} CALL_SIGNAL_TREE;

typedef struct call_tree{
    struct call_tree	*prev_ptr;
    struct call_tree	*next_ptr;
    char				*call_name;		// CALL名
    char				*verilog;		// 引数
    struct call_signal_tree	*signal_top_ptr;
} CALL_TREE;

extern int clean_call_tree();
extern int insert_call_signal_tree();
extern int insert_call_tree();
extern int is_call_tree(char *call_name);
extern int output_call_signal_tree(FILE *fp);
extern int print_call_tree();
extern int register_call_signal_tree(char *call_name, char *signal_name, int num, int size, int inout);
extern int register_call_tree(char *call_name);

extern CALL_TREE *call_tree_top;
extern CALL_TREE *call_tree_current;
extern CALL_TREE *call_tree_prev;

extern CALL_SIGNAL_TREE *call_signal_tree_top;
extern CALL_SIGNAL_TREE *call_signal_tree_current;
extern CALL_SIGNAL_TREE *call_signal_tree_prev;

#endif
