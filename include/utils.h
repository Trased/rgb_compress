#pragma once
#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <math.h>
#include <fstream>
#include <cstring>
#include <array>
#include <queue>
#include <unordered_map>
#include <bitset>

namespace fs = std::filesystem;

#define BLOCK_SIZE 8
#define CIF_X 352
#define CIF_Y 288
#define RGB_CIF_SIZE (CIF_X * CIF_Y * 3)

#define LIMIT(X) ( (X) < 0 ? 0 : (X) > 255 ? 255 : X )

#define FP_YUV2R(Y, Cb, Cr)  LIMIT( Y                      + 1.402   * (Cr-128) )
#define FP_YUV2G(Y, Cb, Cr)  LIMIT( Y - 0.34414 * (Cb-128) - 0.71414 * (Cr-128) )
#define FP_YUV2B(Y, Cb, Cr)  LIMIT( Y + 1.772   * (Cb-128)						)

#define FP_RGB2Y(R, G, B)   LIMIT(       (0.299    * R) + 0.587    * G + (0.114    * B) )
#define FP_RGB2Cb(R, G, B)  LIMIT( 128 - (0.168736 * R) - 0.331264 * G + (0.5      * B) )
#define FP_RGB2Cr(R, G, B)  LIMIT( 128 + (0.5      * R) - 0.418688 * G - (0.081312 * B) )

#define DPCM_8BIT(A, B) ((B-A) + 256 / 2)

/// ci = cos(i*pi/16)
#define c1 0.9807852804032304491262 // cos(pi/16)
#define c2 0.9238795325112867561282 // cos(2*pi/16) = cos(pi/8)
#define c3 0.8314696123025452370788 // cos(3*pi/16)
#define c4 0.7071067811865475244008 // cos(4*pi/16) = cos(pi/4)
#define c5 0.5555702330196022247428 // cos(5*pi/16)
#define c6 0.3826834323650897717285 // cos(6*pi/16) = cos(3*pi/8)
#define c7 0.1950903220161282678483 // cos(7*pi/16)

struct HuffmanNode
{
    uint8_t data;
    size_t frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(uint8_t data, size_t frequency)
        : data(data), frequency(frequency), left(nullptr), right(nullptr) {}
};

const unsigned char TABEL_QUANTIZARE_Y[8][8] =
{
    16,11,10,16,24, 40, 51, 61,
    12,12,14,19,26, 58, 60, 55,
    14,13,16,24,40, 57, 69, 56,
    14,17,22,29,51, 87, 80, 62,
    18,22,37,56,68, 109,103,77,
    24,35,55,64,81, 104,113,92,
    49,64,78,87,103,121,120,101,
    72,92,95,98,112,100,103,99
};

const unsigned char TABEL_QUANTIZARE_CbCr[8][8] =
{
    17,18,24,47,99,99,99,99,
    18,21,26,66,99,99,99,99,
    24,26,56,99,99,99,99,99,
    47,66,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99
};

struct RGB
{
    uint8_t r, g, b;
};

struct YCbCr
{
    uint8_t y, cb, cr;
};

struct Compare
{
    bool operator()(HuffmanNode* a, HuffmanNode* b)
    {
        return a->frequency > b->frequency;
    }
};

enum class CommandUsed
{
    FIRST       = 0,
    HELP        = FIRST + 0,
    COMPRESS    = FIRST + 1,
    DECOMPRESS  = FIRST + 2,
    UNKNOWN     = FIRST + 3,
    LAST
};

CommandUsed findCommand(std::string command);
void printHelp(void);
YCbCr rgbToYuv(const RGB& rgb);
RGB yuvToRgb(const YCbCr& ycbcr);
void vectorTo2DArray(const std::vector<float>& vec, float array[8][8]);
std::array<std::array<float, 8>, 8> convertToStdArray(float var[8][8]);

void compress(const std::string& inputFilePath, const std::string& outputFilePath, int quality);

//void decompress(const std::string& inputFilePath, const std::string& outputFilePath);
std::vector<YCbCr> processFramesForCompression(const std::string& inputFilePath);
std::vector<std::vector<float>> segmentFramesToBlocks(const std::vector<YCbCr>& yuvFrames);
void FDCT_2D(float block[8][8]);
void IDCT_2D(float block[8][8]);
void quantizeBlock(float block[8][8], const unsigned char quantTable[8][8], int quality);
std::vector<uint8_t> recomposeFrame(const std::vector<std::array<std::array<float, 8>, 8>>& quantizedBlocks);


HuffmanNode* buildHuffmanTree(const std::unordered_map<uint8_t, size_t>& frequencies);
void buildHuffmanCodes(HuffmanNode* root, std::string currentCode, std::unordered_map<uint8_t, std::string>& codes);
std::string encodeData(const std::vector<uint8_t>& data, const std::unordered_map<uint8_t, std::string>& codes);
std::vector<uint8_t> encodeHuffman(const std::vector<uint8_t>& data, std::vector<uint8_t>& header);
std::vector<uint8_t> compressData(const std::string& bitstream);


inline std::ostream& operator<<(std::ostream& os, CommandUsed cmd)
{
    switch (cmd)
    {
        case CommandUsed::HELP:       return os << "HELP";
        case CommandUsed::COMPRESS:   return os << "COMPRESS";
        case CommandUsed::DECOMPRESS: return os << "DECOMPRESS";
        case CommandUsed::UNKNOWN:    return os << "UNKNOWN";
        default:                      return os << "INVALID_COMMAND";
    }
}
