#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Font
{
public:

    bool loadFont(const std::string& path);

private:

    std::vector<uint8_t> m_data;


};