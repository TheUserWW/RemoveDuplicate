#include "../include/Resource.h"
#include "../include/AboutDialog.h"

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
    {
        HICON hIcon = LoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_REMOVEDUPLICATE));
        SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        const wchar_t* licenseText = L"This program is free software: you can redistribute it and/or modify\r\n"
            L"it under the terms of the GNU General Public License as published by\r\n"
            L"the Free Software Foundation, either version 3 of the License, or\r\n"
            L"(at your option) any later version.\r\n"
            L"\r\n"
            L"This program is distributed in the hope that it will be useful,\r\n"
            L"but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n"
            L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n"
            L"GNU General Public License for more details.\r\n"
            L"\r\n"
            L"You should have received a copy of the GNU General Public License\r\n"
            L"along with this program.  If not, see <https://www.gnu.org/licenses/>.";
        
        SetDlgItemTextW(hDlg, IDC_EDIT_LICENSE, licenseText);
        
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowAboutDialog(HWND hwndParent) {
    DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_ABOUTBOX), hwndParent, AboutDialogProc);
}