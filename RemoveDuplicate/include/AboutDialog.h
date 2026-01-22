#pragma once

#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void ShowAboutDialog(HWND hwndParent);

#ifdef __cplusplus
}
#endif