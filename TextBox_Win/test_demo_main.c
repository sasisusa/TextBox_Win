#include <windows.h>
#include <stdio.h>

#include "TextBox_Win/TextBox_Win.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char *s;

	s = GetFileStringContent("ExampleText.txt", NULL);
	if(s){
		printf("TextBox: %d\n", TextBox("Titel: Show ExampleText", s, 0, 0));
		FreeAllocSpace(s);
	}
	else{
		puts("Error: GetFileStringContent()");
	}

	return 0;
}
