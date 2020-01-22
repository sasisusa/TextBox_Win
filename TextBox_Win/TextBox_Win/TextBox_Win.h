#ifndef TEXTBOX_WIN_TEXTBOX_WIN_H_
#define TEXTBOX_WIN_TEXTBOX_WIN_H_

//Libraries: gdi32, comctl32
//Linker flags (gcc): -mwindows

#if defined(_WIN32) || defined(_WIN64)


#if defined(WINVER) || defined(_WIN32_WINNT)
	#if (WINVER < _WIN32_WINNT_WINXP) || (_WIN32_WINNT < _WIN32_WINNT_WINXP)
		#error Windows version to old
	#endif
#endif


int FreeAllocSpace(void *);
char *GetFileStringContent(const char *, size_t *);
char *StrConvAndAllocToWinNewline(const char *, size_t *);


////////////////////////////////////////////////////////////////////////////
//
//	TextBox
//
//	iWidth < TBWIN_WIDTH_MIN || iHeight < TBWIN_HEIGHT_MIN => auto-sized window
//
//  Returns:
//		> 0		on failure
//		0		on success
//
int TextBox(const char *, const char *, int, int);


#endif

#endif /* TEXTBOX_WIN_TEXTBOX_WIN_H_ */
