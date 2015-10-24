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
#ifndef _H_SYNVERLL_
#define _H_SYNVERLL_

#define STR_MAX 1024 // 1行の最大長

#define REVISION_MAIN    0
#define REVISION_MANOR   1

extern void help();
extern int main(int argc,char *argv[]);
extern int print_verilog_if();
extern int process(char *csource);
extern int process_function(char *name);
extern int split_c_source(char *filename);

#endif
