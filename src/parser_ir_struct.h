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
#ifndef _H_PARSR_IR_STRUCT_
#define _H_PARSR_IR_STRUCT_

extern int clean_struct_tree();
extern int get_struct_argument(char *label, char *argument);
extern int insert_struct_tree();
extern int get_struct_argument(char *label, char *argument);
extern int parser_struct_tree();
extern int register_struct_tree(char *label, char *argument);
extern int is_struct_name(char *label);

#endif
