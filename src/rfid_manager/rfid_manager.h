#include <Windows.h>
#include <TlHelp32.h>
#include <Commctrl.h>
#include <tchar.h>

#include <algorithm>
#include <bitset>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>

#include "IniFile.h"
#include "str_util.h"
#include "resource.h"


#pragma comment(lib, "comctl32")



using namespace std;

static const string ini_file = "cards.ini";
