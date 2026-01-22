#include "filter.h"
#include "Resource.h"
#include <shlwapi.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")

// 判断文件是否匹配过滤条件
bool IsFileMatchFilter(const std::wstring& filePath, const FilterCriteria& criteria) {
    // 检查文件大小过滤
    if (criteria.filterBySize) {
        long long fileSize = GetFileSize(filePath);
        if (!IsFileSizeInRange(fileSize, criteria.minFileSize, criteria.maxFileSize)) {
            return false;
        }
    }
    
    // 检查文件类型过滤
    if (criteria.filterByType && !criteria.fileTypeFilter.empty()) {
        if (!IsFileTypeMatch(filePath, criteria.fileTypeFilter)) {
            return false;
        }
    }
    
    // 检查日期过滤
    if (criteria.filterByDate && (!criteria.dateFilterFrom.empty() || !criteria.dateFilterTo.empty())) {
        WIN32_FILE_ATTRIBUTE_DATA fileData;
        if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileData)) {
            if (!IsDateInRange(fileData.ftLastWriteTime, criteria.dateFilterFrom, criteria.dateFilterTo)) {
                return false;
            }
        }
    }
    
    // 检查路径过滤
    if (criteria.filterByPath && !criteria.pathFilter.empty()) {
        if (!IsPathMatch(filePath, criteria.pathFilter)) {
            return false;
        }
    }
    
    return true;
}

// 判断文件大小是否在指定范围内
bool IsFileSizeInRange(long long fileSize, long long minSize, long long maxSize) {
    return fileSize >= minSize && fileSize <= maxSize;
}

// 判断文件类型是否匹配
bool IsFileTypeMatch(const std::wstring& filePath, const std::wstring& fileTypeFilter) {
    // 获取文件扩展名
    std::wstring extension = PathFindExtension(filePath.c_str());
    if (extension.empty()) {
        return false;
    }
    
    // 转换为小写进行比较
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    // 检查是否匹配过滤条件
    // 支持多种格式，如 ".txt"、"txt"、"*.txt"、"*.txt;*.doc"等
    std::wstring filter = fileTypeFilter;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::towlower);
    
    // 分割多个文件类型
    std::vector<std::wstring> types;
    std::wstringstream ss(filter);
    std::wstring item;
    
    while (std::getline(ss, item, L';')) {
        // 去除空格
        item.erase(std::remove_if(item.begin(), item.end(), ::iswspace), item.end());
        
        // 处理通配符
        if (item.length() >= 2 && item.substr(0, 2) == L"*.") {
            item = item.substr(1); // 去除*.
        } else if (item.length() > 0 && item[0] != L'.') {
            item = L"." + item; // 添加点号
        }
        
        if (!item.empty()) {
            types.push_back(item);
        }
    }
    
    // 检查是否匹配任一类型
    for (const auto& type : types) {
        if (extension == type) {
            return true;
        }
    }
    
    return false;
}

// 判断文件日期是否在指定范围内
bool IsDateInRange(const FILETIME& fileTime, const std::wstring& dateFrom, const std::wstring& dateTo) {
    // 转换FILETIME为系统时间
    SYSTEMTIME stFile;
    if (!FileTimeToSystemTime(&fileTime, &stFile)) {
        return true; // 如果转换失败，默认通过
    }
    
    // 检查起始日期
    if (!dateFrom.empty()) {
        FILETIME ftFrom = StringToFileTime(dateFrom);
        if (CompareFileTime(&fileTime, &ftFrom) < 0) {
            return false;
        }
    }
    
    // 检查结束日期
    if (!dateTo.empty()) {
        FILETIME ftTo = StringToFileTime(dateTo);
        // 设置为当天的结束时间（23:59:59）
        SYSTEMTIME stTo;
        FileTimeToSystemTime(&ftTo, &stTo);
        stTo.wHour = 23;
        stTo.wMinute = 59;
        stTo.wSecond = 59;
        SystemTimeToFileTime(&stTo, &ftTo);
        
        if (CompareFileTime(&fileTime, &ftTo) > 0) {
            return false;
        }
    }
    
    return true;
}

// 判断文件路径是否匹配过滤条件
bool IsPathMatch(const std::wstring& filePath, const std::wstring& pathFilter) {
    // 转换为小写进行比较
    std::wstring lowerPath = filePath;
    std::wstring lowerFilter = pathFilter;
    
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::towlower);
    
    // 检查是否包含过滤字符串
    return lowerPath.find(lowerFilter) != std::wstring::npos;
}

// 将日期字符串转换为FILETIME
FILETIME StringToFileTime(const std::wstring& dateString) {
    SYSTEMTIME st = { 0 };
    
    // 尝试解析日期字符串，支持格式：YYYY-MM-DD、YYYY/MM/DD等
    int year = 0, month = 0, day = 0;
    
    if (swscanf_s(dateString.c_str(), L"%d-%d-%d", &year, &month, &day) == 3 ||
        swscanf_s(dateString.c_str(), L"%d/%d/%d", &year, &month, &day) == 3) {
        st.wYear = (WORD)year;
        st.wMonth = (WORD)month;
        st.wDay = (WORD)day;
    } else {
        // 如果解析失败，返回当前时间
        GetSystemTime(&st);
    }
    
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    return ft;
}

// 将FILETIME转换为日期字符串
std::wstring FileTimeToString(const FILETIME& fileTime) {
    SYSTEMTIME st;
    if (!FileTimeToSystemTime(&fileTime, &st)) {
        return L"";
    }
    
    wchar_t buffer[32];
    swprintf_s(buffer, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return std::wstring(buffer);
}

// 获取文件大小
long long GetFileSize(const std::wstring& filePath) {
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileData)) {
        return ((long long)fileData.nFileSizeHigh << 32) | fileData.nFileSizeLow;
    }
    return -1;
}

// 解析大小字符串（如 "10MB"、"5KB"等）为字节数
bool ParseSizeString(const std::wstring& sizeStr, long long& sizeInBytes) {
    if (sizeStr.empty()) {
        return false;
    }
    
    std::wstring numStr;
    std::wstring unitStr;
    
    // 分离数字和单位
    for (wchar_t ch : sizeStr) {
        if (iswdigit(ch) || ch == L'.') {
            numStr += ch;
        } else if (!iswspace(ch)) {
            unitStr += ch;
        }
    }
    
    if (numStr.empty()) {
        return false;
    }
    
    // 转换数字部分
    double size = _wtof(numStr.c_str());
    
    // 处理单位
    std::transform(unitStr.begin(), unitStr.end(), unitStr.begin(), ::towlower);
    
    if (unitStr == L"b" || unitStr == L"byte" || unitStr == L"bytes") {
        sizeInBytes = (long long)size;
    } else if (unitStr == L"kb" || unitStr == L"k" || unitStr == L"kilobyte" || unitStr == L"kilobytes") {
        sizeInBytes = (long long)(size * 1024);
    } else if (unitStr == L"mb" || unitStr == L"m" || unitStr == L"megabyte" || unitStr == L"megabytes") {
        sizeInBytes = (long long)(size * 1024 * 1024);
    } else if (unitStr == L"gb" || unitStr == L"g" || unitStr == L"gigabyte" || unitStr == L"gigabytes") {
        sizeInBytes = (long long)(size * 1024 * 1024 * 1024);
    } else if (unitStr == L"tb" || unitStr == L"t" || unitStr == L"terabyte" || unitStr == L"terabytes") {
        sizeInBytes = (long long)(size * 1024LL * 1024LL * 1024LL * 1024LL);
    } else {
        // 默认为字节
        sizeInBytes = (long long)size;
    }
    
    return true;
}

// 格式化文件大小为可读字符串
std::wstring FormatFileSize(long long sizeInBytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unitIndex = 0;
    double size = (double)sizeInBytes;
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    wchar_t buffer[64];
    if (unitIndex == 0) {
        swprintf_s(buffer, L"%lld %s", sizeInBytes, units[unitIndex]);
    } else {
        swprintf_s(buffer, L"%.2f %s", size, units[unitIndex]);
    }
    
    return std::wstring(buffer);
}

// 显示过滤对话框
void ShowFilterDialog(HWND hwnd, FilterCriteria& criteria) {
    // 创建过滤对话框
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FILTER_DIALOG), hwnd, 
        (DLGPROC)FilterDialogProc, (LPARAM)&criteria);
}

// 过滤对话框过程函数
INT_PTR CALLBACK FilterDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static FilterCriteria* pCriteria = nullptr;

    switch (message) {
    case WM_INITDIALOG:
        pCriteria = (FilterCriteria*)lParam;

        // 设置对话框标题
        SetWindowTextW(hDlg, LoadStringResource(IDS_FILTER_DIALOG_TITLE).c_str());

        // 初始化对话框控件
        SendDlgItemMessage(hDlg, IDC_CHECK_SIZE, BM_SETCHECK, pCriteria->filterBySize ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hDlg, IDC_CHECK_TYPE, BM_SETCHECK, pCriteria->filterByType ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hDlg, IDC_CHECK_DATE, BM_SETCHECK, pCriteria->filterByDate ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hDlg, IDC_CHECK_PATH, BM_SETCHECK, pCriteria->filterByPath ? BST_CHECKED : BST_UNCHECKED, 0);

        // 设置控件文本
        SetDlgItemTextW(hDlg, IDC_CHECK_SIZE, LoadStringResource(IDS_CHECK_SIZE_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CHECK_TYPE, LoadStringResource(IDS_CHECK_TYPE_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CHECK_DATE, LoadStringResource(IDS_CHECK_DATE_TEXT).c_str());
        SetDlgItemTextW(hDlg, IDC_CHECK_PATH, LoadStringResource(IDS_CHECK_PATH_TEXT).c_str());

        // 设置文件大小范围（默认为空）
        SetDlgItemTextW(hDlg, IDC_EDIT_MIN_SIZE, L"");
        SetDlgItemTextW(hDlg, IDC_EDIT_MAX_SIZE, L"");

        // 设置文件类型
        SetDlgItemTextW(hDlg, IDC_EDIT_TYPE, pCriteria->fileTypeFilter.c_str());

        // 设置日期范围
        SetDlgItemTextW(hDlg, IDC_EDIT_DATE_FROM, pCriteria->dateFilterFrom.c_str());
        SetDlgItemTextW(hDlg, IDC_EDIT_DATE_TO, pCriteria->dateFilterTo.c_str());

        // 设置路径过滤
        SetDlgItemTextW(hDlg, IDC_EDIT_PATH, pCriteria->pathFilter.c_str());

        // 启用/禁用相关控件
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MIN_SIZE), pCriteria->filterBySize);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MAX_SIZE), pCriteria->filterBySize);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TYPE), pCriteria->filterByType);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DATE_FROM), pCriteria->filterByDate);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DATE_TO), pCriteria->filterByDate);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PATH), pCriteria->filterByPath);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_CHECK_SIZE:
            pCriteria->filterBySize = (SendDlgItemMessage(hDlg, IDC_CHECK_SIZE, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MIN_SIZE), pCriteria->filterBySize);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MAX_SIZE), pCriteria->filterBySize);
            break;

        case IDC_CHECK_TYPE:
            pCriteria->filterByType = (SendDlgItemMessage(hDlg, IDC_CHECK_TYPE, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TYPE), pCriteria->filterByType);
            break;

        case IDC_CHECK_DATE:
            pCriteria->filterByDate = (SendDlgItemMessage(hDlg, IDC_CHECK_DATE, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DATE_FROM), pCriteria->filterByDate);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DATE_TO), pCriteria->filterByDate);
            break;

        case IDC_CHECK_PATH:
            pCriteria->filterByPath = (SendDlgItemMessage(hDlg, IDC_CHECK_PATH, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PATH), pCriteria->filterByPath);
            break;

        case IDOK:
        {
            // 获取文件大小范围
            wchar_t minSizeStr[64] = { 0 };
            wchar_t maxSizeStr[64] = { 0 };
            GetDlgItemText(hDlg, IDC_EDIT_MIN_SIZE, minSizeStr, 64);
            GetDlgItemText(hDlg, IDC_EDIT_MAX_SIZE, maxSizeStr, 64);

            if (pCriteria->filterBySize) {
                if (!ParseSizeString(minSizeStr, pCriteria->minFileSize)) {
                    pCriteria->minFileSize = 0;
                }
                if (!ParseSizeString(maxSizeStr, pCriteria->maxFileSize)) {
                    pCriteria->maxFileSize = LLONG_MAX;
                }
            }

            // 获取文件类型
            if (pCriteria->filterByType) {
                wchar_t typeStr[256] = { 0 };
                GetDlgItemText(hDlg, IDC_EDIT_TYPE, typeStr, 256);
                pCriteria->fileTypeFilter = typeStr;
            }

            // 获取日期范围
            if (pCriteria->filterByDate) {
                wchar_t dateFromStr[32] = { 0 };
                wchar_t dateToStr[32] = { 0 };
                GetDlgItemText(hDlg, IDC_EDIT_DATE_FROM, dateFromStr, 32);
                GetDlgItemText(hDlg, IDC_EDIT_DATE_TO, dateToStr, 32);
                pCriteria->dateFilterFrom = dateFromStr;
                pCriteria->dateFilterTo = dateToStr;
            }

            // 获取路径过滤
            if (pCriteria->filterByPath) {
                wchar_t pathStr[512] = { 0 };
                GetDlgItemText(hDlg, IDC_EDIT_PATH, pathStr, 512);
                pCriteria->pathFilter = pathStr;
            }

            EndDialog(hDlg, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}
// 应用过滤器到重复文件
void ApplyFiltersToDuplicates(const FilterCriteria& criteria) {
    // 这个函数在主程序中被调用，用于过滤重复文件
    // 实际的过滤逻辑在主程序的ApplyFilters函数中实现
    // 这里只是一个占位函数，因为过滤逻辑需要访问主程序的全局变量
}

// 重置过滤条件
void ResetFilterCriteria(FilterCriteria& criteria) {
    criteria.filterBySize = false;
    criteria.filterByType = false;
    criteria.filterByDate = false;
    criteria.filterByPath = false;
    criteria.minFileSize = 0;
    criteria.maxFileSize = LLONG_MAX;
    criteria.fileTypeFilter.clear();
    criteria.dateFilterFrom.clear();
    criteria.dateFilterTo.clear();
    criteria.pathFilter.clear();
}