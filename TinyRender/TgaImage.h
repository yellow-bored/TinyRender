#pragma once

#include <cstdint>
#include <fstream>
#include <vector>

#pragma pack(push,1)
struct TGAHeader {
    std::uint8_t  idlength = 0;
    std::uint8_t  colormaptype = 0;
    std::uint8_t  datatypecode = 0;
    std::uint16_t colormaporigin = 0;
    std::uint16_t colormaplength = 0;
    std::uint8_t  colormapdepth = 0;
    std::uint16_t x_origin = 0;
    std::uint16_t y_origin = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t  bitsperpixel = 0;
    std::uint8_t  imagedescriptor = 0;
};
#pragma pack(pop)

struct TgaColor {
    std::uint8_t bgra[4] = { 0,0,0,0 };
    std::uint8_t bytespp = 4;
    std::uint8_t& operator[](const int i) {
        return bgra[i];
    }
};

class TgaImage
{
public:
    //图片格式
    enum Format {
        GRAYSCALE = 1,
        RGB = 3,
        RGBA = 4,
    }; // 对应通道数量
    TgaImage() = default;
    TgaImage(const int width, const int height, const int bpp);

    //文件操作
    bool  read_tga_file(const std::string filename);
    bool Write_Tga_File(const std::string filename, const bool vflip = true, const bool rle = true) const;

    TgaColor Get(const int x, const int y) const;
    void Set(const int x, const int y, const TgaColor& c);

    int Width() const;
    int Height() const;

    // 上下和水平翻转
    void flip_horizontally();
    void flip_vertically();

private:
    bool load_rle_data(std::ifstream& in);
    bool unload_rle_data(std::ofstream& out) const;
    
    
    int m_Width, m_Height;
    std::uint8_t m_bpp = 0; // 类型
    std::vector<std::uint8_t> m_data = {};



};

