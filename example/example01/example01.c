/*
* Copyright (C)2015 H.Ishihara
*
* License: The Open Software License 3.0
* License URI: http://www.opensource.org/licenses/OSL-3.0
*
* For further information please contact.
*	http://www.aquaxis.com/
*	info(at)aquaxis.com or hidemi(at)sweetcafe.jp
*/
/*!
 * @note
 * LEDチカチカのようなプログラムです。
 */
void example01(char *led){
	int i = 0;
	while(1){
		if( i == 5000000 ){
			i = 0;
			led[0] = ~led[0];
		}else{
			i++;
		}
	}
}
