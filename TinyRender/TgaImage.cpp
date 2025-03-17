#include "TgaImage.h"
#include <cstring>
#include <iostream>


TgaImage::TgaImage(const int width, const int height, const int bpp) :m_Width(width), m_Height(height), m_bpp(bpp), m_data(width* height* bpp) {}

bool TgaImage::read_tga_file(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }
    TGAHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!in.good()) {
        std::cerr << "an error occured while reading the header\n";
        return false;
    }
    m_Width = header.width;
    m_Height = header.height;
    m_bpp = header.bitsperpixel >> 3;
    if (m_Width <= 0 || m_Height <= 0 || (m_bpp != GRAYSCALE && m_bpp != RGB && m_bpp != RGBA)) {
        std::cerr << "bad pp (or width/height) value\n";
        return false;
    }
    size_t nbytes = m_bpp * m_Width * m_Height;
    m_data = std::vector<std::uint8_t>(nbytes, 0);
    if (3 == header.datatypecode || 2 == header.datatypecode) {
        in.read(reinterpret_cast<char*>(m_data.data()), nbytes);
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    }
    else if (10 == header.datatypecode || 11 == header.datatypecode) {
        if (!load_rle_data(in)) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    }
    else {
        std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
        return false;
    }
    if (!(header.imagedescriptor & 0x20))
        flip_vertically();
    if (header.imagedescriptor & 0x10)
        flip_horizontally();
    std::cerr << m_Width << "x" << m_Height << "/" << m_bpp * 8 << "\n";
    return true;
}

bool TgaImage::load_rle_data(std::ifstream& in) {
    size_t pixelcount = m_Width * m_Height;
    size_t currentpixel = 0;
    size_t currentbyte = 0;
    TgaColor colorbuffer;
    do {
        std::uint8_t chunkheader = 0;
        chunkheader = in.get();
        if (!in.good()) {
            std::cerr << "an error occured m_Widthhile reading the data\n";
            return false;
        }
        if (chunkheader < 128) {
            chunkheader++;
            for (int i = 0; i < chunkheader; i++) {
                in.read(reinterpret_cast<char*>(colorbuffer.bgra), m_bpp);
                if (!in.good()) {
                    std::cerr << "an error occured while reading the header\n";
                    return false;
                }
                for (int t = 0; t < m_bpp; t++)
                    m_data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel > pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
        else {
            chunkheader -= 127;
            in.read(reinterpret_cast<char*>(colorbuffer.bgra), m_bpp);
            if (!in.good()) {
                std::cerr << "an error occured while reading the header\n";
                return false;
            }
            for (int i = 0; i < chunkheader; i++) {
                for (int t = 0; t < m_bpp; t++)
                    m_data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel > pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);
    return true;
}


bool TgaImage::Write_Tga_File(const std::string filename, const bool vflip, const bool rle) const {
    constexpr std::uint8_t developer_area_ref[4] = { 0, 0, 0, 0 };
    constexpr std::uint8_t extension_area_ref[4] = { 0, 0, 0, 0 };
    constexpr std::uint8_t footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }
    TGAHeader header = {};
    header.bitsperpixel = m_bpp << 3;
    header.width = m_Width;
    header.height = m_Height;
    header.datatypecode = (m_bpp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
    header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!out.good()) goto err;
    if (!rle) {
        out.write(reinterpret_cast<const char*>(m_data.data()), m_Width * m_Height * m_bpp);
        if (!out.good()) goto err;
    }
    else if (!unload_rle_data(out)) goto err;
    out.write(reinterpret_cast<const char*>(developer_area_ref), sizeof(developer_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char*>(extension_area_ref), sizeof(extension_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char*>(footer), sizeof(footer));
    if (!out.good()) goto err;
    return true;
err:
    std::cerr << "can't dump the tga file\n";
    return false;
}

bool TgaImage::unload_rle_data(std::ofstream& out) const {
    const std::uint8_t max_chunk_length = 128;
    size_t npixels = m_Width * m_Height;
    size_t curpix = 0;
    while (curpix < npixels) {
        size_t chunkstart = curpix * m_bpp;
        size_t curbyte = curpix * m_bpp;
        std::uint8_t run_length = 1;
        bool raw = true;
        while (curpix + run_length < npixels && run_length < max_chunk_length) {
            bool succ_eq = true;
            for (int t = 0; succ_eq && t < m_bpp; t++)
                succ_eq = (m_data[curbyte + t] == m_data[curbyte + t + m_bpp]);
            curbyte += m_bpp;
            if (1 == run_length)
                raw = !succ_eq;
            if (raw && succ_eq) {
                run_length--;
                break;
            }
            if (!raw && !succ_eq)
                break;
            run_length++;
        }
        curpix += run_length;
        out.put(raw ? run_length - 1 : run_length + 127);
        if (!out.good()) return false;
        out.write(reinterpret_cast<const char*>(m_data.data() + chunkstart), (raw ? run_length * m_bpp : m_bpp));
        if (!out.good()) return false;
    }
    return true;
}

TgaColor TgaImage::Get(const int x, const int y) const {
	if (m_data.empty() || x<0 || y<0 || x>m_Width || y>m_Height) return{};
	TgaColor tmp = { 0,0,0,0,m_bpp };
	const std::uint8_t* p = m_data.data() + (m_Width * y + x) * m_bpp;
	for (int i = 0; i < m_bpp; ++i) {
		tmp.bgra[i] = p[i];
	}
	return tmp;
}

void TgaImage::Set(int x, int y, const TgaColor& c) {
	if (m_data.empty() || x<0 || y<0 || x>m_Width || y>m_Height) return;
	memcpy(m_data.data() + (x + y * m_Width) * m_bpp, c.bgra, m_bpp);
}


void TgaImage::flip_horizontally() {
    for (int i = 0; i < m_Width / 2; i++)
        for (int j = 0; j < m_Height; j++)
            for (int b = 0; b < m_bpp; b++)
                std::swap(m_data[(i + j * m_Width) * m_bpp + b], m_data[(m_Width - 1 - i + j * m_Width) * m_bpp + b]);
}

void TgaImage::flip_vertically() {
    for (int i = 0; i < m_Width; i++)
        for (int j = 0; j < m_Height / 2; j++)
            for (int b = 0; b < m_bpp; b++)
                std::swap(m_data[(i + j * m_Width) * m_bpp + b], m_data[(i + (m_Height - 1 - j) * m_Width) * m_bpp + b]);
}


int TgaImage::Width() const {
	return m_Width;
}

int TgaImage::Height() const {
	return m_Height;
}