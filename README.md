# GXT Text replacer
GXT Text replacer for GTA San Andreas.

This tool allows you to replace GXT texts with the texts in UTF-8 text files quickly.

**IMPORTANT: This tool is just a software prototype, especially made for Japanese Kanji Mod.**  
**I'll create more a stable version with an GXT text extracter and a more flexible builder than Silent's GXT Builder.**

## Building

You need Visual Studio 2017 to compile.

## Using

    gxt_text_replacer [GXT filename] [Text folder] [-usecharmap] [-ansitext] [-unicodetext] [-ansicodepage (value)]

Text folder must contain sub folders whose name is same as one table name in GXT files and the sub folders must contain txt files. No recursive search.

    Example: [root folder]--[MAIN]--[some txt files]
    
Entry name (or its hash) and text MUST be separated with a single tabulator (\t)!!!  
Entry name hash must be formatted like "0x10000000".

### Text example:  
```0x00000000	NULL text```  
```TEST1	foo bar```

## Help

For additional help, use:

    gxt_text_replacer --help
