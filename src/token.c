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
 * @file	token.c
 * @brief	トークンを取得し整理する関数郡
 *
 * @author	Hidemi Ishiahra
 * @date	2007/12/05
 * @date	2008/09/24
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "synverll.h"
#include "token.h"

TOKEN_ORDER *token_order_top        = NULL;
TOKEN_ORDER *token_order_current    = NULL;
TOKEN_ORDER *token_order_prev       = NULL;

/*!
 * @brief 漢字チェック
 *
 * @param[in] code 漢字チェックするコードを入力する。
 * @return コードが漢字でない場合は0を返す。
 */
int iskanji(char *code)
{
    if((*code >= 0xa1) && (*code <= 0xfe)){
        // EUCの場合
        return 1;
    }else if(((*code >= 0x81) && (*code <= 0x9F)) || ((*code >= 0xe0) && (*code <= 0xfc))){
        // SJISの場合
        (*code)++;
        // SJISの2Byte目を判断する
        if((*code >= 0x40) && (*code <= 0x7e)){
            // 正常なSJISコード
            return 2;
        }else{
            // 異常なSJISコード
            return 3;
        }
    }else{
        return 0;
    }
}

/*!
 * @brief ファイルから1行取得
 *
 * @param[out] line ファイルから取得した文字列を返すポインタ
 * @param[in] fp 文字列を取得するファイルのポインタ
 * @return 正常は0です。ファイルがない場合は-1は0です。
 */
int get_line(char *line, FILE *fp)
{
//	int leng;
//	char buf[STR_MAX];

    if(*line == '\0'){
        if(fgets(line, STR_MAX, fp) == NULL){
            return -1;
        }
#if 0
        strcpy(buf, line);
        while(buf[strlen(buf)-1] == '\n' || buf[strlen(buf)-1] == '\r'){
	        buf[strlen(buf)-1] = '\0';
		}
#endif
    }
    return 0;
}

/*!
 * @brief 字句解析
 *
 * 字句解析を行い、トークンを拾います。
 *
 * @param[in] line 入力する文字列
 * @param[out] token 文字列から取得できた最初のトークン
 * @return 常に0です。
 */
int get_token(char *line, char *token)
{
    char *p,*ptk,str;
    char *pp;

    // ポインタの初期化
    p   = line;
    ptk = token;

    // 空白かタブの場合、読み飛ばす
    while(*p == ' ' || *p == '\t') ++p;
    // バッファが空の場合は終了する
    if(*p == '\0'){
		pp = malloc(strlen(p)+1);	// 残りの文字列をlineに戻す
		strcpy(pp,p);
		strcpy(line,pp);
		free(pp);
        token[0] = '\0';
        return 1;
    }

    if(iskanji(p)){                // 漢字の判定
        do{
            *ptk++ = *p++;          // 1バイト目
            if(!iskanji(p)){       // 2バイト目
                //printf("can't decode 2Byte Character(KANJI).\n");
                //exit(1);
            }
            //*ptk++ = *p++;
        }while(iskanji(p) && *p != '\0');
        strcpy(token, "KANJI");     // 漢字を検出した場合はポインタを戻して、NULL文字とする
    }else if(isalpha(*p)){          // 1文字目は英字、2文字目以降は英数字
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_') && *p !='\0');    // 2文字目以降は英数字、又は"_"である
    }else if(*p == '0'){                                    // 数字(0で始まる数字)
        *ptk++ = *p++;
        if(*p == 'x' || *p == 'X'){
            do{
                *ptk++ = *p++;
            }while((isdigit(*p) || *p =='.' ||
                            (*p >= 'a' && *p <= 'f') ||
                            (*p >= 'A' && *p <= 'F')) && *p !='\0');
        }else if(isdigit(*p)){
            do{
                *ptk++ = *p++;
            }while((isdigit(*p) || *p =='.') && *p !='\0');
        }else if(*p == '.'){	// 小数点以下の数値
            do{
                *ptk++ = *p++;
            }while((isdigit(*p) || *p =='.') && *p !='\0');
        }
    }else if(isdigit(*p)){      // 数字(0以外で始まる数字)
        do{
            *ptk++ = *p++;
        }while((isdigit(*p) || *p =='.') && *p !='\0');
    }else if(*p == '\'' || *p == '"'){  // 文字・文字列定数: ', ", '', "",
        str = *p;
        do{
            *ptk++ = *p++;
        }while(*p != str && *p !='\0');
        *ptk++ = *p++;
    }else if(*p == '*'){    // 乗算、コメント: *, *=, **, */
        str = *p;
        *ptk++ = *p++;
        if(*p == str || *p == '/' || *p == '='){
            *ptk++ = *p++;
        }else{
			// ポインター
			do{
				*ptk++ = *p++;
			}while((isalnum(*p) || *p == '_') && *p !='\0');    // 2文字目以降は英数字、又は"_"である
		}
    }else if(*p == '+'){    // 加算: +, ++, +=
        str = *p;
        *ptk++ = *p++;
        if(*p == str || *p == '='){
            *ptk++ = *p++;
        }
    }else if(*p == '-'){    // 減算: -, --, ->, -=
        str = *p;
        *ptk++ = *p++;
        if(*p == str || *p == '>' || *p == '='){
            *ptk++ = *p++;
        }
    }else if(*p == '/'){    // 除算、コメント: /, /=, //, /*
        str = *p;
        *ptk++ = *p++;
        if(*p == str || *p == '*' || *p == '='){
            *ptk++ = *p++;
        }
    }else if(*p == '&' || *p == '|'){   // 論理演算子: &, |, &&, ||, &=, |=
        str = *p;
        *ptk++ = *p++;
        if(*p == str || *p == '='){
            *ptk++ = *p++;
        }
    }else if(*p == '>' || *p == '<'){   // 比較、シフト: >, <, >>, <<, >=, <=
        str = *p;
        *ptk++ = *p++;
        if(*p == str || *p == '='){
            *ptk++ = *p++;
        }
    }else if(*p == '!'){    // 等しくない: !, !=
        str = *p;
        *ptk++ = *p++;
        if(*p == '='){
            *ptk++ = *p++;
        }
    }else if(*p == '='){    // 等しい: =, ==
        str = *p;
        *ptk++ = *p++;
        if(*p == str){
            *ptk++ = *p++;
        }
    }else{    // その他の文字
		*ptk++ = *p++;
	}
    *ptk = '\0';            // 文字の終端
    pp = malloc(strlen(p)+1);	// 残りの文字列をlineに戻す
    strcpy(pp,p);
    strcpy(line,pp);
    free(pp);
    return 0;
}

/*!
 * @brief トークンの整理
 *
 * コメント行やスペース、空行、リターンなどを削除し、必要なトークンだけを残します。
 *
 * @param[in] line 入力文字列
 * @param[out] token 取得できた最初のトークン
 * @param[in] fp ファイルポインタ
 * @return 常に0です。
 */
int insert_token_order(char *line, char *token, FILE *fp)
{
//    TOKEN_ORDER *new_token_order;

    if(!strcmp(token,"//")){
        // //のコメント行は行末までコメントなので読み飛ばす
        do{
            get_line(line, fp);
            get_token(line, token);
        }while(strcmp(token,"\n"));
    }else if(!strcmp(token,"/*")){
        // /*のコメント行は*/までコメントなので読み飛ばす
        do{
            get_line(line, fp);
            get_token(line, token);
        }while(strcmp(token,"*/"));
        // 改行の場合は何もしない
    }else if(!strcmp(token,"\r")){
        // 改行の場合は何もしない
    }else if(!strcmp(token,"")){
        // トークンに何もない場合は何も処理しない
    }else{
        // トークンオーダーの新規取得
        create_token_order();

        // トークンオーダーにトークンを登録する
        token_order_current->str = (char *)malloc(strlen(token)+1);
        strcpy(token_order_current->str, token);
    }
    return 0;
}

/*!
 * @brief トークン・オーダーの生成
 *
 * @param[in]
 */
int create_token_order()
{
    // トークンオーダーの生成
    if(token_order_top == NULL){
        // 先頭トークンオーダーが空の場合、新規取得したトークンオーダーをセットする
        token_order_top = (TOKEN_ORDER *)malloc(sizeof(TOKEN_ORDER));
        token_order_current = token_order_top;
    }else{
        token_order_current->next_ptr = (TOKEN_ORDER *)malloc(sizeof(TOKEN_ORDER));
        token_order_prev    = token_order_current;
        token_order_current = token_order_prev->next_ptr;
    }
    token_order_current->next_ptr   = NULL;
    token_order_current->str        = NULL;
    return 0;
}

/*!
 * @brief トークン・オーダーの削除
 *
 * @param[in]
 */
int clean_token_order()
{
    TOKEN_ORDER *new_token_order;

    token_order_current = token_order_top;
    while(1){
        if(token_order_current == NULL) break;
        new_token_order = token_order_current->next_ptr;
        free(token_order_current);
        token_order_current = new_token_order;
    }
    token_order_top = NULL;
    return 0;
}

/*!
 * @brief トークン・オーダーからトークンの取得
 *
 * @param[in]
 */
int read_token(char *token)
{
    if(token_order_current != NULL){
        strcpy(token, token_order_current->str);
        token_order_prev    = token_order_current;
        token_order_current = token_order_current->next_ptr;
        return 0;
    }else{
        *token = '\0';
        return 1;
    }
}

/*!
 * @brief 現在のトークンオーダーの位置を返す
 */
TOKEN_ORDER * get_token_order_current()
{
    return token_order_current;
}

/*!
 * @brief トークンオーダーの先頭位置を返す
 */
TOKEN_ORDER * get_token_order_top()
{
    return token_order_top;
}

/*!
 * @brief 前回のトークンオーダーの位置を返す
 */
TOKEN_ORDER * get_token_order_prev()
{
    return token_order_prev;
}

/*!
 * @brief 現在のトークンオーダーの位置をセットする
 */
int set_token_order_current(TOKEN_ORDER * token_order)
{
    token_order_current = token_order;
    return 0;
}

/*!
 * @brief 現在のトークンオーダーのネクストポインタをセットする
 */
int set_token_order_current_next(TOKEN_ORDER * token_order)
{
    token_order_current->next_ptr = token_order;
    return 0;
}

/*!
 * @brief 前回のトークンオーダーのネクストポインタをセットする
 */
int set_token_order_prev_next(TOKEN_ORDER * token_order)
{
    token_order_prev->next_ptr = token_order;
    return 0;
}

/*!
 * @brief ファイルの読み込み
 *
 * ファイルを読み込みます。
 *
 * @param[in] filename 読み込むファイル名
 * @return 常に0です。
 */
int read_file(char *filename)
{
    FILE *fp;
    char *line, *token;

	line = malloc(STR_MAX);
	token = malloc(STR_MAX);

	memset(line, 0, STR_MAX);
	memset(token, 0, STR_MAX);

    printf("Open Filename: %s\n",filename);
    if((fp =fopen(filename,"rb")) == NULL){
        printf("can't open file.\n");
		free(line);
		free(token);
        exit(1);
    }
    while(1){
        get_line(line, fp);
        get_token(line, token);
        if(*token == '\0') break;
        insert_token_order(line, token, fp);    // トークンを追加する
    }
    fclose(fp);

	free(line);
	free(token);

    return 0;
}

/*!
 * @brief 字句解析(LLVM-IR用)
 *
 * 字句解析を行い、トークンを拾います。
 *
 * @param[in] line 入力する文字列
 * @param[out] token 文字列から取得できた最初のトークン
 * @return	常に0です。
 */
int get_token_llvm(char *line, char *token)
{
    char *p,*ptk,str;
    char *pp;

    // ポインタの初期化
    p   = line;
    ptk = token;

    // 空白かタブの場合、読み飛ばす
    while(*p == ' ' || *p == '\t') ++p;
    // バッファが空の場合は終了する
    if(*p == '\0'){
		pp = malloc(strlen(p)+1);	// 残りの文字列をlineに戻す
		strcpy(pp,p);
		strcpy(line,pp);
		free(pp);
        token[0] = '\0';
        return 1;
    }

    if(iskanji(p)){                // 漢字の判定
        do{
            *ptk++ = *p++;          // 1バイト目
            if(!iskanji(p)){       // 2バイト目
                //printf("can't decode 2Byte Character(KANJI).\n");
                //exit(1);
            }
            //*ptk++ = *p++;
        }while(iskanji(p) && *p != '\0');
        strcpy(token, "KANJI");     // 漢字を検出した場合はポインタを戻して、NULL文字とする
    }else if(*p == 'c' && *(p+1) == '"'){
		*ptk++ = *p++;
		// c"で始まる(これはキャラクター宣言である)
		do{
			*ptk++ = *p++;
		}while(*p != '"' && *p !='\0');
		*ptk++ = *p++;
    }else if(isalpha(*p)){          // 1文字目は英字、2文字目以降は英数字
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_' || *p == '*' || *p =='.') && *p !='\0');    // 2文字目以降は英数字、又は"_"である
    }else if(*p == '0'){                                    // 数字(0で始まる数字)
        *ptk++ = *p++;
        if(*p == 'x' || *p == 'X'){
            do{
                *ptk++ = *p++;
            }while((isdigit(*p) || *p =='.' ||
                            (*p >= 'a' && *p <= 'f') ||
                            (*p >= 'A' && *p <= 'F')) && *p !='\0');
        }else if(isdigit(*p)){
            do{
                *ptk++ = *p++;
            }while((isdigit(*p) || *p =='.') && *p !='\0');
        }else if(*p == '.'){	// 小数点以下の数値
            do{
                *ptk++ = *p++;
            }while((isdigit(*p) || *p =='.') && *p !='\0');
        }
    }else if(isdigit(*p)){      // 数字(0以外で始まる数字)
        do{
            *ptk++ = *p++;
        }while((isdigit(*p) || *p =='.') && *p !='\0');
    }else if(*p == '-'){      // マイナス値
        do{
            *ptk++ = *p++;
        }while((isdigit(*p) || *p =='.') && *p !='\0');
    }else if(*p == '.'){		// ラベル名
        str = *p;
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_' || *p == '.') && *p !='\0');
    }else if(*p == '@'){
        str = *p;
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_' || *p == '.') && *p !='\0');
    }else if(*p == '%'){
        str = *p;
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_' || *p == '.' || *p == '*') && *p !='\0');
    }else if(*p == '!'){
        str = *p;
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_' || *p == '.') && *p !='\0');
    }else if(*p == '#'){
        str = *p;
        do{
            *ptk++ = *p++;
        }while((isalnum(*p) || *p == '_' || *p == '.') && *p !='\0');
    }else if(*p == '='){    // 等しい: =, ==
        str = *p;
        *ptk++ = *p++;
        if(*p == str){
            *ptk++ = *p++;
        }
    }else if(*p == '>' || *p == ']'){    // ポインタ: >*, ]*
        str = *p;
        *ptk++ = *p++;
        if(*p == '*'){
            *ptk++ = *p++;
        }
    }else if(*p == '.'){    // ...
        str = *p;
        *ptk++ = *p++;
        do{
            *ptk++ = *p++;
        }while((*p == str) && *p !='\0');
    }else if(*p == '\\'){
        do{
            *ptk++ = *p++;
        }while(isalnum(*p) && *p !='\0');    // 2文字目以降は英数字
    }else if((*p == '\r') || (*p == '\n')){    // 改行
        str = '\0';
        *ptk++ = *p++;
    }else{    // その他の文字
		*ptk++ = *p++;
	}
    *ptk = '\0';            // 文字の終端
    pp = malloc(strlen(p)+1);	// 残りの文字列をlineに戻す
    strcpy(pp,p);
    strcpy(line,pp);
    free(pp);
    return 0;
}

/*!
 * @brief	文字列解析(LLVM-IR用)
 *
 * @note
 * c"..."で括られている文字列から1文字の返す
 */
int get_chara_llvm(char *line, char *token)
{
    char *p, *ptk, *pp;

    // ポインタの初期化
    p   = line;
    ptk = token;

    *ptk = 0;

    // cか"の場合、読み飛ばす
    while(*p == 'c' || *p == '"') ++p;
    // バッファが空の場合は終了する
    if(*p == '\0'){
		pp = malloc(strlen(p)+1);	// 残りの文字列をlineに戻す
		strcpy(pp,p);
		strcpy(line,pp);
		free(pp);
        token[0] = '\0';
        return 1;
    }

    if(*p == '"'){
		// "は終了
		*ptk = 0;
		return -1;
    }else if(*p == '\\'){
        (*p)++;
		// \はこの後に2文字16進数が続く
		/*
		 * ToDo:
		 *   isxdigit(*p)でエラー処理を入れる
		 */
        *ptk = *p++;
		*ptk <<= 4;	// 16倍する
        *ptk = *p++;
    }else{	// それ以外は一文字のキャラクターとして処理する
        *ptk = *p++;
	}
    pp = malloc(strlen(p)+1);	// 残りの文字列をlineに戻す
    strcpy(pp,p);
    strcpy(line,pp);
    free(pp);
	return 0;
}
