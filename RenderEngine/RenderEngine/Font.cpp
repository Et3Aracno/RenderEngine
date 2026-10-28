#include "Font.h"
#include <fstream>
#include <iostream>
#include <cstring>

bool Font::loadFont(const std::string& path)
{
    m_data.clear();


    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) 
    {
        std::cerr << "Cannot open " << path << "\n"; return false; 
    }
    size_t size = file.tellg();
    
    m_data.resize(size);
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(m_data.data()), size);

    return parseTableDirectory();
}


uint16_t Font::readU16(size_t offset) const
{
    return m_data[offset] << 8 | m_data[offset+1];
}

int16_t  Font::readI16(size_t offset) const
{

    return static_cast<int16_t>(readU16(offset));

}

uint32_t Font::readU32(size_t offset) const 
{

    return static_cast<uint32_t>(readU16(offset))<<16 | static_cast<uint32_t>(readU16(offset+2));

}

bool Font::parseTableDirectory()
{

    m_tables.clear();

    uint32_t version = readU32(0);
    if (version != 0x00010000 && version != 0x74727565) {
        throw std::runtime_error("please load a ttf file");
    }

    uint16_t tables = readU16(4);
    for (size_t  i = 0; i < tables; i++)
    {

        size_t pos = 12 + i * 16;

        std::string tag;
        for (size_t k = 0; k < 4; ++k)
            tag += static_cast<char>(m_data[pos + k]);

        TableInfo info;
        info.offset = readU32(pos + 8);   // +4 est le checksum, ignoré
        info.length = readU32(pos + 12);

        // Chaque table doit tenir dans le fichier
        if (static_cast<size_t>(info.offset) + info.length > m_data.size())
        {
            throw std::runtime_error("Table hors du fichier\n");
        }

        m_tables[tag] = info;



    }
    for (const auto& [tag, info] : m_tables)
        std::cout << tag << " offset=" << info.offset << " length=" << info.length << "\n";


    return parseHeadAndMaxp();
}

bool Font::parseHeadAndMaxp()
{
   
    // 1. Les deux tables doivent exister
    auto headIt = m_tables.find("head");
    auto maxpIt = m_tables.find("maxp");

    if (headIt == m_tables.end() || maxpIt == m_tables.end())
    {
        std::cerr << "Table head ou maxp manquante\n";
        return false;
    }

    size_t headBase = headIt->second.offset;
    size_t maxpBase = maxpIt->second.offset;

    // 2. Magic number : contrôle de cohérence
    if (readU32(headBase + 12) != 0x5F0F3CF5)
    {
        std::cerr << "Magic number de head invalide\n";
        return false;
    }

    // 3. Valeurs de head
    m_unitsPerEm = readU16(headBase + 18);
    m_indexToLocFormat = readI16(headBase + 50);

    if (m_indexToLocFormat != 0 && m_indexToLocFormat != 1)
    {
        std::cerr << "indexToLocFormat invalide\n";
        return false;
    }

    // 4. Valeur de maxp
    m_numGlyphs = readU16(maxpBase + 4);


    return true;
}
