#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <map>
#include "TypeSystem/TypeSystem.h"

struct FUNCTION
{
    std::string name;
    std::string convention;
    std::string ret;
    std::vector<std::string> params;
};

static std::map<std::string, FUNCTION> s_functions;

#ifdef _WIN64
static CR_NameScope s_namescope(std::make_shared<CR_ErrorInfo>(), true);
#else
static CR_NameScope s_namescope(std::make_shared<CR_ErrorInfo>(), false);
#endif

BOOL GetWondersDirectory(LPWSTR pszPath, INT cchPath)
{
    WCHAR szDir[MAX_PATH], szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szDir, ARRAYSIZE(szDir));
    PathRemoveFileSpecW(szDir);

    lstrcpynW(szPath, szDir, ARRAYSIZE(szPath));
    PathAppendW(szPath, L"WondersXP");
    if (!PathIsDirectoryW(szPath))
    {
        lstrcpynW(szPath, szDir, ARRAYSIZE(szPath));
        PathAppendW(szPath, L"..\\WondersXP");
        if (!PathIsDirectoryW(szPath))
        {
            lstrcpynW(szPath, szDir, ARRAYSIZE(szPath));
            PathAppendW(szPath, L"..\\..\\WondersXP");
            if (!PathIsDirectoryW(szPath))
            {
                lstrcpynW(szPath, szDir, ARRAYSIZE(szPath));
                PathAppendW(szPath, L"..\\..\\..\\WondersXP");
                if (!PathIsDirectoryW(szPath))
                    return FALSE;
            }
        }
    }

    lstrcpynW(pszPath, szPath, cchPath);
    return TRUE;
}

BOOL DoLoadFunctions(LPCWSTR prefix, LPCWSTR suffix)
{
    std::wstring filename = prefix;
    filename += L"functions";
    filename += suffix;

    FILE *fp = _wfopen(filename.c_str(), "r");
    if (!fp)
        return FALSE;

    char buf[512];
    while (fgets(buf, ARRAYSIZE(buf), fp))
    {
        StrTrimA(buf, " \t\r\n");

        FUNCTION fn;
        split(fn.params, buf, "\t");
        if (vec.size() < 3)
            continue;

        fn.name = fn.params[0];
        fn.convention = fn.params[1];
        fn.ret = fn.params[2];
        fn.params.erase(fn.params.begin(), fn.params.begin() + 3);
        s_functions.insert(fn.name, fn);
    }

    fclose(fp9;
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    WCHAR szPath[MAX_PATH];
    if (!GetWondersDirectory(szPath, ARRAYSIZE(szPath)))
    {
        MessageBoxW(hwnd, L"Wonders not found", NULL, MB_ICONERROR)
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }
    PathAddBackslashW(szPath);

#ifdef _WIN64
    if (!namescope.LoadFromFiles(szPath, L"-cl-64-w.dat") ||
        !DoLoadFunctions(szPath, L"-cl-64-w.dat"))
#else
    if (!namescope.LoadFromFiles(szPath, L"-cl-32-w.dat") ||
        !DoLoadFunctions(szPath, L"-cl-32-w.dat"))
#endif
    {
        MessageBoxW(hwnd, L"Cannot load Wonders.", NULL, MB_ICONERROR)
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }

    SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"C/C++");
    SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);

    SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"Function");
    SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"Structure");
    SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"Type");
    SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"Macro");
    SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);

    return TRUE;
}

void OnEdt1(HWND hwnd)
{
    WCHAR szTextW[128];
    GetDlgItemTextW(hwnd, edt1, szTextW, ARRAYSIZE(szTextW));
    StrTrimW(szTextW, L" \t\r\n");

    SendDlgItemMessageW(hwnd, lst1, LB_RESETCONTENT, 0, 0);
    if (szTextW[0] == 0)
    {
        for (auto& pair : s_functions)
        {
            SendDlgItemMessageA(hwnd, lst1, LB_ADDSTRING, 0, (LPARAM)pair.first.c_str());
        }
        return;
    }

    CHAR szTextA[128];
    WideCharToMultiByte(CP_ACP, 0, szTextW, -1, szTextA, ARRAYSIZE(szTextA), NULL, NULL);

    for (auto& pair : s_functions)
    {
        if (pair.first.find(szTextA) != std::string::npos)
        {
            SendDlgItemMessageA(hwnd, lst1, LB_ADDSTRING, 0, (LPARAM)pair.first.c_str());
        }
    }
}

void OnCmb1(HWND hwnd)
{
}

void OnCmb2(HWND hwnd)
{
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case cmb1:
        OnCmb1(hwnd);
        break;
    case cmb2:
        OnCmb2(hwnd);
        break;
    case edt1:
        if (codeNotify == EN_CHANGE)
        {
            OnEdt1(hwnd);
        }
        break;
    }
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    DialogBoxW(hInstance, MAKEINTRESOURCEW(1), NULL, DialogProc);
    return 0;
}
