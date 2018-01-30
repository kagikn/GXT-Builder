#pragma once

namespace GXTEnum
{
    enum eGXTVersion
    {
        GXT_VC,
        GXT_SA,
        GXT_SA_MOBILE
    };

    enum eTextConvertingMode
    {
        UseCharacterMap,
        UseUtf8OrUtf16,
        UseAnsi
    };
}