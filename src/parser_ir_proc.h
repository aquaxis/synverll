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
#ifndef _H_PARSR_IR_PROC_
#define _H_PARSR_IR_PROC_

extern int clean_proc_tree();
extern char *convname(char *buf);
extern int create_adrs_getelementptr(char *line, char *buf);
extern int create_verilog_call_args(char *name, char *args, char *buf, int is_call);
extern int create_verilog_label();
extern int create_verilog_proc_tree();
extern int create_verilog_state();
extern int get_getelement_type(char *line, char *result);
extern int insert_proc_tree();
extern char *labelalloc(char *name);
extern int output_proc_tree(FILE *fp);
extern int print_proc_tree(FILE *fp);
extern int recall_proc_tree(int stage);
extern int recall_proc_tree_end();
extern char *regalloc(char *name);
extern char * register_verilog(char *body, char *buf);
extern int output_verilog_sdiv(FILE *fp);
extern int output_verilog_return(FILE *fp);

#endif
