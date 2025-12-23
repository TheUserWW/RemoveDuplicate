#include "RemoveDuplicate.h"
#include "Resource.h"
#include "filter.h"
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wincrypt.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")

// 全局变量定义
HWND g_hwnd;
HWND g_hEditFolder;
HWND g_hBtnBrowse;
HWND g_hBtnScan;
HWND g_hBtnDelete;
HWND g_hListView;  // 改为ListView控件以支持勾选框
HWND g_hProgress;
HWND g_hStatus;
HWND g_hBtnSelectAll;  // 全选按钮
HWND g_hBtnDeselectAll;  // 取消全选按钮
std::wstring g_selectedFolder;
std::map<std::wstring, std::vector<std::wstring>> g_duplicateFiles;
std::map<int, std::wstring> g_listItemToFilePath;  // 修改为ListView相关
std::map<int, std::wstring> g_groupHeaderToMD5;  // 组标题到MD5的映射
std::set<std::wstring> g_checkedGroups;  // 已勾选的组

// 过滤相关全局变量定义
bool g_filterBySizeEnabled = false;
bool g_filterByTypeEnabled = false;
bool g_filterByDateEnabled = false;
bool g_filterByPathEnabled = false;
long long g_minFileSize = 0;
long long g_maxFileSize = LLONG_MAX;
std::wstring g_fileTypeFilter;
std::wstring g_dateFilterFrom;
std::wstring g_dateFilterTo;
std::wstring g_pathFilter;

// 加载字符串资源
std::wstring LoadStringResource(UINT id) {
    wchar_t buffer[256];
    LoadStringW(GetModuleHandle(NULL), id, buffer, 256);
    return std::wstring(buffer);
}

// 初始化ListView控件
void InitListView() {
    // 设置ListView样式，添加LVS_EX_INFOTIP以支持缩进
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);

    // 设置图像列表（用于缩进）
    HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 0);
    if (hImageList) {
        ListView_SetImageList(g_hListView, hImageList, LVSIL_SMALL);
    }

    // 添加列
    LVCOLUMNW lvc = { 0 };
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    // 第一列：文件名
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 400;
    lvc.pszText = (LPWSTR)L"文件名";
    lvc.iSubItem = 0;
    ListView_InsertColumn(g_hListView, 0, &lvc);

    // 第二列：路径
    lvc.cx = 300;
    lvc.pszText = (LPWSTR)L"路径";
    lvc.iSubItem = 1;
    ListView_InsertColumn(g_hListView, 1, &lvc);
}

// 全选所有组
void SelectAllGroups() {
    for (const auto& group : g_duplicateFiles) {
        g_checkedGroups.insert(group.first);
    }

    // 更新ListView中的勾选状态
    int itemCount = ListView_GetItemCount(g_hListView);
    for (int i = 0; i < itemCount; i++) {
        ListView_SetCheckState(g_hListView, i, TRUE);
    }
}

// 取消全选所有组
void DeselectAllGroups() {
    g_checkedGroups.clear();

    // 更新ListView中的勾选状态
    int itemCount = ListView_GetItemCount(g_hListView);
    for (int i = 0; i < itemCount; i++) {
        ListView_SetCheckState(g_hListView, i, FALSE);
    }
}

// 更新组勾选状态
void UpdateGroupCheckState(int index, BOOL checked) {
    // 检查是否是组标题项
    auto it = g_groupHeaderToMD5.find(index);
    if (it != g_groupHeaderToMD5.end()) {
        // 这是组标题，更新整个组的勾选状态
        std::wstring md5 = it->second;

        if (checked) {
            g_checkedGroups.insert(md5);
        }
        else {
            g_checkedGroups.erase(md5);
        }

        // 更新组内所有文件的勾选状态
        int itemCount = ListView_GetItemCount(g_hListView);
        for (int i = index + 1; i < itemCount; i++) {
            // 检查是否到达下一个组标题
            if (g_groupHeaderToMD5.find(i) != g_groupHeaderToMD5.end()) {
                break;
            }

            ListView_SetCheckState(g_hListView, i, checked);
        }
    }
}

// 过滤相关函数实现
void ShowFilterBySizeDialog() {
    FilterCriteria criteria;
    criteria.filterBySize = true;
    criteria.filterByType = g_filterByTypeEnabled;
    criteria.filterByDate = g_filterByDateEnabled;
    criteria.filterByPath = g_filterByPathEnabled;
    criteria.minFileSize = g_minFileSize;
    criteria.maxFileSize = g_maxFileSize;
    criteria.fileTypeFilter = g_fileTypeFilter;
    criteria.dateFilterFrom = g_dateFilterFrom;
    criteria.dateFilterTo = g_dateFilterTo;
    criteria.pathFilter = g_pathFilter;

    ShowFilterDialog(g_hwnd, criteria);

    // 更新全局变量
    g_filterBySizeEnabled = criteria.filterBySize;
    g_filterByTypeEnabled = criteria.filterByType;
    g_filterByDateEnabled = criteria.filterByDate;
    g_filterByPathEnabled = criteria.filterByPath;
    g_minFileSize = criteria.minFileSize;
    g_maxFileSize = criteria.maxFileSize;
    g_fileTypeFilter = criteria.fileTypeFilter;
    g_dateFilterFrom = criteria.dateFilterFrom;
    g_dateFilterTo = criteria.dateFilterTo;
    g_pathFilter = criteria.pathFilter;

    // 应用过滤器
    ApplyFilters();
}

void ShowFilterByTypeDialog() {
    FilterCriteria criteria;
    criteria.filterBySize = g_filterBySizeEnabled;
    criteria.filterByType = true;
    criteria.filterByDate = g_filterByDateEnabled;
    criteria.filterByPath = g_filterByPathEnabled;
    criteria.minFileSize = g_minFileSize;
    criteria.maxFileSize = g_maxFileSize;
    criteria.fileTypeFilter = g_fileTypeFilter;
    criteria.dateFilterFrom = g_dateFilterFrom;
    criteria.dateFilterTo = g_dateFilterTo;
    criteria.pathFilter = g_pathFilter;

    ShowFilterDialog(g_hwnd, criteria);

    // 更新全局变量
    g_filterBySizeEnabled = criteria.filterBySize;
    g_filterByTypeEnabled = criteria.filterByType;
    g_filterByDateEnabled = criteria.filterByDate;
    g_filterByPathEnabled = criteria.filterByPath;
    g_minFileSize = criteria.minFileSize;
    g_maxFileSize = criteria.maxFileSize;
    g_fileTypeFilter = criteria.fileTypeFilter;
    g_dateFilterFrom = criteria.dateFilterFrom;
    g_dateFilterTo = criteria.dateFilterTo;
    g_pathFilter = criteria.pathFilter;

    // 应用过滤器
    ApplyFilters();
}

void ShowFilterByDateDialog() {
    FilterCriteria criteria;
    criteria.filterBySize = g_filterBySizeEnabled;
    criteria.filterByType = g_filterByTypeEnabled;
    criteria.filterByDate = true;
    criteria.filterByPath = g_filterByPathEnabled;
    criteria.minFileSize = g_minFileSize;
    criteria.maxFileSize = g_maxFileSize;
    criteria.fileTypeFilter = g_fileTypeFilter;
    criteria.dateFilterFrom = g_dateFilterFrom;
    criteria.dateFilterTo = g_dateFilterTo;
    criteria.pathFilter = g_pathFilter;

    ShowFilterDialog(g_hwnd, criteria);

    // 更新全局变量
    g_filterBySizeEnabled = criteria.filterBySize;
    g_filterByTypeEnabled = criteria.filterByType;
    g_filterByDateEnabled = criteria.filterByDate;
    g_filterByPathEnabled = criteria.filterByPath;
    g_minFileSize = criteria.minFileSize;
    g_maxFileSize = criteria.maxFileSize;
    g_fileTypeFilter = criteria.fileTypeFilter;
    g_dateFilterFrom = criteria.dateFilterFrom;
    g_dateFilterTo = criteria.dateFilterTo;
    g_pathFilter = criteria.pathFilter;

    // 应用过滤器
    ApplyFilters();
}

void ShowFilterByPathDialog() {
    FilterCriteria criteria;
    criteria.filterBySize = g_filterBySizeEnabled;
    criteria.filterByType = g_filterByTypeEnabled;
    criteria.filterByDate = g_filterByDateEnabled;
    criteria.filterByPath = true;
    criteria.minFileSize = g_minFileSize;
    criteria.maxFileSize = g_maxFileSize;
    criteria.fileTypeFilter = g_fileTypeFilter;
    criteria.dateFilterFrom = g_dateFilterFrom;
    criteria.dateFilterTo = g_dateFilterTo;
    criteria.pathFilter = g_pathFilter;

    ShowFilterDialog(g_hwnd, criteria);

    // 更新全局变量
    g_filterBySizeEnabled = criteria.filterBySize;
    g_filterByTypeEnabled = criteria.filterByType;
    g_filterByDateEnabled = criteria.filterByDate;
    g_filterByPathEnabled = criteria.filterByPath;
    g_minFileSize = criteria.minFileSize;
    g_maxFileSize = criteria.maxFileSize;
    g_fileTypeFilter = criteria.fileTypeFilter;
    g_dateFilterFrom = criteria.dateFilterFrom;
    g_dateFilterTo = criteria.dateFilterTo;
    g_pathFilter = criteria.pathFilter;

    // 应用过滤器
    ApplyFilters();
}

void ResetFilters() {
    g_filterBySizeEnabled = false;
    g_filterByTypeEnabled = false;
    g_filterByDateEnabled = false;
    g_filterByPathEnabled = false;
    g_minFileSize = 0;
    g_maxFileSize = LLONG_MAX;
    g_fileTypeFilter.clear();
    g_dateFilterFrom.clear();
    g_dateFilterTo.clear();
    g_pathFilter.clear();

    // 保存重置后的过滤器设置到INI文件
    FilterCriteria criteria;
    criteria.filterBySize = g_filterBySizeEnabled;
    criteria.filterByType = g_filterByTypeEnabled;
    criteria.filterByDate = g_filterByDateEnabled;
    criteria.filterByPath = g_filterByPathEnabled;
    criteria.minFileSize = g_minFileSize;
    criteria.maxFileSize = g_maxFileSize;
    criteria.fileTypeFilter = g_fileTypeFilter;
    criteria.dateFilterFrom = g_dateFilterFrom;
    criteria.dateFilterTo = g_dateFilterTo;
    criteria.pathFilter = g_pathFilter;

    
    MessageBox(g_hwnd, L"已重置所有过滤器", L"提示", MB_OK | MB_ICONINFORMATION);

    // 重新显示所有重复文件
    if (!g_duplicateFiles.empty()) {
        // 重新填充ListView
        ListView_DeleteAllItems(g_hListView);
        g_listItemToFilePath.clear();
        g_groupHeaderToMD5.clear();

        int index = 0;
        for (const auto& group : g_duplicateFiles) {
            // 添加组标题
            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT | LVIF_GROUPID;
            lvi.iItem = index;
            lvi.iSubItem = 0;

            std::wstring header = L"重复文件组 (" + std::to_wstring(group.second.size()) + L" 个文件): " + group.first;
            lvi.pszText = (LPWSTR)header.c_str();

            ListView_InsertItem(g_hListView, &lvi);
            g_groupHeaderToMD5[index] = group.first;
            index++;

            // 添加组内文件
            for (const auto& file : group.second) {
                lvi.iItem = index;
                lvi.mask = LVIF_TEXT | LVIF_INDENT;
                lvi.iIndent = 1;

                std::wstring fileName = PathFindFileName(file.c_str());
                lvi.pszText = (LPWSTR)fileName.c_str();

                ListView_InsertItem(g_hListView, &lvi);
                g_listItemToFilePath[index] = file;

                // 设置路径列
                ListView_SetItemText(g_hListView, index, 1, (LPWSTR)file.c_str());

                index++;
            }
        }

        UpdateStatus(L"已显示所有重复文件");
    }
}

// 显示过滤设置对话框
void ShowFilterSettingsDialog() {
    FilterCriteria criteria;
    criteria.filterBySize = g_filterBySizeEnabled;
    criteria.filterByType = g_filterByTypeEnabled;
        criteria.filterByDate = g_filterByDateEnabled;
        criteria.filterByPath = g_filterByPathEnabled;
        criteria.minFileSize = g_minFileSize;
        criteria.maxFileSize = g_maxFileSize;
        criteria.fileTypeFilter = g_fileTypeFilter;
        criteria.dateFilterFrom = g_dateFilterFrom;
        criteria.dateFilterTo = g_dateFilterTo;
        criteria.pathFilter = g_pathFilter;
    

    ShowFilterDialog(g_hwnd, criteria);

    // 更新全局变量
    g_filterBySizeEnabled = criteria.filterBySize;
    g_filterByTypeEnabled = criteria.filterByType;
    g_filterByDateEnabled = criteria.filterByDate;
    g_filterByPathEnabled = criteria.filterByPath;
    g_minFileSize = criteria.minFileSize;
    g_maxFileSize = criteria.maxFileSize;
    g_fileTypeFilter = criteria.fileTypeFilter;
    g_dateFilterFrom = criteria.dateFilterFrom;
    g_dateFilterTo = criteria.dateFilterTo;
    g_pathFilter = criteria.pathFilter;

    // 应用过滤器
    ApplyFilters();
}

void ApplyFilters() {
    if (g_duplicateFiles.empty()) {
        MessageBox(g_hwnd, L"没有可过滤的文件，请先扫描文件夹", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 创建过滤条件
    FilterCriteria criteria;
    criteria.filterBySize = g_filterBySizeEnabled;
    criteria.filterByType = g_filterByTypeEnabled;
    criteria.filterByDate = g_filterByDateEnabled;
    criteria.filterByPath = g_filterByPathEnabled;
    criteria.minFileSize = g_minFileSize;
    criteria.maxFileSize = g_maxFileSize;
    criteria.fileTypeFilter = g_fileTypeFilter;
    criteria.dateFilterFrom = g_dateFilterFrom;
    criteria.dateFilterTo = g_dateFilterTo;
    criteria.pathFilter = g_pathFilter;

    // 应用过滤器到重复文件
    ApplyFiltersToDuplicates(criteria);

    // 重新填充ListView
    ListView_DeleteAllItems(g_hListView);
    g_listItemToFilePath.clear();
    g_groupHeaderToMD5.clear();

    int index = 0;
    int totalFiles = 0;

    for (const auto& group : g_duplicateFiles) {
        std::vector<std::wstring> filteredFiles;

        // 过滤组内文件
        for (const auto& file : group.second) {
            if (IsFileMatchFilter(file, criteria)) {
                filteredFiles.push_back(file);
            }
        }

        // 如果过滤后仍有文件，则添加到ListView
        if (!filteredFiles.empty()) {
            // 添加组标题
            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT | LVIF_GROUPID;
            lvi.iItem = index;
            lvi.iSubItem = 0;

            std::wstring header = L"重复文件组 (" + std::to_wstring(filteredFiles.size()) + L" 个文件): " + group.first;
            lvi.pszText = (LPWSTR)header.c_str();

            ListView_InsertItem(g_hListView, &lvi);
            g_groupHeaderToMD5[index] = group.first;
            index++;

            // 添加组内文件
            for (const auto& file : filteredFiles) {
                lvi.iItem = index;
                lvi.mask = LVIF_TEXT | LVIF_INDENT;
                lvi.iIndent = 1;

                std::wstring fileName = PathFindFileName(file.c_str());
                lvi.pszText = (LPWSTR)fileName.c_str();

                ListView_InsertItem(g_hListView, &lvi);
                g_listItemToFilePath[index] = file;

                // 设置路径列
                ListView_SetItemText(g_hListView, index, 1, (LPWSTR)file.c_str());

                index++;
                totalFiles++;
            }
        }
    }

    std::wstring status = L"过滤完成，共显示 " + std::to_wstring(totalFiles) + L" 个文件";
    UpdateStatus(status);
}

// 计算文件的MD5哈希值
std::wstring CalculateFileMD5(const std::wstring& filepath) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::wstring md5;

    // 打开文件
    HANDLE hFile = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return L"";
    }

    // 获取加密服务提供程序
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CloseHandle(hFile);
        return L"";
    }

    // 创建哈希对象
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return L"";
    }

    // 读取文件并更新哈希
    const DWORD BUFFER_SIZE = 4096;
    BYTE buffer[BUFFER_SIZE];
    DWORD bytesRead = 0;

    while (ReadFile(hFile, buffer, BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0) {
        if (!CryptHashData(hHash, buffer, bytesRead, 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            CloseHandle(hFile);
            return L"";
        }
    }

    // 获取哈希值
    DWORD hashSize = 0;
    DWORD hashSizeSize = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashSize, &hashSizeSize, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return L"";
    }

    std::vector<BYTE> hashBytes(hashSize);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hashBytes.data(), &hashSize, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return L"";
    }

    // 转换为十六进制字符串
    std::wstringstream ss;
    for (DWORD i = 0; i < hashSize; i++) {
        ss << std::hex << std::setw(2) << std::setfill(L'0') << static_cast<int>(hashBytes[i]);
    }
    md5 = ss.str();

    // 清理
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return md5;
}

// 移动到回收站
bool MoveToRecycleBin(const std::wstring& filepath) {
    SHFILEOPSTRUCTW fileOp = { 0 };

    // 准备文件路径（需要双null结尾）
    std::wstring fromPath = filepath;
    if (fromPath.back() != L'\\') {
        fromPath += L'\\';
    }
    fromPath += L'\0';

    fileOp.hwnd = g_hwnd;
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = fromPath.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&fileOp);
    return (result == 0);
}

// 递归扫描文件夹中的文件
void FindAllFiles(const std::wstring& folder, std::vector<std::wstring>& files) {
    std::wstring searchPath = folder + L"\\*.*";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = folder + L"\\" + findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 递归扫描子文件夹
            FindAllFiles(fullPath, files);
        }
        else {
            // 添加到文件列表
            files.push_back(fullPath);
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

// 扫描重复文件
void ScanDuplicates() {
    if (g_selectedFolder.empty()) {
        MessageBox(g_hwnd, LoadStringResource(IDS_SELECT_FOLDER_FIRST).c_str(), L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 清空之前的记录
    g_duplicateFiles.clear();
    g_listItemToFilePath.clear();
    g_groupHeaderToMD5.clear();
    g_checkedGroups.clear();
    ListView_DeleteAllItems(g_hListView);

    UpdateStatus(LoadStringResource(IDS_SCANNING_FILES));

    // 获取所有文件
    std::vector<std::wstring> files;
    FindAllFiles(g_selectedFolder, files);

    if (files.empty()) {
        UpdateStatus(LoadStringResource(IDS_NO_FILES_FOUND));
        return;
    }

    UpdateStatus(LoadStringResource(IDS_CALCULATING_HASH));
    SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, files.size()));
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);

    // 使用MD5哈希值分组文件
    std::map<std::wstring, std::vector<std::wstring>> fileGroups;
    int processed = 0;

    for (const auto& file : files) {
        std::wstring md5 = CalculateFileMD5(file);
        if (!md5.empty()) {
            fileGroups[md5].push_back(file);
        }

        processed++;
        SendMessage(g_hProgress, PBM_SETPOS, processed, 0);

        // 更新状态
        if (processed % 10 == 0) {
            std::wstring status = LoadStringResource(IDS_PROCESSING_FILE) + L" " + std::to_wstring(processed) +
                L" / " + std::to_wstring(files.size());
            UpdateStatus(status);
        }
    }

    // 找出重复的文件组
    for (const auto& group : fileGroups) {
        if (group.second.size() > 1) {
            g_duplicateFiles[group.first] = group.second;

            // 默认勾选所有组
            g_checkedGroups.insert(group.first);

            // 添加组标题到ListView
            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = ListView_GetItemCount(g_hListView);

            std::wstring groupText = L"重复组 (" + std::to_wstring(group.second.size()) + L" 个文件)";
            lvi.pszText = (LPWSTR)groupText.c_str();
            int groupIndex = ListView_InsertItem(g_hListView, &lvi);

            // 设置勾选状态（默认勾选）
            ListView_SetCheckState(g_hListView, groupIndex, TRUE);

            // 记录组标题到MD5的映射
            g_groupHeaderToMD5[groupIndex] = group.first;

            // 添加组内文件
            for (size_t i = 0; i < group.second.size(); i++) {
                LVITEMW fileItem = { 0 };
                fileItem.mask = LVIF_TEXT | LVIF_INDENT;
                fileItem.iItem = ListView_GetItemCount(g_hListView);
                fileItem.iIndent = 1;  // 设置缩进级别，使子文件向右缩进

                // 获取文件名
                std::wstring fileName = group.second[i];
                size_t lastSlash = fileName.find_last_of(L"\\");
                if (lastSlash != std::wstring::npos) {
                    fileName = fileName.substr(lastSlash + 1);
                }

                // 设置第一列：文件名
                std::wstring displayText = std::to_wstring(i + 1) + L". " + fileName;
                fileItem.pszText = (LPWSTR)displayText.c_str();
                int fileIndex = ListView_InsertItem(g_hListView, &fileItem);

                // 设置第二列：路径
                std::wstring filePath = group.second[i];
                ListView_SetItemText(g_hListView, fileIndex, 1, (LPWSTR)filePath.c_str());

                // 设置勾选状态（默认勾选）
                ListView_SetCheckState(g_hListView, fileIndex, TRUE);

                // 建立映射关系
                g_listItemToFilePath[fileIndex] = group.second[i];
            }
        }
    }

    if (g_duplicateFiles.empty()) {
        UpdateStatus(LoadStringResource(IDS_NO_DUPLICATES_FOUND));

        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.pszText = (LPWSTR)LoadStringResource(IDS_NO_DUPLICATES_FOUND).c_str();
        ListView_InsertItem(g_hListView, &lvi);
    }
    else {
        std::wstring status = LoadStringResource(IDS_FOUND_DUPLICATES) + L" " + std::to_wstring(g_duplicateFiles.size()) +
            L" 组重复文件";
        UpdateStatus(status);

        EnableWindow(g_hBtnDelete, TRUE);
        EnableWindow(g_hBtnSelectAll, TRUE);
        EnableWindow(g_hBtnDeselectAll, TRUE);
    }
}

// 删除重复文件
void DeleteDuplicates() {
    if (g_duplicateFiles.empty()) {
        MessageBox(g_hwnd, LoadStringResource(IDS_NO_DUPLICATES_TO_DELETE).c_str(), L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 检查是否有勾选的组
    if (g_checkedGroups.empty()) {
        MessageBox(g_hwnd, L"没有勾选任何重复组", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int confirm = MessageBox(g_hwnd,
        LoadStringResource(IDS_CONFIRM_DELETE).c_str(),
        L"确认删除",
        MB_YESNO | MB_ICONQUESTION);

    if (confirm != IDYES) {
        return;
    }

    int totalDeleted = 0;
    int totalFailed = 0;
    int groupsToProcess = 0;

    // 计算需要处理的组数
    for (const auto& group : g_duplicateFiles) {
        if (g_checkedGroups.find(group.first) != g_checkedGroups.end()) {
            groupsToProcess++;
        }
    }

    UpdateStatus(LoadStringResource(IDS_DELETING_FILES));
    SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, groupsToProcess));
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);

    int processed = 0;

    for (const auto& group : g_duplicateFiles) {
        // 只处理勾选的组
        if (g_checkedGroups.find(group.first) == g_checkedGroups.end()) {
            continue;
        }

        // 保留第一个文件，删除其他文件
        for (size_t i = 1; i < group.second.size(); i++) {
            if (MoveToRecycleBin(group.second[i])) {
                totalDeleted++;
            }
            else {
                totalFailed++;
            }
        }

        processed++;
        SendMessage(g_hProgress, PBM_SETPOS, processed, 0);
    }

    std::wstring status = LoadStringResource(IDS_DELETE_COMPLETE) + L" " + std::to_wstring(totalDeleted) +
        LoadStringResource(IDS_DELETED) + L" " + std::to_wstring(totalFailed) + LoadStringResource(IDS_DELETE_FAILED);
    UpdateStatus(status);

    // 重新扫描以更新列表
    ScanDuplicates();
}

// 浏览文件夹
void BrowseFolder() {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = g_hwnd;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);

    if (pidl != NULL) {
        wchar_t folderPath[MAX_PATH];
        SHGetPathFromIDListW(pidl, folderPath);

        g_selectedFolder = folderPath;
        SetWindowTextW(g_hEditFolder, folderPath);

        // 重置状态
        g_duplicateFiles.clear();
        g_listItemToFilePath.clear();
        g_groupHeaderToMD5.clear();
        g_checkedGroups.clear();
        ListView_DeleteAllItems(g_hListView);
        UpdateStatus(LoadStringResource(IDS_SELECT_FOLDER));
        EnableWindow(g_hBtnDelete, FALSE);
        EnableWindow(g_hBtnSelectAll, FALSE);
        EnableWindow(g_hBtnDeselectAll, FALSE);

        CoTaskMemFree(pidl);
    }
}

// 更新状态文本
void UpdateStatus(const std::wstring& text) {
    SetWindowTextW(g_hStatus, text.c_str());
}

// 添加文本到列表视图（保留函数名以兼容现有代码）
void AddToListBox(const std::wstring& text) {
    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (LPWSTR)text.c_str();
    ListView_InsertItem(g_hListView, &lvi);
}

// 打开文件
void OpenFile(const std::wstring& filepath) {
    SHELLEXECUTEINFOW sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.hwnd = g_hwnd;
    sei.lpVerb = L"open";
    sei.lpFile = filepath.c_str();
    sei.nShow = SW_SHOW;

    if (!ShellExecuteExW(&sei)) {
        MessageBox(g_hwnd, L"无法打开文件", L"错误", MB_OK | MB_ICONERROR);
    }
}

// 创建菜单
void CreateMenu(HWND hwnd) {
    HMENU hMenu = CreateMenu();
    HMENU hLanguageMenu = CreatePopupMenu();
    HMENU hFilterMenu = CreatePopupMenu();

    // 添加联合国六种语言选项
    AppendMenuW(hLanguageMenu, MF_STRING, ID_MENU_LANG_CHINESE, LoadStringResource(IDS_LANG_CHINESE).c_str());
    AppendMenuW(hLanguageMenu, MF_STRING, ID_MENU_LANG_ENGLISH, LoadStringResource(IDS_LANG_ENGLISH).c_str());
    AppendMenuW(hLanguageMenu, MF_STRING, ID_MENU_LANG_FRENCH, LoadStringResource(IDS_LANG_FRENCH).c_str());
    AppendMenuW(hLanguageMenu, MF_STRING, ID_MENU_LANG_RUSSIAN, LoadStringResource(IDS_LANG_RUSSIAN).c_str());
    AppendMenuW(hLanguageMenu, MF_STRING, ID_MENU_LANG_ARABIC, LoadStringResource(IDS_LANG_ARABIC).c_str());
    AppendMenuW(hLanguageMenu, MF_STRING, ID_MENU_LANG_SPANISH, LoadStringResource(IDS_LANG_SPANISH).c_str());

    // 添加过滤设置选项
    AppendMenuW(hFilterMenu, MF_STRING, ID_MENU_FILTER_SETTINGS, L"过滤设置");
    AppendMenuW(hFilterMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFilterMenu, MF_STRING, ID_MENU_FILTER_RESET, L"重置过滤器");

    // 添加"语言"菜单到主菜单
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hLanguageMenu, LoadStringResource(IDS_LANGUAGE).c_str());

    // 添加"过滤"菜单到主菜单
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFilterMenu, L"过滤");

    // 添加"帮助"菜单到主菜单
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_ABOUT, L"关于");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"帮助");

    // 设置窗口菜单
    SetMenu(hwnd, hMenu);
}

// 创建控件
void CreateControls(HWND hwnd) {
    // 获取客户区大小
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // 文件夹路径标签
    CreateWindowW(L"STATIC", LoadStringResource(IDS_FOLDER_PATH).c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CONTROL_MARGIN, CONTROL_MARGIN, 80, 20,
        hwnd, NULL, NULL, NULL);

    // 文件夹路径编辑框
    g_hEditFolder = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        CONTROL_MARGIN + 85, CONTROL_MARGIN,
        clientWidth - (BUTTON_WIDTH + CONTROL_MARGIN * 2 + 85), 20,
        hwnd, NULL, NULL, NULL);

    // 浏览按钮
    g_hBtnBrowse = CreateWindowW(L"BUTTON", LoadStringResource(IDS_BROWSE).c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        clientWidth - BUTTON_WIDTH - CONTROL_MARGIN, CONTROL_MARGIN,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)1, NULL, NULL);

    // 扫描按钮
    g_hBtnScan = CreateWindowW(L"BUTTON", LoadStringResource(IDS_SCAN_DUPLICATES).c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CONTROL_MARGIN, CONTROL_MARGIN + 35,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)2, NULL, NULL);

    // 删除按钮（初始禁用）
    g_hBtnDelete = CreateWindowW(L"BUTTON", LoadStringResource(IDS_DELETE_DUPLICATES).c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        CONTROL_MARGIN + BUTTON_WIDTH + 10, CONTROL_MARGIN + 35,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)3, NULL, NULL);

    // 全选按钮（初始禁用）
    g_hBtnSelectAll = CreateWindowW(L"BUTTON", L"全选",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        CONTROL_MARGIN + BUTTON_WIDTH * 2 + 20, CONTROL_MARGIN + 35,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)4, NULL, NULL);

    // 取消全选按钮（初始禁用）
    g_hBtnDeselectAll = CreateWindowW(L"BUTTON", L"取消全选",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        CONTROL_MARGIN + BUTTON_WIDTH * 3 + 30, CONTROL_MARGIN + 35,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)5, NULL, NULL);

    // 列表视图（显示重复文件，支持勾选框）
    g_hListView = CreateWindowW(WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
        CONTROL_MARGIN, CONTROL_MARGIN + 70,
        clientWidth - 2 * CONTROL_MARGIN,
        clientHeight - 150,
        hwnd, NULL, NULL, NULL);

    // 初始化ListView
    InitListView();

    // 进度条
    g_hProgress = CreateWindowW(PROGRESS_CLASSW, L"",
        WS_CHILD | WS_VISIBLE,
        CONTROL_MARGIN, clientHeight - 70,
        clientWidth - 2 * CONTROL_MARGIN, 20,
        hwnd, NULL, NULL, NULL);

    // 状态文本
    g_hStatus = CreateWindowW(L"STATIC", LoadStringResource(IDS_READY).c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CONTROL_MARGIN, clientHeight - 40,
        clientWidth - 2 * CONTROL_MARGIN, 20,
        hwnd, NULL, NULL, NULL);
}

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        CreateMenu(hwnd);
        CreateControls(hwnd);
        break;

    case WM_SIZE:
    {
        // 重新定位和调整控件大小
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        // 调整编辑框和浏览按钮
        SetWindowPos(g_hEditFolder, NULL, CONTROL_MARGIN + 85, CONTROL_MARGIN,
            clientWidth - (BUTTON_WIDTH + CONTROL_MARGIN * 2 + 85), 20, SWP_NOZORDER);
        SetWindowPos(g_hBtnBrowse, NULL, clientWidth - BUTTON_WIDTH - CONTROL_MARGIN, CONTROL_MARGIN,
            BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

        // 调整列表视图
        SetWindowPos(g_hListView, NULL, CONTROL_MARGIN, CONTROL_MARGIN + 70,
            clientWidth - 2 * CONTROL_MARGIN, clientHeight - 150, SWP_NOZORDER);

        // 调整进度条
        SetWindowPos(g_hProgress, NULL, CONTROL_MARGIN, clientHeight - 70,
            clientWidth - 2 * CONTROL_MARGIN, 20, SWP_NOZORDER);

        // 调整状态文本
        SetWindowPos(g_hStatus, NULL, CONTROL_MARGIN, clientHeight - 40,
            clientWidth - 2 * CONTROL_MARGIN, 20, SWP_NOZORDER);
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: // 浏览按钮
            BrowseFolder();
            break;

        case 2: // 扫描按钮
            ScanDuplicates();
            break;

        case 3: // 删除按钮
            DeleteDuplicates();
            break;

        case 4: // 全选按钮
            SelectAllGroups();
            break;

        case 5: // 取消全选按钮
            DeselectAllGroups();
            break;

        case ID_MENU_LANG_CHINESE: // 中文
        case ID_MENU_LANG_ENGLISH: // 英语
        case ID_MENU_LANG_FRENCH:  // 法语
        case ID_MENU_LANG_RUSSIAN: // 俄语
        case ID_MENU_LANG_ARABIC:  // 阿拉伯语
        case ID_MENU_LANG_SPANISH: // 西班牙语
            // 预留功能，暂时不实现
            MessageBox(hwnd, LoadStringResource(IDS_LANGUAGE_NOT_IMPLEMENTED).c_str(), L"提示", MB_OK | MB_ICONINFORMATION);
            break;

            // 过滤菜单处理
        case ID_MENU_FILTER_SETTINGS:
            ShowFilterSettingsDialog();
            break;

        case ID_MENU_FILTER_RESET:
            ResetFilters();
            break;
        }
        break;

    case WM_NOTIFY:
    {
        NMHDR* pnmhdr = (NMHDR*)lParam;
        if (pnmhdr->hwndFrom == g_hListView) {
            switch (pnmhdr->code) {
            case NM_DBLCLK: // 双击事件
            {
                int selectedIndex = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                if (selectedIndex != -1) {
                    // 检查映射关系中是否存在对应的文件路径
                    auto it = g_listItemToFilePath.find(selectedIndex);
                    if (it != g_listItemToFilePath.end() && !it->second.empty()) {
                        OpenFile(it->second);
                    }
                }
                break;
            }
            case LVN_ITEMCHANGED: // 项目状态改变事件（包括勾选状态）
            {
                NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_STATEIMAGEMASK)) {
                    // 勾选状态改变
                    BOOL checked = (ListView_GetCheckState(g_hListView, pnmv->iItem) != 0);
                    UpdateGroupCheckState(pnmv->iItem, checked);
                }
                break;
            }
            }
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // 注册窗口类
    const wchar_t CLASS_NAME[] = L"DuplicateFileRemoverClass";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // 创建窗口
    g_hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"RemoveDuplicate",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwnd == NULL) {
        return 0;
    }

    // 加载过滤设置
    FilterCriteria criteria;
        g_filterBySizeEnabled = criteria.filterBySize;
        g_filterByTypeEnabled = criteria.filterByType;
        g_filterByDateEnabled = criteria.filterByDate;
        g_filterByPathEnabled = criteria.filterByPath;
        g_minFileSize = criteria.minFileSize;
        g_maxFileSize = criteria.maxFileSize;
        g_fileTypeFilter = criteria.fileTypeFilter;
        g_dateFilterFrom = criteria.dateFilterFrom;
        g_dateFilterTo = criteria.dateFilterTo;
        g_pathFilter = criteria.pathFilter;
    

    
    // 如果有上次选择的文件夹，更新编辑框
    if (!g_selectedFolder.empty()) {
        SetWindowText(g_hEditFolder, g_selectedFolder.c_str());
    }

    // 显示窗口
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // 消息循环
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}