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
        throw std::runtime_error("Table head ou maxp manquante\n");
        return false;
    }

    size_t headBase = headIt->second.offset;
    size_t maxpBase = maxpIt->second.offset;

    // 2. Magic number : contrôle de cohérence
    if (readU32(headBase + 12) != 0x5F0F3CF5)
    {
        throw std::runtime_error("Magic number de head invalide\n");
        return false;
    }

    // 3. Valeurs de head
    m_unitsPerEm = readU16(headBase + 18);
    m_indexToLocFormat = readI16(headBase + 50);

    if (m_indexToLocFormat != 0 && m_indexToLocFormat != 1)
    {
        throw std::runtime_error("indexToLocFormat invalide\n");
        return false;
    }

    // 4. Valeur de maxp
    m_numGlyphs = readU16(maxpBase + 4);


    return parseCmap();
}



bool Font::parseCmap()
{
   
    auto cmapId = m_tables.find("cmap");
    if (cmapId == m_tables.end()) {
        throw std::runtime_error("cmap not found");
    }
    size_t base = cmapId->second.offset;

    size_t nm_cmapSubtable = readU16(base + 2);

    for (size_t i = 0; i < nm_cmapSubtable; i++) {

        size_t location = base + 4 + i*8;

        size_t platformID = readU16(location);

        size_t encodingId = readU16(location + 2);



        bool accepted = (platformID == 3 && encodingId == 1) || platformID == 0;
        if (!accepted)
            continue;


        size_t offset = readU32(location+4);



        size_t cmapAbsoluteloc = base + offset;
        size_t U16format = readU16(cmapAbsoluteloc);
        if (U16format != 4)
            std::cerr << "Wrong address\n";

        m_cmapSubtable = cmapAbsoluteloc;
        parseCmapSegments();
        debugPrintSegments();


        return true;
    }


    throw std::runtime_error("Aucune sous-table cmap format 4 Unicode trouvee");



}

void Font::debugPrintSegments() const
{
    for (size_t i = 0; i < m_segCount; ++i)
        std::cout << "seg " << i
        << " : [" << readU16(m_startCodeAddr + 2 * i)
        << " .. " << readU16(m_endCodeAddr + 2 * i) << "]"
        << " delta=" << readI16(m_idDeltaAddr + 2 * i)
        << " rangeOffset=" << readU16(m_idRangeOffsetAddr + 2 * i)
        << "\n";
}

void Font::parseCmapSegments()
{
    size_t segCountX2 = readU16(m_cmapSubtable + 6);
    m_segCount = segCountX2 / 2;


    m_endCodeAddr = m_cmapSubtable + 14 ; 
    m_startCodeAddr = (m_endCodeAddr + segCountX2)+2;    // taille d'un tableau + padding
    m_idDeltaAddr = m_startCodeAddr + segCountX2;  // taille d'un tableau
    m_idRangeOffsetAddr = m_idDeltaAddr + segCountX2;    // taille d'un tableau


}


uint16_t Font::getGlyphIndex(uint32_t codepoint) const
{
    // 0. hors BMP, ou 0xFFFF (segment terminal) : pas de glyphe
    if (codepoint >= 0xFFFF)
        return 0;

    for (size_t i = 0; i < m_segCount; ++i)
    {
        // 1. segment trop bas : on passe au suivant
        if (codepoint > readU16(m_endCodeAddr + 2 * i))
            continue;

        // 2. on est dans le premier segment dont end >= codepoint
        uint16_t start = readU16(m_startCodeAddr + 2 * i);
        if (codepoint < start)
            return 0;                       // trou entre deux segments

        // 3. les deux autres valeurs du segment
        uint16_t delta = readU16(m_idDeltaAddr + 2 * i);
        size_t   rangeAddr = m_idRangeOffsetAddr + 2 * i;
        uint16_t range = readU16(rangeAddr);

        // 4. raccourci : pas de glyphIdArray
        if (range == 0)
            return static_cast<uint16_t>((codepoint + delta) & 0xFFFF);

        // 5. lecture dans glyphIdArray, adresse relative à la case idRangeOffset[i]
        size_t addr = rangeAddr + range + 2 * (codepoint - start);
        if (addr + 1 >= m_data.size())
            return 0;

        uint16_t glyph = readU16(addr);
        if (glyph == 0)
            return 0;

        return static_cast<uint16_t>((glyph + delta) & 0xFFFF);
    }
    return 0;
}