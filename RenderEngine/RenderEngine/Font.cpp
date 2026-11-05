#include "Font.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <stdexcept>


bool Font::loadFont(const std::string& path)
{
    data_.clear();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::cerr << "Cannot open " << path << "\n"; return false;
    }
    size_t size = file.tellg();

    data_.resize(size);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(data_.data()), size);

    return parseTableDirectory();
}


uint16_t Font::readU16(size_t offset) const
{
    return static_cast<uint16_t>((data_[offset] << 8) | data_[offset + 1]);
}

int16_t  Font::readI16(size_t offset) const
{
    return static_cast<int16_t>(readU16(offset));
}

uint32_t Font::readU32(size_t offset) const
{
    return static_cast<uint32_t>(readU16(offset)) << 16 | static_cast<uint32_t>(readU16(offset + 2));
}

bool Font::parseTableDirectory()
{
    tables_.clear();

    uint32_t version = readU32(0);
    if (version != 0x00010000 && version != 0x74727565) { // 0x74727565 = 'true' pour les collections de polices en ASCII, et 0x00010000 = version classique du format TTF
        throw std::runtime_error("please load a ttf file");
    }

    uint16_t tableCount = readU16(4);
    for (size_t i = 0; i < tableCount; i++)
    {
        size_t pos = 12 + i * 16;

        std::string tag;
        for (size_t k = 0; k < 4; ++k)
            tag += static_cast<char>(data_[pos + k]);

        TableInfo info;
        info.offset = readU32(pos + 8);   // +4 est le checksum, ignoré
        info.length = readU32(pos + 12);

        // Chaque table doit tenir dans le fichier
        if (static_cast<size_t>(info.offset) + info.length > data_.size())
        {
            throw std::runtime_error("Table hors du fichier\n");
        }

        tables_[tag] = info;
    }
    for (const auto& [tag, info] : tables_)
        std::cout << tag << " offset=" << info.offset << " length=" << info.length << "\n";

    return parseHeadAndMaxp();
}

bool Font::parseHeadAndMaxp()
{
    // 1. Les deux tables doivent exister
    auto headIt = tables_.find("head");
    auto maxpIt = tables_.find("maxp");

    if (headIt == tables_.end() || maxpIt == tables_.end())
    {
        throw std::runtime_error("Table head ou maxp manquante\n");
    }

    size_t headBase = headIt->second.offset;
    size_t maxpBase = maxpIt->second.offset;

    // 2. Magic number : contrôle de cohérence
    if (readU32(headBase + 12) != 0x5F0F3CF5)
    {
        throw std::runtime_error("Magic number de head invalide\n");
    }

    // 3. Valeurs de head
    unitsPerEm_ = readU16(headBase + 18);
    indexToLocFormat_ = readI16(headBase + 50);

    if (indexToLocFormat_ != 0 && indexToLocFormat_ != 1)
    {
        throw std::runtime_error("indexToLocFormat invalide\n");
    }

    // 4. Valeur de maxp
    numGlyphs_ = readU16(maxpBase + 4);

    return parseCmap();
}


bool Font::parseCmap()
{
    auto cmapIt = tables_.find("cmap");
    if (cmapIt == tables_.end()) {
        throw std::runtime_error("cmap not found");
    }
    size_t base = cmapIt->second.offset;

    size_t subtableCount = readU16(base + 2);

    for (size_t i = 0; i < subtableCount; i++) {

        size_t location = base + 4 + i * 8;

        size_t platformId = readU16(location);
        size_t encodingId = readU16(location + 2);

        bool accepted = (platformId == 3 && encodingId == 1) || platformId == 0;
        if (!accepted)
            continue;

        size_t offset = readU32(location + 4);

        size_t cmapAbsoluteLoc = base + offset;
        size_t format = readU16(cmapAbsoluteLoc);
        if (format != 4) {
            std::cerr << "Wrong address\n";
            continue;
        }
        cmapSubtable_ = cmapAbsoluteLoc;

        parseCmapSegments();
        return true;
    }

    throw std::runtime_error("Aucune sous-table cmap format 4 Unicode trouvee");
}


void Font::parseCmapSegments()
{
    size_t segCountX2 = readU16(cmapSubtable_ + 6);
    segCount_ = segCountX2 / 2;

    endCodeAddr_ = cmapSubtable_ + 14;
    startCodeAddr_ = (endCodeAddr_ + segCountX2) + 2;    // taille d'un tableau + padding
    idDeltaAddr_ = startCodeAddr_ + segCountX2;          // taille d'un tableau
    idRangeOffsetAddr_ = idDeltaAddr_ + segCountX2;      // taille d'un tableau
}

uint16_t Font::getGlyphIndex(uint32_t codepoint) const
{
    // 0. hors BMP, ou 0xFFFF (segment terminal) : pas de glyphe
    if (codepoint >= 0xFFFF)
        return 0;

    for (size_t i = 0; i < segCount_; ++i)
    {
        // 1. segment trop bas : on passe au suivant
        if (codepoint > readU16(endCodeAddr_ + 2 * i))
            continue;

        // 2. on est dans le premier segment dont end >= codepoint
        uint16_t start = readU16(startCodeAddr_ + 2 * i);
        if (codepoint < start)
            return 0;                       // trou entre deux segments

        // 3. les deux autres valeurs du segment
        uint16_t delta = readU16(idDeltaAddr_ + 2 * i);
        size_t   rangeAddr = idRangeOffsetAddr_ + 2 * i;
        uint16_t range = readU16(rangeAddr);

        // 4. raccourci : pas de glyphIdArray
        if (range == 0)
            return static_cast<uint16_t>((codepoint + delta) & 0xFFFF);

        // 5. lecture dans glyphIdArray, adresse relative à la case idRangeOffset[i]
        size_t addr = rangeAddr + range + 2 * (codepoint - start);
        if (addr + 1 >= data_.size())
            return 0;

        uint16_t glyph = readU16(addr);
        if (glyph == 0)
            return 0;

        return static_cast<uint16_t>((glyph + delta) & 0xFFFF);
    }
    return 0;
}


bool Font::getGlyphLocation(uint16_t glyphIndex, size_t& offset, size_t& length) const
{
    if (glyphIndex >= numGlyphs_)
        return false;

    auto loca = tables_.find("loca");
    auto glyf = tables_.find("glyf");
    if (loca == tables_.end() || glyf == tables_.end())
        return false;

    size_t locaBase = loca->second.offset;
    size_t glyfBase = glyf->second.offset;
    size_t start = 0;
    size_t end = 0;

    if (indexToLocFormat_ == 0)
    {
        // uint16 * 2
        size_t entrySize = 2;
        if ((glyphIndex + 2) * entrySize > loca->second.length)
            return false;

        start = static_cast<size_t>(readU16(locaBase + 2 * glyphIndex)) * 2;
        end = static_cast<size_t>(readU16(locaBase + 2 * (glyphIndex + 1))) * 2;
    }
    else
    {
        // uint32
        size_t entrySize = 4;
        if ((static_cast<size_t>(glyphIndex) + 2) * entrySize > loca->second.length)
            return false;

        start = readU32(locaBase + 4 * glyphIndex);
        end = readU32(locaBase + 4 * (glyphIndex + 1));
    }

    // longueur
    if (end <= start)
        return false;

    length = end - start;

    // le glyphe doit tenir dans la table glyf
    if (end > glyf->second.length)
        return false;

    offset = glyfBase + start;
    return true;
}


bool Font::loadGlyph(uint16_t glyphIndex, GlyphOutline& out) const
{
    out = GlyphOutline{};

    // glyphe loc
    size_t offset = 0, length = 0;
    if (!getGlyphLocation(glyphIndex, offset, length))
        return false;

    size_t end = offset + length;     // premier octet après le glyphe
    size_t p = offset;                // curseur de lecture

    // En-tête de 10 octets
    if (length < 10)
        return false;

    int16_t contourCount = readI16(p);
    out.xMin = readI16(p + 2);
    out.yMin = readI16(p + 4);
    out.xMax = readI16(p + 6);
    out.yMax = readI16(p + 8);
    p += 10;

    if (contourCount <= 0)            // négatif : composite (non géré)
        return false;

    // 3. Fins de contours
    size_t nc = static_cast<size_t>(contourCount);
    if (p + 2 * nc + 2 > end)         // endPts + instructionLength doivent tenir
        return false;

    out.endPts.resize(nc);
    for (size_t i = 0; i < nc; ++i)
    {
        out.endPts[i] = readU16(p + 2 * i);
        if (i > 0 && out.endPts[i] <= out.endPts[i - 1])
            return false;             // doit être strictement croissant
    }
    p += 2 * nc;

    size_t pointCount = static_cast<size_t>(out.endPts.back()) + 1;

    // 4. Instructions : on saute
    uint16_t instructionLength = readU16(p);
    p += 2 + instructionLength;
    if (p > end)
        return false;

    // 5. Flags (avec répétition)
    std::vector<uint8_t> flags;
    flags.reserve(pointCount);
    while (flags.size() < pointCount)
    {
        if (p >= end) return false;
        uint8_t f = data_[p++];
        flags.push_back(f);

        if (f & 0x08)                 // REPEAT : l'octet suivant = nombre de copies
        {
            if (p >= end) return false;
            uint8_t repeat = data_[p++];
            for (uint8_t r = 0; r < repeat && flags.size() < pointCount; ++r)
                flags.push_back(f);
        }
    }

    // 6. Coordonnées x (toutes les x d'abord)
    out.points.resize(pointCount);
    int x = 0;
    for (size_t i = 0; i < pointCount; ++i)
    {
        uint8_t f = flags[i];
        if (f & 0x02)                 // X_SHORT : 1 octet, signe donné par 0x10
        {
            if (p >= end) return false;
            int dx = data_[p++];
            x += (f & 0x10) ? dx : -dx;
        }
        else if (!(f & 0x10))         // ni short ni "same" : delta sur 2 octets signés
        {
            if (p + 2 > end) return false;
            x += readI16(p);
            p += 2;
        }
        // sinon : x identique au précédent, rien à lire

        out.points[i].pointCord.x = static_cast<float>(x);
        out.points[i].isOnCurve = (f & 0x01) != 0;
    }

    // 7. Coordonnées y (même logique, bits 0x04 et 0x20)
    int y = 0;
    for (size_t i = 0; i < pointCount; ++i)
    {
        uint8_t f = flags[i];
        if (f & 0x04)
        {
            if (p >= end) return false;
            int dy = data_[p++];
            y += (f & 0x20) ? dy : -dy;
        }
        else if (!(f & 0x20))
        {
            if (p + 2 > end) return false;
            y += readI16(p);
            p += 2;
        }
        out.points[i].pointCord.y = static_cast<float>(y);
    }

    return true;
}

static Maths::Vec2S midpoint(Maths::Vec2S a, Maths::Vec2S b)
{
    return { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
}

// Ajoute à 'out' les points de la courbe, SANS le point de départ p0 (déjà dans out)
static void flattenQuad(Maths::Vec2S p0, Maths::Vec2S p1, Maths::Vec2S p2, int steps, std::vector<Maths::Vec2S>& out)
{
    for (int i = 1; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        float u = 1.0f - t;
        out.push_back({
            u * u * p0.x + 2.0f * u * t * p1.x + t * t * p2.x,
            u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y
            });
    }
}

std::vector<std::vector<Maths::Vec2S>> Font::flattenOutline(const GlyphOutline& g, int steps)
{
    struct P { Maths::Vec2S p; bool isOn; };

    std::vector<std::vector<Maths::Vec2S>> contours;
    size_t first = 0;

    for (uint16_t endIdx : g.endPts)
    {
        size_t last = endIdx;
        if (last < first || last >= g.points.size())
            break;                                   // données incohérentes

        size_t n = last - first + 1;
        if (n < 2) { first = last + 1; continue; }   // contour dégénéré

        // 1. Liste cyclique avec points on-curve implicites
        std::vector<P> pts;
        for (size_t i = 0; i < n; ++i)
        {
            const GlyphPoint& cur = g.points[first + i];
            const GlyphPoint& nxt = g.points[first + (i + 1) % n];

            Maths::Vec2S c = { static_cast<float>(cur.pointCord.x), static_cast<float>(cur.pointCord.y) };
            Maths::Vec2S d = { static_cast<float>(nxt.pointCord.x), static_cast<float>(nxt.pointCord.y) };

            pts.push_back({ c, cur.isOnCurve });
            if (!cur.isOnCurve && !nxt.isOnCurve)
                pts.push_back({ midpoint(c, d), true });
        }

        // 2. Commencer sur un point on-curve
        size_t s = 0;
        while (s < pts.size() && !pts[s].isOn) ++s;
        if (s == pts.size()) { first = last + 1; continue; }
        std::rotate(pts.begin(), pts.begin() + s, pts.end());

        // 3. Parcours
        size_t m = pts.size();
        std::vector<Maths::Vec2S> poly;
        Maths::Vec2S cur = pts[0].p;
        poly.push_back(cur);

        size_t i = 1;
        while (i <= m)
        {
            const P& q = pts[i % m];
            if (q.isOn)
            {
                poly.push_back(q.p);
                cur = q.p;
                i += 1;
            }
            else
            {
                Maths::Vec2S endPoint = pts[(i + 1) % m].p;      // forcément on-curve
                flattenQuad(cur, q.p, endPoint, steps, poly);
                cur = endPoint;
                i += 2;
            }
        }

        // 4. Le dernier point retombe sur le premier : on retire le doublon
        if (poly.size() > 1)
            poly.pop_back();

        contours.push_back(std::move(poly));
        first = last + 1;
    }
    return contours;
}

uint16_t Font::getAdvanceWidth(uint16_t glyphIndex) const
{
    uint16_t hMetricCount = readU16(tables_.at("hhea").offset + 34);
    if (glyphIndex >= hMetricCount)
        glyphIndex = hMetricCount - 1;
    return readU16(tables_.at("hmtx").offset + 4 * glyphIndex);
}


void Font::fill(const std::vector<std::vector<Maths::Vec2S>>& contours, Bitmap& bmp)
{
    for (int y = 0; y < bmp.height; ++y)
    {
        float sy = y + 0.5f;
        std::vector<float> xs;

        for (const auto& poly : contours)
            for (size_t i = 0; i < poly.size(); ++i)
            {
                Maths::Vec2S a = poly[i], b = poly[(i + 1) % poly.size()];
                if ((a.y <= sy) != (b.y <= sy))
                    xs.push_back(a.x + (sy - a.y) / (b.y - a.y) * (b.x - a.x));
            }
        std::sort(xs.begin(), xs.end());

        for (size_t i = 0; i + 1 < xs.size(); i += 2)
        {
            int x0 = std::max(0, int(std::ceil(xs[i] - 0.5f)));
            int x1 = std::min(bmp.width, int(std::ceil(xs[i + 1] - 0.5f)));
            for (int x = x0; x < x1; ++x)
                bmp.pixels[y * bmp.width + x] = 255;
        }
    }
}


Bitmap Font::renderText(const std::string& text, float pixelSize) const
{
    float scale = pixelSize / unitsPerEm_;
    float ascent = getAscender() * scale;
    float descent = -getDescender() * scale;

    float total = 0;
    for (char c : text)
        total += getAdvanceWidth(getGlyphIndex((unsigned char)c)) * scale;

    Bitmap bmp(int(std::ceil(total)) + 1, int(std::ceil(ascent + descent)) + 1);

    float penX = 0;
    for (char c : text)
    {
        uint16_t glyphIndex = getGlyphIndex((unsigned char)c);

        GlyphOutline outline;
        if (loadGlyph(glyphIndex, outline))
        {
            auto contours = flattenOutline(outline);
            for (auto& poly : contours)
                for (auto& p : poly)
                {
                    p.x = penX + p.x * scale;      // échelle + décalage
                    p.y = ascent - p.y * scale;    // inversion de Y
                }
            fill(contours, bmp);
        }
        penX += getAdvanceWidth(glyphIndex) * scale;   // on avance le curseur
    }
    return bmp;
}