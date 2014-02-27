#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>


#include <Windows.h>

using namespace std;

wstring str2wstr(const char * aIn);
wstring str2wstr(const string & s);
string wstr2str(const wchar_t * aIn);
string wstr2str(const wstring & s);

vector<unsigned char> hex2bin(const string & input);
string bin2hex(const vector<unsigned char> & input);
unsigned long long bstr2dec(const string & bstr);
