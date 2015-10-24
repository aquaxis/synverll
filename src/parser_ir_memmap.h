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
#ifndef _H_PARSR_IR_MEMMAP_
#define _H_PARSR_IR_MEMMAP_

extern int clean_memmap_tree();
extern unsigned int get_adrs_memmap(char *module, char *label);
extern unsigned int get_size_memmap(char *module, char *label);
extern int insert_memmap_tree();
extern int is_memmap_tree(char *module, char *label);
extern int print_memmap_tree(FILE *fp);
extern int register_memmap_tree(char *module, char *label, int size);

#endif
