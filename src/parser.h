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
#ifndef _H_PARSER_
#define _H_PARSER_

#define PARSER_FLAG_NOP             0
#define PARSER_FLAG_INCLUDE_SYS     1
#define PARSER_FLAG_INCLUDE_USER    2
#define PARSER_FLAG_DEFINE          3
#define PARSER_FLAG_SCMODULE        11
#define PARSER_FLAG_SCCTOR          21
#define PARSER_FLAG_SCMETHOD        22
#define PARSER_FLAG_SCTHREAD        23
#define PARSER_FLAG_SENSITIVE       24
#define PARSER_FLAG_PROCESS         25
#define PARSER_FLAG_PROCESS_BODY    26
#define PARSER_FLAG_INOUT           31
#define PARSER_FLAG_SCSIGNAL        41
#define PARSER_FLAG_SIGNAL          42
#define PARSER_FLAG_REQUIREMENT     51
#define PARSER_FLAG_LINE            61

// 構文解析木構造体
typedef struct parser_tree{
    struct parser_tree *prev_ptr;
    struct parser_tree *next_ptr;
    unsigned int    flag;
    int             level;
    char            *str;

//    union {
        // define宣言
        struct define {
            char name[STR_MAX];
            char value[STR_MAX];
        } define;
        // include宣言
        struct include {
            char filename[STR_MAX];
        } include;
        // SC_MODULE
        struct sc_module {
            struct parser_tree *next_ptr;
            char            name[STR_MAX];
            char            port[STR_MAX];
        } sc_module;
        // SC_CTOR
        struct sc_ctor {
            struct parser_tree *next_ptr;
            char            name[STR_MAX];
        } sc_ctor;
        // PORT(in/out/inout)
        struct port {
            char            inout[STR_MAX];
            char            type[STR_MAX];
            char            vector[STR_MAX];
            char            name[STR_MAX];
            unsigned int    reg;
        } port;
        // SIGNAL
        struct signal {
            char            type[STR_MAX];
            char            vector[STR_MAX];
            char            name[STR_MAX];
            unsigned int    reg;
        } signal;
        // process
        struct process {
            char            top[STR_MAX];
            char            name[STR_MAX];
            char            sensitive[STR_MAX];
            char            body[STR_MAX];
        } process;
        // process body
        struct process_body {
            char            entity[STR_MAX];
            char            process[STR_MAX];
            char            reg;
        } process_body;
        // SC_METHOD
        struct sc_method {
            struct parser_tree *next_ptr;
            char            top[STR_MAX];
            char            name[STR_MAX];
            char            sensitive[STR_MAX];
        } sc_method;
        // SC_THREAD
        // SC_CTHREAD
        // Reuirement文(if文、switch文)
        struct requirement {
            struct parser_tree *next_ptr;
            char            requirement[STR_MAX];
            char            body[STR_MAX];
        } requirement;
        // case文
        // switch文
        //struct switch_line {
        //    struct parser_tree *next_ptr;
        //    char            requirement[STR_MAX];
        //    char            body[STR_MAX];
        //} switch_line;
        // sensitive
        struct sensitive {
            char            type[STR_MAX];
            char            name[STR_MAX];
        } sensitive;
        // line
        struct line {
            char            type[STR_MAX];
            char            *body;
        } line;
//    };
} PARSER_TREE;


// parser.c
extern int create_header_source();
extern int create_proc_source();
extern PARSER_TREE * get_parser_tree_current();
extern int insert_parser_tree(char *str);
extern int parser_c_source();
extern int parser_tree_current_top();

#endif
