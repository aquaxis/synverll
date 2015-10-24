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
 * @file
 * @brief 構文解析
 * @author Hidemi Ishiahra
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

// 構文解析木の深さ
int parser_level = 0;

PARSER_TREE *parser_tree_top        = NULL;
PARSER_TREE *parser_tree_current    = NULL;
PARSER_TREE *parser_tree_prev       = NULL;

/*!
 * @brief	文字列の領域取得と登録
 */
char *charalloc(char *in)
{
	char *buf;

	buf = calloc(strlen(in)+1,1);
	strcpy(buf, in);

	return (char *)buf;
}

/*!
 * @brief 構文解析木の追加
 *
 * 構文解析木を追加します。
 *
 * @param[in] str 追加する文字列
 * @return 無し
 */
int insert_parser_tree(char *str)
{
    // パーサーツリーの新規登録
    if(parser_tree_top == NULL){
        parser_tree_top     = (PARSER_TREE *)malloc(sizeof(PARSER_TREE));
        parser_tree_current = parser_tree_top;
    }else{
        parser_tree_current->next_ptr   = (PARSER_TREE *)malloc(sizeof(PARSER_TREE));
        parser_tree_prev                = parser_tree_current;
        parser_tree_current             = parser_tree_current->next_ptr;
        parser_tree_current->prev_ptr   = parser_tree_prev;
        parser_tree_current->next_ptr   = NULL;
    }

    // パーサーツリーに文字列の登録
	parser_tree_current->str = charalloc(str);
    return 0;
}

/*!
 * @brief 現在の構文解析木の位置を返す
 */
PARSER_TREE * get_parser_tree_current()
{
    return parser_tree_current;
}

/*!
 * @brief 現在の構文解析木の位置を返す
 */
int parser_tree_current_top()
{
    parser_tree_current = parser_tree_top;
    return 0;
}

/*!
 * @brief 関数の解析
 */
int create_proc_source()
{
//    PARSER_TREE *now_parser_tree;
//    PARSER_TREE *parser_tree_process;
//    unsigned int now_level;
    char *token;
    char *line;
    int func_num = 0;
	char *filename;
	FILE *fp;
	int size;
	char *const_header = "#include \"__extern.h\"\n\n\0";

	token = calloc(STR_MAX, 1);
	line = calloc(STR_MAX, 1);

	filename = (char *)malloc(128);
	memset(filename, 0, 128);

    parser_tree_current = parser_tree_top;
    while(parser_tree_current != NULL){
        if(parser_tree_current->flag == PARSER_FLAG_PROCESS){
			sprintf(filename, "__%03d.c", func_num);
			func_num++;
			fp = fopen(filename, "wb+");
			/*
			 * ToDo: fopenのエラー処理が必要
			 */
			size = strlen(parser_tree_current->line.body);
			fwrite(const_header, 1, strlen(const_header), fp);
			fwrite(parser_tree_current->line.body, 1, size, fp);
			fclose(fp);
        }
        parser_tree_current = parser_tree_current->next_ptr;
    }

	free(token);
	free(line);

	free(filename);

    return func_num;
}

/*!
 * @brief 関数以外の解析
 */
int create_header_source()
{
//    PARSER_TREE *now_parser_tree;
//    PARSER_TREE *parser_tree_process;
//    unsigned int now_level;
    char *token;
    char *line;
    int m = 0;
    FILE *fp;
    int size, start;
//    char *filename;
	char *const_extern = "extern \0";
	char *const_define = "#define \0";
	char *const_space = " \0";
	int i;
	char *const_extern_c = "__extern.c\0";
	char *const_extern_h = "__extern.h\0";

	char *const_grmem_func = "void __gr_mem(){\n";

	token = calloc(STR_MAX, 1);

	/*
	 * グローバル変数用ソースコードの生成
	 */
	fp = fopen(const_extern_c, "wb+");
	fwrite(const_grmem_func, 1, strlen(const_grmem_func), fp);
    parser_tree_current = parser_tree_top;
    while(parser_tree_current != NULL){
        if(parser_tree_current->flag == PARSER_FLAG_LINE){
			size = strlen(parser_tree_current->line.body);
			fwrite(parser_tree_current->line.body, 1, size, fp);
			fwrite("\n\n", 1, 2, fp);
        }
        parser_tree_current = parser_tree_current->next_ptr;
    }
	fwrite("}\n", 1, 2, fp);
	fclose(fp);

	/*
	 * externヘッダファイルの生成
	 */
	fp = fopen(const_extern_h, "wb+");
    parser_tree_current = parser_tree_top;

	/*
	 * デファイン文
	 */
    while(parser_tree_current != NULL){
        if(parser_tree_current->flag == PARSER_FLAG_DEFINE){
			fwrite(const_define, 1, strlen(const_define), fp);
			size = strlen(parser_tree_current->define.name);
			fwrite(parser_tree_current->define.name, 1, size, fp);
			fwrite(const_space, 1, strlen(const_space), fp);
			size = strlen(parser_tree_current->define.value);
			fwrite(parser_tree_current->define.value, 1, size, fp);
			fwrite(const_space, 1, strlen(const_space), fp);
			fwrite("\n", 1, 1, fp);
		}
        parser_tree_current = parser_tree_current->next_ptr;
	}

	/*
	 * グローバル変数
	 */
    parser_tree_current = parser_tree_top;
    while(parser_tree_current != NULL){
        if(parser_tree_current->flag == PARSER_FLAG_LINE){
			// 予約語から
			m++;

			line = charalloc(parser_tree_current->line.body);

			// fopenのエラー処理が必要
			size = strlen(line);

			m = 0;
			// "="を見つけたら強制的に";\0"に変換する
			size = strlen(line);
			for(i = 0; i < size; i++){
				if(line[i] == '='){
					line[i] = ';';
					line[i+1] = '\0';
					break;
				}
			}

			// static宣言はextern宣言にして出力する
			// typedefはそのまま出力する
			// その他の宣言はextern化して出力する
			get_token(line, token);
			if(!strcmp(token, "static")){
				fwrite(const_extern, 1, strlen(const_extern), fp);
				start = strlen(const_space);
				size = strlen(line);
				fwrite(&line[start], 1, size-start, fp);
			}else if(!strcmp(token, "typedef")){
				fwrite(token, 1, strlen(token), fp);
				size = strlen(line);
				fwrite(line, 1, size, fp);
			}else{
				fwrite(const_extern, 1, strlen(const_extern), fp);
				fwrite(token, 1, strlen(token), fp);
				size = strlen(line);
				fwrite(line, 1, size, fp);
			}
			fwrite(const_space, 1, strlen(const_space), fp);
			fwrite("\n\n", 1, 2, fp);

			free(line);
        }
        parser_tree_current = parser_tree_current->next_ptr;
    }

	/*
	 * extern関数定義
	 */
    parser_tree_current = parser_tree_top;

    while(parser_tree_current != NULL){
        if(parser_tree_current->flag == PARSER_FLAG_PROCESS){
			// 予約語から
			m++;

			line = charalloc(parser_tree_current->line.body);
			// fopenのエラー処理が必要
			size = strlen(parser_tree_current->line.body);

			m = 0;
			// ")"を見つけたら強制的に";\0"に変換する
			size = strlen(line);
			for(i = 0; i < size; i++){
				if(line[i] == ')'){
					line[i+1] = ';';
					line[i+2] = '\0';
					break;
				}
			}

			// static宣言はextern宣言にして出力する
			// typedefはそのまま出力する
			// その他の宣言はextern化して出力する
			get_token(line, token);
			if(!strcmp(token, "static")){
				fwrite(const_extern, 1, strlen(const_extern), fp);
				start = strlen(const_space);
				size = strlen(line);
				fwrite(&line[start], 1, size-start, fp);
			}else{
				fwrite(const_extern, 1, strlen(const_extern), fp);
				fwrite(token, 1, strlen(token), fp);
				size = strlen(line);
				fwrite(line, 1, size, fp);
			}
			fwrite(const_space, 1, strlen(const_space), fp);
			fwrite("\n\n", 1, 2, fp);

			free(line);
        }
        parser_tree_current = parser_tree_current->next_ptr;
    }

	fclose(fp);

	free(token);

    return 0;
}

/*!
 * @brief 構文解析
 */
int parser_c_source()
{
    char *token;
    TOKEN_ORDER *token_order_now, *token_order_next;
//    PARSER_TREE *now_parser_tree;
    int level = 0;
    int procvalid = 0;
//    int iflevel = 0;
    int i, max, leng;

	token = calloc(STR_MAX, 1);

    parser_level ++;

    printf(" -> Parser for C Source\n");

    while(!read_token(token)){
        insert_parser_tree(token);
        if(!strcmp(token,"#")){
            // 宣言文の処理
            // 宣言文には以下のものがある
            //  #define     : 定義宣言
            //  #include    : インクルードファイル宣言
            read_token(token);
            if(!strcmp(token,"define")){
                // #defineの取得
                // #defineは基本的に取得しない
                // 一行を読み込む
                parser_tree_current->flag = PARSER_FLAG_DEFINE;
                parser_tree_current->level = parser_level;
				read_token(token);
                strcat(parser_tree_current->define.name,token);
                while(strcmp(token,"\n")){
					read_token(token);
					strcat(parser_tree_current->define.value,token);
				}
            }else if(!strcmp(token,"include")){
                // #includeの取得
                // #inculdeの場合、標準ヘッダやシステムで提供されているヘッダは読み込まず、ユーザー定義のインクルードファイルのみ読み込む
                read_token(token);
                if(!strcmp(token,"<")){
                    // 標準インクルード・ヘッダ
                    parser_tree_current->flag = PARSER_FLAG_INCLUDE_SYS;
                    parser_tree_current->level = parser_level;
                    do{
                        read_token(token);
                        if(strcmp(token,">")){
                            strcat(parser_tree_current->include.filename,token);
                        }
                    }while(strcmp(token,">"));
                }else{
                    // ユーザー・インクルード・ヘッダ
                    // ユーザー・インクルード・ヘッダはソースの一部として抜き出し、追加する
                    parser_tree_current->flag   = PARSER_FLAG_INCLUDE_USER;
                    parser_tree_current->level  = parser_level;
                    strcat(parser_tree_current->include.filename, token);

                    //
                    max = strlen(token);
                    for(i=1;i<max-1;i++){
                        token[i-1] = token[i];
                    }
                    token[i-1] = 0x00;
//                    printf(" - include file: %s\n",token);

                    token_order_now     = get_token_order_prev();
                    token_order_next    = get_token_order_current();
                    set_token_order_current(token_order_now);
                    // ユーザー・インクルード・ヘッダの読み込み
                    read_file(token);
                    set_token_order_current_next(token_order_next);
                    set_token_order_current(token_order_now->next_ptr);
                }
            }
        }
		else if((level == 0) && !strcmp(token,"\n")){
			// 行頭で改行コードなら無視する
		}
        else{
			// この時点でフラグは確定しない
            parser_tree_current->flag = PARSER_FLAG_NOP;
            parser_tree_current->level = parser_level;
//            leng = strlen(token) + 1;
            parser_tree_current->line.body = charalloc(token);
//            memset(parser_tree_current->line.body, 0, leng);
//            strcat(parser_tree_current->line.body, token);
			while(1){
                read_token(token);
                if(token == NULL) break;
				leng = strlen(parser_tree_current->line.body) + strlen(token) + 2;
                parser_tree_current->line.body = realloc(parser_tree_current->line.body, leng);
				parser_tree_current->line.body[leng-1] = 0;
				strcat(parser_tree_current->line.body, " ");
				strcat(parser_tree_current->line.body, token);
                if(!strcmp(token,"(")){
					if(level == 0){
						// 関数検出（引数宣言開始）
						procvalid = 1;
					}
				}else if(!strcmp(token,")")){
					// 関数：引数宣言終了
				}else if(!strcmp(token,"{")){
					level++;
				}else if(!strcmp(token,"}")){
					level--;
					if((procvalid == 1) && (level == 0)){
						// 関数確定
						parser_tree_current->flag = PARSER_FLAG_PROCESS;
						break;
					}
				}else if(!strcmp(token,";")){
					if(level == 0){
						if(procvalid == 0){
							// 通常文として確定
							parser_tree_current->flag = PARSER_FLAG_LINE;
						}else{
							// 関数宣言と判断する
							parser_tree_current->flag = PARSER_FLAG_NOP;
						}
							break;
					}
				}
			}
        }
    }
    parser_level --;

    free(token);

    return 0;
}
