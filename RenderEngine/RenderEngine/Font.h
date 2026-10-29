#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
class Font
{
public:

    uint16_t m_unitsPerEm = 0; //échelle de la police de caractère
    int16_t  m_indexToLocFormat = 0; // 0 = format court donc on le met en readU16 * 2 sinon si c'est 1 c'est le format long et on le laisse en readU32 
    uint16_t m_numGlyphs = 0;



    bool loadFont(const std::string& path);
    size_t Getm_dataSize()const { return m_data.size(); }
    uint16_t readU16(size_t offset) const;
    int16_t  readI16(size_t offset) const;
    uint32_t readU32(size_t offset) const;
    bool parseTableDirectory();
    bool parseHeadAndMaxp();




protected:
    bool parseCmap();
    size_t m_cmapSubtable = 0;   // adresse absolue (dans m_data) de la sous-table format 4
    struct TableInfo
    {
        uint32_t offset;
        uint32_t length;
    };

    std::vector<uint8_t> m_data;
    std::map<std::string, TableInfo> m_tables;

};