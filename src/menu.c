#include "menu.h"

unsigned char menuBuf[MENU_WIDTH * MENU_HEIGHT * 4];

void prettyColors()
{
	
	for(int i = 0; i < 272; i++)
	{
		for(int j = 0; j < 480*4; j+=4)
		{
			menuBuf[i * 1920 + j + 0] = 255 * ((272 - i) / 272.0);
			menuBuf[i * 1920 + j + 1] = 255 * ((480 * 4 - j) / (480.0 * 4));
			menuBuf[i * 1920 + j + 2] = 127 * (i / (272.0)) + 128 * (j / (480.0 * 4));
			menuBuf[i * 1920 + j + 3] = 255;
		}
	}
}