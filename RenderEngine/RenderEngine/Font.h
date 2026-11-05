#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include "External Lib/MathLibrary/MathLib.h"

struct GlyphPoint
{
	Maths::Vec2S pointCord;
    bool onCurve;
};

struct GlyphOutline
{
    std::vector<GlyphPoint> points;
    std::vector<uint16_t>   endPts;   // dernier point de chaque contour
    int xMin = 0, yMin = 0, xMax = 0, yMax = 0;
};
struct Bitmap
{
    int width, height;
    std::vector<uint8_t> pixels;
    Bitmap(int w, int h) : width(w), height(h), pixels(w* h, 0) {}
};

class Font
{

private:

    size_t m_segCount = 0;
    size_t m_endCodeAddr = 0;
    size_t m_startCodeAddr = 0;
    size_t m_idDeltaAddr = 0;
    size_t m_idRangeOffsetAddr = 0;

    static void fill(const std::vector<std::vector<Maths::Vec2S>>& contours, Bitmap& bmp);
protected:
    bool parseCmap();
    size_t m_cmapSubtable = 0;     // adresse absolue (dans m_data) de la sous-table format 4
    

    struct TableInfo
    {
        uint32_t offset;
        uint32_t length;
    };

    std::vector<uint8_t> m_data;
    std::map<std::string, TableInfo> m_tables;

public:

    
    int16_t  m_ascender = 0;
    int16_t  m_descender = 0;
    int16_t  m_lineGap = 0;
    uint16_t m_numHMetrics = 0;
    uint16_t m_unitsPerEm = 0; //échelle de la police de caractère
    int16_t  m_indexToLocFormat = 0; // 0 = format court donc on le met en readU16 * 2 sinon si c'est 1 c'est le format long et on le laisse en readU32 
    uint16_t m_numGlyphs = 0;
    static std::vector<std::vector<Maths::Vec2S>> flattenOutline(const GlyphOutline& g, int steps = 8);


    
    bool loadFont(const std::string& path);
    size_t Getm_dataSize()const { return m_data.size(); }
    uint16_t readU16(size_t offset) const;
    int16_t  readI16(size_t offset) const;
    uint32_t readU32(size_t offset) const;
    bool parseTableDirectory();
    bool parseHeadAndMaxp();
    void parseCmapSegments();
    bool parseHhea();
    int16_t  getLeftSideBearing(uint16_t glyphIndex) const;
    bool loadGlyph(uint16_t glyphIndex, GlyphOutline& out) const;
    bool getGlyphLocation(uint16_t glyphIndex, size_t& offset, size_t& length) const;
    uint16_t getGlyphIndex(uint32_t codepoint) const;
    uint16_t getAdvanceWidth(uint16_t glyphIndex) const;
    int16_t getAscender() const { return readI16(m_tables.at("hhea").offset + 4); }
    int16_t getDescender() const { return readI16(m_tables.at("hhea").offset + 6); }
    Bitmap   renderText(const std::string& text, float pixelSize) const;



};