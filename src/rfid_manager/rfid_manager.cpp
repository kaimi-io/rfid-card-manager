#include "rfid_manager.h"

int (WINAPI * rf_init_com)(int port, int baud_rate);
int (WINAPI * rf_ClosePort)();
int (WINAPI * rf_beep)(unsigned short hz, unsigned char ms);
int (WINAPI * Read_Em4001)(void * dst);
int (WINAPI * Standard_Write)(char a1, char a2, const void * src, char a4);
int (__cdecl * Reset_Command)();

bool compute_parity(unsigned int v)
{
    bool parity = false;

    while (v)
    {
        parity = !parity;
        v = v & (v - 1);
    }

    return parity;
}

inline void append_bits(vector<unsigned char>& vec, unsigned int value, unsigned char& vector_pos, unsigned char& used_bits)
{
    value &= 0x1F;

    vec[vector_pos] |= used_bits <= 3 ? value << (3 - used_bits) : value >> (used_bits - 3);
    used_bits += 5;

    if(used_bits >= 8)
    {
        used_bits -= 8;
        vec[++vector_pos] |= value << (8 - used_bits);
    }
}

vector<unsigned char> id_transform(const string & hex)
{
    vector<unsigned char> ret(8, 0);
    ret[0] = 0xFF;
    ret[1] = 0x80;
	
    vector<unsigned char> bin = hex2bin(hex);
    unsigned int col_parity = 0;
    unsigned int temp;
    unsigned char used_bits = 1;
    unsigned char curr_vector_pos = 1;

	
    for(vector<unsigned char>::const_iterator it = bin.begin(); it != bin.end(); ++it)
    {
        unsigned char c = (*it);
       
        temp = (c >> 4) << 1;
        temp |= (char)compute_parity( c >> 4 );
        
        col_parity ^= temp;
        
        append_bits(ret, temp, curr_vector_pos, used_bits);

        
        temp = (c & 0x0F) << 1;
        temp |= (char)compute_parity( c & 0x0F );
          
        append_bits(ret, temp, curr_vector_pos, used_bits);

        col_parity ^= temp;
    }

    col_parity &= 0x1E;
    ret[curr_vector_pos] |= used_bits <= 3 ? col_parity << (3 - used_bits) : col_parity >> (used_bits - 3);

	
    return ret;
}

HTREEITEM insert_child(HWND tree, const wstring & title, map<HTREEITEM, pair<wstring, wstring>> & cards, HTREEITEM parent)
{
    TVINSERTSTRUCT insert;
    wchar_t temp[256];

    ZeroMemory(&insert, sizeof(TVINSERTSTRUCT));
    wcscpy_s(temp, _countof(temp), title.c_str());

    insert.hParent = parent;
    insert.hInsertAfter = TVI_LAST;
    insert.item.mask = TVIF_TEXT;
    insert.item.pszText = temp;
    insert.item.cchTextMax = _countof(temp);

    return TreeView_InsertItem(tree, &insert);
}

HTREEITEM insert_root(HWND tree, const wstring & title, map<wstring, HTREEITEM> * buildings)
{
    HTREEITEM root;
    TVINSERTSTRUCT insert;
    wchar_t temp[256];

    ZeroMemory(&insert, sizeof(TVINSERTSTRUCT));
    wcscpy_s(temp, _countof(temp), title.c_str());

    insert.hParent = NULL;
    insert.hInsertAfter = TVI_ROOT;
    insert.item.mask = TVIF_TEXT | TVIF_CHILDREN;
    insert.item.cChildren = 1;
    insert.item.pszText = temp;
    insert.item.cchTextMax = _countof(temp);
    
    root = TreeView_InsertItem(tree, &insert);

    if(buildings != 0)
        buildings->insert(make_pair(title, root));

    return root;
}

map<HTREEITEM, pair<wstring, wstring>> fill_tree_view(CIniFile & ini, HWND tree)
{
    HTREEITEM root;
    wstring v, d, n;
    map<wstring, HTREEITEM> buildings;
    map<HTREEITEM, pair<wstring, wstring>> cards;
    map<wstring, HTREEITEM>::const_iterator b_it;
    
    
    vector<CIniFile::Record> r;
    vector<string> s = ini.GetSectionNames(ini_file);


    for(vector<string>::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        v = str2wstr(ini.GetValue("building", (*it), ini_file));
        
        if(v.empty())
            continue;

        if(buildings.find(v) == buildings.end())
        {
            root = insert_root(tree, v, &buildings);
        }
        else
        {
            b_it = buildings.find(v);
            if(b_it != buildings.end())
                root = (*b_it).second;
        }

        
        r = ini.GetSection((*it), ini_file);
		
        root = insert_child(tree, str2wstr(r[0].Section), cards, root);

        d = str2wstr(ini.GetValue("data", (*it), ini_file));
        n = str2wstr(ini.GetValue("note", (*it), ini_file));

        cards.insert(make_pair(root, make_pair(d, n)));
    }

    return cards;
}

int MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TV_ITEM item;
    HTREEITEM parent;
    static bool conn_state = false;
    unsigned int i;
    wchar_t name[256], data[256], note[256], building[256];

    static CIniFile ini;

    static HMODULE dll;
    static HWND tree;
    static map<HTREEITEM, pair<wstring, wstring>> cards;

    map<HTREEITEM, pair<wstring, wstring>>::iterator it;
    vector<unsigned char> exch_data;


    switch(uMsg)
    {
        case WM_INITDIALOG:
            dll = LoadLibrary(L"MasterRD.dll");
            if(dll == NULL)
            {
                MessageBox(hWnd, L"Failed to load MasterRD.dll", L"Error", MB_OK | MB_ICONERROR);
                EndDialog(hWnd, 0);

                break;
            }

            (FARPROC &)rf_init_com =  GetProcAddress(dll, "rf_init_com");
            (FARPROC &)rf_ClosePort =  GetProcAddress(dll, "rf_ClosePort");
            (FARPROC &)rf_beep =  GetProcAddress(dll, "rf_beep");
            (FARPROC &)Read_Em4001 =  GetProcAddress(dll, "Read_Em4001");
            (FARPROC &)Standard_Write =  GetProcAddress(dll, "Standard_Write");
            (FARPROC &)Reset_Command =  GetProcAddress(dll, "Reset_Command");

            if
            (
                rf_init_com == NULL || rf_ClosePort == NULL || rf_beep == NULL
                ||
                Read_Em4001 == NULL || Standard_Write == NULL || Reset_Command == NULL
            )
            {
                MessageBox(hWnd, L"Failed to obtain procedure address from MasterRD.dll", L"Error", MB_OK | MB_ICONERROR);
                EndDialog(hWnd, 0);

                break;
            }

            tree = GetDlgItem(hWnd, IDC_TREE);
            cards = fill_tree_view(ini, tree);

            SendDlgItemMessage(hWnd, IDC_COM, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"COM1"));
            SendDlgItemMessage(hWnd, IDC_COM, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"COM2"));
            SendDlgItemMessage(hWnd, IDC_COM, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"COM3"));
            SendDlgItemMessage(hWnd, IDC_COM, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"COM4"));

            SendDlgItemMessage(hWnd, IDC_DATA, EM_LIMITTEXT, 10, 0);
            SendDlgItemMessage(hWnd, IDC_NAME, EM_LIMITTEXT, 256, 0);
            SendDlgItemMessage(hWnd, IDC_NOTE, EM_LIMITTEXT, 256, 0);
        break;

        case WM_NOTIFY:
            switch((reinterpret_cast<LPNMHDR>(lParam))->code)
            {
                case TVN_SELCHANGED:
                    it = cards.find(reinterpret_cast<LPNMTREEVIEW>(lParam)->itemNew.hItem);
                    if(it != cards.end())
                    {
                        ZeroMemory(&item, sizeof(TV_ITEM));

                        item.mask = TVIF_HANDLE | TVIF_TEXT;
                        item.hItem = (*it).first;
                        item.pszText = data;
                        item.cchTextMax = _countof(data);

                        TreeView_GetItem(tree, &item);

                        SetDlgItemText(hWnd, IDC_NAME, data);
                        SetDlgItemText(hWnd, IDC_DATA, (*it).second.first.c_str());
                        SetDlgItemText(hWnd, IDC_NOTE, (*it).second.second.c_str());


                        parent = TreeView_GetParent(tree, (*it).first);

                        ZeroMemory(&item, sizeof(TV_ITEM));

                        item.mask = TVIF_HANDLE | TVIF_TEXT;
                        item.hItem = parent;
                        item.pszText = data;
                        item.cchTextMax = _countof(data);

                        TreeView_GetItem(tree, &item);


                        SetDlgItemText(hWnd, IDC_BLD, data);
                    }
                break;
            }
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_WRITE:
                    i = GetDlgItemText(hWnd, IDC_DATA, data, sizeof(data));

                    if(i != 10)
                    {
                        SetDlgItemText(hWnd, IDC_STATUS, L"Erroneous ID size");
                        break;
                    }

                    exch_data = id_transform(wstr2str(data));

                    if
                    (
                        Standard_Write(2, 0, &exch_data[0], 1) == 0
                        &&
                        Standard_Write(2, 0, &exch_data[4], 2) == 0
                        &&
                        Standard_Write(2, 0, "\x00\x14\x80\x40", 0) == 0
                    )
                    {
                        SetDlgItemText(hWnd, IDC_STATUS, L"Data was successfully written");
                        rf_beep(0, 6);
                    }
                    else
                    {
                        SetDlgItemText(hWnd, IDC_STATUS, L"Can't write data");
                    }

                    Reset_Command();
                break;

                case IDC_READ:
                    exch_data.resize(5);

                    if(Read_Em4001(&exch_data[0]))
                    {
                        SetDlgItemText(hWnd, IDC_STATUS, L"Can't read card data");
                    }
                    else
                    {
                        SetDlgItemText(hWnd, IDC_DATA, str2wstr(bin2hex(exch_data)).c_str());
                        rf_beep(0, 6);
                    }
                break;

                case IDC_CONNECT:
                    if(conn_state)
                    {
                        conn_state = false;
                        rf_ClosePort();

                        SetDlgItemText(hWnd, IDC_CONNECT, L"Connect");
                        SetDlgItemText(hWnd, IDC_STATUS, L"Disconnected");

                        EnableWindow(GetDlgItem(hWnd, IDC_READ), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_WRITE), FALSE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hWnd, IDC_CONNECT), FALSE);

                        i = SendDlgItemMessage(hWnd, IDC_COM, CB_GETCURSEL, 0, 0);
                    
                        i++;

                        if(rf_init_com(i, 9600))
                        {
                            SetDlgItemText(hWnd, IDC_STATUS, L"Can't connect to device");
                            EnableWindow(GetDlgItem(hWnd, IDC_CONNECT), TRUE);
                            break;
                        }

                        conn_state = true;
                        SetDlgItemText(hWnd, IDC_CONNECT, L"Disconnect");
                        SetDlgItemText(hWnd, IDC_STATUS, L"Connected");

                        EnableWindow(GetDlgItem(hWnd, IDC_CONNECT), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_READ), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_WRITE), TRUE);
                    }
                break;
                case IDC_DELETE:
                    it = cards.find(TreeView_GetSelection(tree));
                    if(it == cards.end())
                        break;
                    else
                        ini.DeleteSection(wstr2str(name), ini_file);
                    
                /* Intentional break omit */
                case IDC_REFRESH:
                    TreeView_DeleteAllItems(tree);
                    cards = fill_tree_view(ini, tree);
                break;

                case IDC_SAVE:
                    GetDlgItemText(hWnd, IDC_NAME, name, _countof(name));
                    GetDlgItemText(hWnd, IDC_DATA, data, _countof(data));
                    GetDlgItemText(hWnd, IDC_NOTE, note, _countof(note));
                    GetDlgItemText(hWnd, IDC_BLD, building, _countof(building));

                    
                    if(ini.GetSection(wstr2str(name), ini_file).empty())
                        ini.AddSection(wstr2str(name), ini_file);

                    ini.SetValue("data", wstr2str(data), wstr2str(name), ini_file);
                    ini.SetValue("note", wstr2str(note), wstr2str(name), ini_file);
                    ini.SetValue("building", wstr2str(building), wstr2str(name), ini_file);

                    TreeView_DeleteAllItems(tree);
                    cards = fill_tree_view(ini, tree); 
                break;
            }
        break;

        case WM_CLOSE:
            rf_ClosePort();
            EndDialog(hWnd, 0);
        break;

        default:
            return 0;
    }

    return 1;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX ccl =
    {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES
    };

    InitCommonControlsEx(&ccl);

    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, (DLGPROC) MainDlgProc, 0);
 
    return 0;
}
