#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <ctime>

// 过滤条件结构体
struct FilterCriteria {
    bool filterBySize;
    bool filterByType;
    bool filterByDate;
    bool filterByPath;
    long long minFileSize;
    long long maxFileSize;
    std::wstring fileTypeFilter;
    std::wstring dateFilterFrom;
    std::wstring dateFilterTo;
    std::wstring pathFilter;
};

// 字符串资源加载函数
std::wstring LoadStringResource(UINT id);

// 过滤相关函数声明
bool IsFileMatchFilter(const std::wstring& filePath, const FilterCriteria& criteria);
bool IsFileSizeInRange(long long fileSize, long long minSize, long long maxSize);
bool IsFileTypeMatch(const std::wstring& filePath, const std::wstring& fileTypeFilter);
bool IsDateInRange(const FILETIME& fileTime, const std::wstring& dateFrom, const std::wstring& dateTo);
bool IsPathMatch(const std::wstring& filePath, const std::wstring& pathFilter);
FILETIME StringToFileTime(const std::wstring& dateString);
std::wstring FileTimeToString(const FILETIME& fileTime);
long long GetFileSize(const std::wstring& filePath);
void ApplyFiltersToDuplicates(const FilterCriteria& criteria);
void ShowFilterDialog(HWND hwnd, FilterCriteria& criteria);
INT_PTR CALLBACK FilterDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ResetFilterCriteria(FilterCriteria& criteria);
bool ParseSizeString(const std::wstring& sizeStr, long long& sizeInBytes);
std::wstring FormatFileSize(long long sizeInBytes);
