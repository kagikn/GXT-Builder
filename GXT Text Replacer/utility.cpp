#include "Crc32keygen.h"
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
#include <filesystem>

#include <windows.h>
#include <io.h>
#include <fcntl.h>


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
    int iBufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, (wchar_t*)NULL, 0);

    std::vector<wchar_t> dest(iBufferSize, L'\0');

    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, dest.data(), iBufferSize);

    return std::wstring(dest.data(), dest.data() + iBufferSize - 1);
}

std::unordered_map<std::string, std::string> EntryLoader::LoadEntryTextsInDirectory(const std::wstring& textDirectory, GXTEnum::eTextConvertingMode textConvertingMode, std::ofstream& logFile)
{
    namespace fs = std::experimental::filesystem::v1;
    std::unordered_map<std::string, std::string> entryMap;

    for (auto & p : fs::directory_iterator(textDirectory))
    {
        if (p.path().extension() == ".txt")
        {            
            EntryLoader::LoadFileContent(p.path().c_str(), entryMap, logFile);
        }
    }

    return entryMap;
}

std::unordered_map<std::string, std::string> EntryLoader::LoadHashEntryTextsInDirectory(const std::wstring& textDirectory, GXTEnum::eTextConvertingMode textConvertingMode, std::ofstream& logFile)
{
    namespace fs = std::experimental::filesystem::v1;
    std::unordered_map<std::string, std::string> entryMap;

    for (auto & p : fs::directory_iterator(textDirectory))
    {
        if (p.path().extension() == ".txt")
        {
            EntryLoader::LoadFileContent(p.path().c_str(), entryMap, logFile);
        }
    }

    return entryMap;
}

void EntryLoader::LoadFileContent(const wchar_t* fileName, std::unordered_map<std::string, std::string>& entryMap, std::ofstream& logFile)
{
    std::ifstream		InputFile(fileName, std::ifstream::in);

    if (InputFile.is_open())
    {
        std::wcout << L"Reading entries from " << fileName << L"...\n";

        if (!Utf8Validator::IsValid(InputFile))
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
                        logFile << L"ERROR: the entry name " << EntryName << " at line " << lineCount << " contains non-ASCII characters! " << "Only ASCII characters can be used for entry names.";
                        continue;
                    }
                }
                if (EntryName.length() >= 8)
                {
                    logFile << L"ERROR: the entry name " << EntryName << " at line " << lineCount << " is too long! " << "Entry names must be less than 8 characters.";
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

void EntryLoader::LoadFileContentForHashEntry(const wchar_t* fileName, std::unordered_map<uint32_t, std::string>& entryMap, std::ofstream& logFile)
{
    std::ifstream		InputFile(fileName, std::ifstream::in);

    if (InputFile.is_open())
    {
        std::wcout << L"Reading entries from " << fileName << L"...\n";

        if (!Utf8Validator::IsValid(InputFile))
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

                if (EntryName.size() >= 3)
                {
                    std::string twoStr = EntryName.substr(0, 2);

                    if (twoStr == "0x" || twoStr == "0X")
                    {
                        std::string hexStr = EntryName.substr(2);

                        auto hexValue = HexStringToUInt32(hexStr);
                        if (hexValue != std::nullopt)
                        {
                            if (!entryMap.emplace(hexValue.value(), EntryContent).second)
                            {
                                if (logFile.is_open())
                                {
                                    std::wstring wideFileName(fileName);
                                    logFile << "Entry " << EntryName << " duplicated in " << std::string(wideFileName.begin(), wideFileName.end()) << " file!\n";
                                }
                            }

                            continue;
                        }
                        else
                        {
                            logFile << L"ERROR: the entry name " << EntryName << " has invalid hex value!\n";
                            continue;
                        }
                    }
                }

                for (char& c : EntryName)
                {
                    if (c > 0x7e)
                    {
                        logFile << L"ERROR: the entry name " << EntryName << " at line " << lineCount << " contains non-ASCII characters! " << "Only ASCII characters can be used for entry names.\n";
                        continue;
                    }
                }
                if (EntryName.length() >= 8)
                {
                    logFile << L"ERROR: the entry name " << EntryName << " at line " << lineCount << " is too long! " << "Entry names must be less than 8 characters.\n";
                    continue;
                }

                uint32_t entryHash = Crc32KeyGen::GetUppercaseKey(EntryName.c_str());

                // Push entry into table map
                if (!entryMap.emplace(entryHash, EntryContent).second)
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

std::optional<uint32_t> EntryLoader::HexStringToUInt32(const std::string& hexString)
{
    if (hexString.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos)
    {
        uint32_t hex;
        std::stringstream ss;
        ss << std::hex << hexString;
        ss >> hex;

        return hex;
    }
    else
    {
        return std::nullopt;
    }
}

bool Utf8Validator::IsValid(std::ifstream& file)
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

CharMapArray CharMap::ParseCharacterMap(const std::wstring& szFileName)
{
    std::ifstream		CharMapFile(szFileName, std::ifstream::in);
    CharMapArray		characterMap;

    if (CharMapFile.is_open() && Utf8Validator::IsValid(CharMapFile))
    {
        CharMapArray::iterator charMapIterator = characterMap.begin();
        for (size_t i = 0; i < CHARACTER_MAP_HEIGHT; ++i)
        {
            std::string FileLine;
            std::getline(CharMapFile, FileLine);

            utf8::iterator<std::string::iterator> utf8It(FileLine.begin(), FileLine.begin(), FileLine.end());
            for (size_t j = 0; j < CHARACTER_MAP_WIDTH; ++j)
            {
                while (utf8It.base() != FileLine.end() && *utf8It == '\t')
                {
                    ++utf8It;
                }

                if (utf8It.base() == FileLine.end())
                    throw std::runtime_error("Cannot parse character map file " + std::string(szFileName.begin(), szFileName.end()) + "!");

                *charMapIterator++ = *utf8It++;
            }
        }
    }
    else
        throw std::runtime_error("Cannot parse character map file " + std::string(szFileName.begin(), szFileName.end()) + "!");

    return characterMap;
}

void CharMap::ApplyCharacterMap(tableMap_t& TablesMap, const CharMapArray& characterMap)
{
    for (auto& it : TablesMap)
    {
        for (utf8::iterator<std::string::iterator> strIt(it.second->Content.begin(), it.second->Content.begin(), it.second->Content.end());
            strIt.base() != it.second->Content.end(); ++strIt)
        {
            bool	bFound = false;
            if (*strIt == '\0')
            {
                it.second->PushFormattedChar('\0');
                continue;
            }
            for (size_t i = 0; i < CHARACTER_MAP_SIZE; ++i)
            {
                if (*strIt == characterMap[i])
                {
                    it.second->PushFormattedChar(static_cast<int>(i) + 32);
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                std::ostringstream tmpstream;
                tmpstream << "Can't locate character \"" << static_cast<wchar_t>(*strIt) << "\" (" << *strIt << ") in a character map!";
                throw std::runtime_error(tmpstream.str());
            }
        }

    }
}

