#include "str_util.h"

vector<unsigned char> hex2bin(const string & input)
{
    vector<unsigned char> result;

    if(input.size() % 2)
        return result;

    result.reserve(input.size() / 2);
    
    //rewrite
    unsigned int ch;
    for(size_t i = 0; i < input.size(); i += 2)
    {
        sscanf_s(input.substr(i, 2).c_str(), "%X", &ch);
        result.push_back(ch);
    }

    return result;
}

string bin2hex(const vector<unsigned char> & input)
{
    stringstream ss;

    for(vector<unsigned char>::const_iterator i = input.begin(); i != input.end(); ++i)
        ss<<setw(2)<<setfill('0')<<hex<<static_cast<int>(*i);

    return ss.str();
}

unsigned long long bstr2dec(const string & bstr)
{
    unsigned long long result = 0;

    for(string::const_iterator it = bstr.begin(); it != bstr.end(); ++it)
    {
        result *= 2;
        result += (*it) == '1' ? 1 : 0;
    }

    return result;
}

wstring str2wstr(const char * aIn)
{
    int tLen = lstrlenA(aIn) + 1;

    if(tLen == 1)
        return wstring(); 

    wchar_t * tOutArr = new wchar_t[tLen];

    int tCount = MultiByteToWideChar(CP_ACP, 0, aIn, tLen, tOutArr, tLen);

    if(tCount == 0)
    {
        delete[] tOutArr;
        return wstring(); 
    }
    
    try
    {
        wstring tOutWString(tOutArr);
        delete[] tOutArr;

        return tOutWString;
    }
    catch(const exception&)
    {
        delete[] tOutArr;
        throw;
    }
}

wstring str2wstr(const string & s)
{
    return str2wstr(s.c_str());
}

string wstr2str(const wchar_t * aIn)
{
    int tLen = lstrlenW(aIn) + 1;

    if(tLen == 1)
        return string(); 

    char * tOutArr = new char[tLen];

    BOOL tUnmappable;
    int tCount = WideCharToMultiByte(CP_ACP, 0, aIn, tLen, tOutArr, tLen, NULL, &tUnmappable);

    if(tCount == 0)
    {
        delete[] tOutArr;
        return string();
    }
    
    try
    {
        string tOutString(tOutArr);
        delete[] tOutArr;

        return tOutString;
    }
    catch(const exception&)
    {
        delete[] tOutArr;
        throw;
    }
}

string wstr2str(const wstring & s)
{
    return wstr2str(s.c_str());
}
