#include "Font.h"
#include <fstream>
#include <iostream>
#include <cstring>

bool Font::loadFont(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) 
    {
        std::cerr << "Cannot open " << path << "\n"; return false; 
    }

    return true;
}