#include "utility.h"
#include "utf8.h"

#include <fstream>
#include <iostream>
#include <forward_list>
#include <Shlwapi.h>
#include <ctime>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>

#include <windows.h>

bool Directory::Exists(const std::wstring& dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


std::wstring Encoding::AnsiStringToWString(std::string const& src)
{
    int iBufferSize = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, (wchar_t*)NULL, 0);

    std::vector<wchar_t> dest(iBufferSize, L'\0');

    MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, dest.data(), iBufferSize);

    return std::wstring(dest.data(), dest.data() + iBufferSize - 1);
}

std::wstring Encoding::Utf8ToUtf16(const std::string& utf8)
{
    std::vector<unsigned long> unicode;
    size_t i = 0;
    while (i < utf8.size())
    {
        unsigned long uni;
        size_t todo;
        bool error = false;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F)
        {
            uni = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            throw std::logic_error("not a UTF-8 string");
        }
        else if (ch <= 0xDF)
        {
            uni = ch & 0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            uni = ch & 0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            uni = ch & 0x07;
            todo = 3;
        }
        else
        {
            throw std::logic_error("not a UTF-8 string");
        }
        for (size_t j = 0; j < todo; ++j)
        {
            if (i == utf8.size())
                throw std::logic_error("not a UTF-8 string");
            unsigned char ch = utf8[i++];
            if (ch < 0x80 || ch > 0xBF)
                throw std::logic_error("not a UTF-8 string");
            uni <<= 6;
            uni += ch & 0x3F;
        }
        if (uni >= 0xD800 && uni <= 0xDFFF)
            throw std::logic_error("not a UTF-8 string");
        if (uni > 0x10FFFF)
            throw std::logic_error("not a UTF-8 string");
        unicode.push_back(uni);
    }
    std::wstring utf16;
    for (size_t i = 0; i < unicode.size(); ++i)
    {
        unsigned long uni = unicode[i];
        if (uni <= 0xFFFF)
        {
            utf16 += (wchar_t)uni;
        }
        else
        {
            uni -= 0x10000;
            utf16 += (wchar_t)((uni >> 10) + 0xD800);
            utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
        }
    }
    return utf16;
}

void EntryLoader::LoadFileContent(const wchar_t* fileName, std::unordered_map<std::string, std::string>& entryMap, std::ofstream& logFile)
{
    std::ifstream		InputFile(fileName, std::ifstream::in);

    if (InputFile.is_open())
    {
        std::wcout << L"Reading entries from " << fileName << L"...\n";

        if (!Utf8Validator::MakeSureFileIsValid(InputFile))
        {
            std::wcerr << L"ERROR: File " << fileName << " contains invalid UTF-8 characters!\n";
            return;
        }

        uint64_t lineCount = 0;
        std::string	fileLine;
        while (std::getline(InputFile, fileLine))
        {
            lineCount++;

            if (!fileLine.empty() && fileLine[0] != '#')
            {
                // Extract entry name
                std::string::size_type tabPos = fileLine.find_first_of('\t');
                if (tabPos == std::string::npos) continue;

                std::string		EntryName(fileLine.begin(), fileLine.begin() + tabPos);
                std::string		EntryContent(fileLine.begin() + fileLine.find_first_not_of('\t', tabPos), fileLine.end());

                for (char& c : EntryName)
                {
                    if (c > 0x7e)
                    {
                        std::wcerr << L"ERROR: the entry name " << Encoding::Utf8ToUtf16(EntryName) << "at line" << lineCount << "contains non-ASCII characters!" << "Only ASCII characters can be used for entry names.";
                        continue;
                    }
                }
                if (EntryName.length() >= 8)
                {
                    std::wcerr << L"ERROR: the entry name " << Encoding::Utf8ToUtf16(EntryName) << "at line" << lineCount << "is too long!" << "Entry names must be less than 8 characters.";
                    continue;
                }
                // Push entry into table map
                if (!entryMap.emplace(EntryName, EntryContent).second)
                {
                    if (logFile.is_open())
                    {
                        std::wstring wideFileName(fileName);
                        logFile << "Entry " << EntryName << " duplicated in " << std::string(wideFileName.begin(), wideFileName.end()) << " file!\n";
                    }
                }
            }
        }
    }
    else
    {
        std::wstring tmp(fileName);
        throw std::runtime_error(std::string(tmp.begin(), tmp.end()) + " not found!");
    }
}

bool Utf8Validator::MakeSureFileIsValid(std::ifstream& file)
{
    std::istreambuf_iterator<char> it(file.rdbuf());
    std::istreambuf_iterator<char> eos;
    if (!utf8::is_valid(it, eos))
        return false;

    file.seekg(0, std::ios_base::beg);

    // Skip BOM (if exists)
    // starts_with_bom advances the pointer all the time, so we have to call seekg anyway
    file.seekg(utf8::starts_with_bom(it, eos) ? 3 : 0, std::ios_base::beg);
    return true;
}

