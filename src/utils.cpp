#include "utils.h"

CommandUsed findCommand(std::string command)
{
    CommandUsed comm = CommandUsed::UNKNOWN;
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    if ("-h" == command || "/h" == command || "/?" == command)
    {
        comm = CommandUsed::HELP;
    }
    else if ("-c" == command || "/c" == command)
    {
        comm = CommandUsed::COMPRESS;
    }
    else
    {
        if ("-u" == command || "/u" == command)
        {
            comm = CommandUsed::DECOMPRESS;
        }
    }

    return comm;
}

void printHelp()
{
    std::cout <<
        "-c or /c [quality] [input filepath] [output filepath]\n"
        "\tCompresses a CIF RGB24 file using a specified [quality] (1-100),\n"
        "\tfrom [input filepath] to [output filepath]\n"
        "-u or /u [input filepath] [output filepath]\n"
        "\tUncompresses a compressed file from [input filepath] to [output filepath]\n";
}

YCbCr rgbToYuv(const RGB& rgb)
{
    YCbCr ycbcr;
    ycbcr.y = FP_RGB2Y(rgb.r, rgb.g, rgb.b);
    ycbcr.cb = FP_RGB2Cb(rgb.r, rgb.g, rgb.b);
    ycbcr.cr = FP_RGB2Cr(rgb.r, rgb.g, rgb.b);
    return ycbcr;
}

RGB yuvToRgb(const YCbCr& ycbcr)
{
    RGB rgb;
    rgb.r = FP_YUV2R(ycbcr.y, ycbcr.cb, ycbcr.cr);
    rgb.b = FP_YUV2B(ycbcr.y, ycbcr.cb, ycbcr.cr);
    rgb.g = FP_YUV2G(ycbcr.y, ycbcr.cb, ycbcr.cr);

    return rgb;
}

void vectorTo2DArray(const std::vector<float>& vec, float array[8][8]) {
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            array[i][j] = vec[i * 8 + j];
        }
    }
}

std::vector<uint8_t> recomposeFrame(const std::vector<std::array<std::array<float, 8>, 8>>& quantizedBlocks) {
    std::vector<uint8_t> frame(CIF_X * CIF_Y * 3, 0);
    size_t blockIndex = 0;

    for (size_t y = 0; y < CIF_Y; y += 8) {
        for (size_t x = 0; x < CIF_X; x += 8) {
            // Copy Y block
            for (size_t i = 0; i < 8; ++i) {
                for (size_t j = 0; j < 8; ++j) {
                    size_t pixelIndex = (y + i) * CIF_X + (x + j);
                    if (y + i < CIF_Y && x + j < CIF_X) {
                        frame[pixelIndex] = static_cast<uint8_t>(quantizedBlocks[blockIndex][i][j]);
                    }
                }
            }
            blockIndex++;

            for (size_t i = 0; i < 8; ++i) {
                for (size_t j = 0; j < 8; ++j) {
                    size_t pixelIndex = ((y + i) * CIF_X + (x + j)) * 3 + 1;
                    if (y + i < CIF_Y && x + j < CIF_X) {
                        frame[pixelIndex] = static_cast<uint8_t>(quantizedBlocks[blockIndex][i][j]);
                    }
                }
            }
            blockIndex++;

            for (size_t i = 0; i < 8; ++i) {
                for (size_t j = 0; j < 8; ++j) {
                    size_t pixelIndex = ((y + i) * CIF_X + (x + j)) * 3 + 2;
                    if (y + i < CIF_Y && x + j < CIF_X) {
                        frame[pixelIndex] = static_cast<uint8_t>(quantizedBlocks[blockIndex][i][j]);
                    }
                }
            }
            blockIndex++;
        }
    }
    frame.push_back(255);
    return frame;
}

std::array<std::array<float, 8>, 8> convertToStdArray(float var[8][8]) {
    std::array<std::array<float, 8>, 8> result;
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            result[i][j] = var[i][j];
        }
    }
    return result;
}