#pragma once

#include <array>

class Crc32KeyGen
{
public:
    static uint32_t GetKey(const char *pString, int iSize);
    static uint32_t GetKey(const char *pString);
    static uint32_t GetUppercaseKey(const char *pString);
    static uint32_t AppendStringToKey(unsigned int uiHash, const char *pString);
};