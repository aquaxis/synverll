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
 * @file	common.c
 * @brief	���ʊ֐�
 * @author	Hidemi Ishiahra
 *
 * @note
 * ���ʊ֐���u��
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

/*!
 * @brief	�r�b�g�T�C�Y��Ԃ�
 */
int get_width(char *buf)
{
	int size = 0;
	
	if(!strcmp(buf, "void")){
		size = 0;
	}else if(!strcmp(buf, "i1")){
		size = 1;
	}else if(!strcmp(buf, "i8")){
		size = 8;
	}else if(!strcmp(buf, "i16")){
		size = 16;
	}else if(!strcmp(buf, "i32")){
		size = 32;
	}else if(buf[strlen(buf)-1] == '*'){
		// �|�C���^�錾(�|�C���^�͍Ō��'*'���t��)
		// �|�C���^��i8*, i16*, i32*, %�\���̖�*, ARRAY�錾*, ��z��
		size = 32;
	}else{
		/*
		 * ToDo:
		 * �����Ə������邱�ƁB���܂̂Ƃ���b�肾��B
		 */
		size = 32;
		printf("[WRANING] get_width(): %s\n", buf);
	}

	return size;
}

/*!
 * @brief	������̗̈�擾�Ɠo�^
 */
char *charalloc(char *in)
{
	char *buf;

	buf = calloc(strlen(in)+1,1);
	strcpy(buf, in);

	return (char *)buf;
}

/*!
 * @brief	�ϐ����̕ϊ�
 *
 * @note
 * �ϐ����̐擪��'%', '@'����菜��
 * �ϐ���'.'��'_'�ɒu��������
 */
char *convname(char *buf)
{
	int i;
	if((buf[0] == '%') || (buf[0] == '@')){
		for(i=0;i<strlen(buf)-1;i++){
			buf[i] = buf[i+1];
		}
		buf[strlen(buf)-1] = 0;
	}
	
	for(i=0;i<strlen(buf);i++){
		if(buf[i] == '.') buf[i] = '_';
	}

	return buf;
}

/*!
 * @brief	�^�C�v�̕ϊ�
 *
 * @note
 * �^�C�v�̖�����'*'���폜����
 */
char *convtype(char *buf)
{
	if(buf[strlen(buf)-1] == '*'){
		buf[strlen(buf)-1] = 0;
	}
	return buf;
}

/*
char *sep_p(char *name){
	if(name[0] == '%'){
printf("%s\n", &name[1]);
		return &name[1];
	}else{
		return name;
	}
}
*/
