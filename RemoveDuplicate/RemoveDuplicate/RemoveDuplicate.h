#pragma once

#include "resource.h"
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <commctrl.h>

// 常量定义
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int CONTROL_MARGIN = 10;
const int BUTTON_WIDTH = 120;
const int BUTTON_HEIGHT = 25;

// 菜单ID定义在Resource.h中定义

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateMenu(HWND hwnd);
void CreateControls(HWND hwnd);
void BrowseFolder();
void ScanDuplicates();
std::wstring CalculateFileMD5(const std::wstring& filepath);
bool MoveToRecycleBin(const std::wstring& filepath);
void FindAllFiles(const std::wstring& folder, std::vector<std::wstring>& files);
void DeleteDuplicates();
void UpdateStatus(const std::wstring& text);
void AddToListBox(const std::wstring& text);
void OpenFile(const std::wstring& filepath);
std::wstring LoadStringResource(UINT id);

// 过滤相关函数
void CreateFilterMenu();
void ShowFilterBySizeDialog();
void ShowFilterByTypeDialog();
void ShowFilterByDateDialog();
void ShowFilterByPathDialog();
void ShowFilterSettingsDialog();
void ResetFilters();
void ApplyFilters();

// 全局变量声明（使用extern）
extern HWND g_hwnd;
extern HWND g_hEditFolder;
extern HWND g_hBtnBrowse;
extern HWND g_hBtnScan;
extern HWND g_hBtnDelete;
extern HWND g_hListView;  // 改为ListView控件以支持勾选框
extern HWND g_hProgress;
extern HWND g_hStatus;
extern HWND g_hBtnSelectAll;  // 全选按钮
extern HWND g_hBtnDeselectAll;  // 取消全选按钮
extern std::wstring g_selectedFolder;
extern std::map<std::wstring, std::vector<std::wstring>> g_duplicateFiles;
extern std::map<int, std::wstring> g_listItemToFilePath;  // 修改为ListView相关
extern std::map<int, std::wstring> g_groupHeaderToMD5;  // 组标题到MD5的映射
extern std::set<std::wstring> g_checkedGroups;  // 已勾选的组

// 过滤相关全局变量
extern bool g_filterBySizeEnabled;
extern bool g_filterByTypeEnabled;
extern bool g_filterByDateEnabled;
extern bool g_filterByPathEnabled;
extern long long g_minFileSize;
extern long long g_maxFileSize;
extern std::wstring g_fileTypeFilter;
extern std::wstring g_dateFilterFrom;
extern std::wstring g_dateFilterTo;
extern std::wstring g_pathFilter;
