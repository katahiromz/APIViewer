#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <map>

struct FUNCTION
{
    std::string name;
    std::string convention;
    std::string ret;
    std::vector<std::string> params;
};

static std::map<std::string, FUNCTION> s_functions;

template <typename t_string_container, 
          typename t_string = typename t_string_container::value_type>
void split(t_string_container& container,
    const typename t_string_container::value_type& str,
    typename t_string::value_type sep)
{
    container.clear();
    std::size_t i = 0, j = str.find(sep);
    while (j != t_string_container::value_type::npos) {
        container.emplace_back(std::move(str.substr(i, j - i)));
        i = j + 1;
        j = str.find(sep, i);
    }
    container.emplace_back(std::move(str.substr(i, -1)));
}

std::string DoDumpFunction(const FUNCTION& fn)
{
    std::vector<std::string> fields;

    split(fields, fn.ret, ':');
    std::string ret = fields[1];

    ret += " ";
    ret += fn.convention;
    ret += " ";
    ret += fn.name;
    ret += "(";
    bool first = true;
    for (auto& param : fn.params)
    {
        if (param == "...")
        {
            ret += "...";
            break;
        }
        split(fields, param, ':');
        if (!first)
            ret += ", ";
        ret += fields[1];
        first = false;
    }
    ret += ");\n\n";

    return ret;
}

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

    FILE *fp = _wfopen(filename.c_str(), L"r");
    if (!fp)
        return FALSE;

    char buf[512];
    fgets(buf, ARRAYSIZE(buf), fp);
    while (fgets(buf, ARRAYSIZE(buf), fp))
    {
        StrTrimA(buf, " \t\r\n");

        FUNCTION fn;
        split(fn.params, buf, '\t');
        if (fn.params.size() < 3)
            continue;

        fn.name = fn.params[0];
        fn.convention = fn.params[1];
        fn.ret = fn.params[2];
        fn.params.erase(fn.params.begin(), fn.params.begin() + 3);
        s_functions.insert(std::make_pair(fn.name, fn));
    }

    fclose(fp);
    return TRUE;
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    WCHAR szPath[MAX_PATH];
    if (!GetWondersDirectory(szPath, ARRAYSIZE(szPath)))
    {
        MessageBoxW(hwnd, L"Wonders not found", NULL, MB_ICONERROR);
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }
    PathAddBackslashW(szPath);

#ifdef _WIN64
    if (!DoLoadFunctions(szPath, L"-cl-64-w.dat"))
#else
    if (!DoLoadFunctions(szPath, L"-cl-32-w.dat"))
#endif
    {
        MessageBoxW(hwnd, L"Cannot load Wonders.", NULL, MB_ICONERROR);
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }

    SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"C/C++");
    SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);

    SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"Function");
    SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);

    for (auto& pair : s_functions)
    {
        SendDlgItemMessageA(hwnd, lst1, LB_ADDSTRING, 0, (LPARAM)pair.first.c_str());
    }

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

        if (SendDlgItemMessageW(hwnd, lst1, LB_GETCOUNT, 0, 0) == 1)
        {
            SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, 0, 0);
        }
        return;
    }

    CHAR szTextA[128];
    WideCharToMultiByte(CP_ACP, 0, szTextW, -1, szTextA, ARRAYSIZE(szTextA), NULL, NULL);

    for (auto& pair : s_functions)
    {
        if (pair.first.find(szTextA) == 0)
        {
            SendDlgItemMessageA(hwnd, lst1, LB_ADDSTRING, 0, (LPARAM)pair.first.c_str());
        }
    }

    if (SendDlgItemMessageW(hwnd, lst1, LB_GETCOUNT, 0, 0) == 1)
    {
        SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, 0, 0);
    }
}

void OnCmb1(HWND hwnd)
{
}

void OnCmb2(HWND hwnd)
{
}

void OnAdd(HWND hwnd)
{
    INT i, nCount = (INT)SendDlgItemMessageW(hwnd, lst1, LB_GETCOUNT, 0, 0);
    if (nCount == 0)
        return;

    if (nCount == 1)
        i = 0;
    else
        i = (INT)SendDlgItemMessageW(hwnd, lst1, LB_GETCURSEL, 0, 0);

    CHAR szItem[128];
    SendDlgItemMessageA(hwnd, lst1, LB_GETTEXT, i, (LPARAM)szItem);

    auto it = s_functions.find(szItem);
    if (it == s_functions.end())
        return;

    std::string str;
    str = DoDumpFunction(it->second);

    INT cchText = GetWindowTextLengthA(GetDlgItem(hwnd, edt2));
    SendDlgItemMessageA(hwnd, edt2, EM_SETSEL, cchText, cchText);
    SendDlgItemMessageA(hwnd, edt2, EM_REPLACESEL, TRUE, (LPARAM)str.c_str());
    cchText = GetWindowTextLengthA(GetDlgItem(hwnd, edt2));
    SendDlgItemMessageA(hwnd, edt2, EM_SETSEL, cchText, cchText);
    SendDlgItemMessageA(hwnd, edt2, EM_SCROLLCARET, 0, 0);
}

void OnCopy(HWND hwnd)
{
    WCHAR szText[512];
    GetDlgItemTextW(hwnd, edt2, szText, ARRAYSIZE(szText));

    if (!OpenClipboard(hwnd))
        return;

    EmptyClipboard();

    DWORD cbData = (lstrlenW(szText) + 1) * sizeof(WCHAR);
    HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cbData);
    if (hGlobal)
    {
        LPWSTR pszData = (LPWSTR)GlobalLock(hGlobal);
        CopyMemory(pszData, szText, cbData);
        GlobalUnlock(hGlobal);

        SetClipboardData(CF_UNICODETEXT, hGlobal);
    }

    CloseClipboard();
}

void OnClear(HWND hwnd)
{
    SetDlgItemTextW(hwnd, edt2, L"");
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
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
    case IDOK:
        OnAdd(hwnd);
        break;
    case psh2:
        OnCopy(hwnd);
        break;
    case psh3:
        OnClear(hwnd);
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
