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

void vectorTo2DArray(const std::vector<float>& vec, float array[8][8])
{
    for (size_t i = 0; i < 8; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            array[i][j] = vec[i * 8 + j];
        }
    }
}

std::vector<uint8_t> recomposeFrame(const std::vector<std::array<std::array<float, 8>, 8>>& quantizedBlocks)
{
    std::vector<uint8_t> frame;
#ifdef DEBUG_LARGE_BLOCK
    std::cout << "Recomposing frame with " << quantizedBlocks.size() << " blocks\n";
    std::cout << "Total size: " << quantizedBlocks.size() * 8 * 8 << " bytes\n";
#endif //DEBUG_LARGE_BLOCK
    frame.reserve(quantizedBlocks.size() * 8 * 8);

    for(const auto& block :quantizedBlocks)
    {
        for(const auto& row : block)
        {
            for(const auto& value : row)
            {
                frame.push_back(static_cast<uint8_t>(value));
            }
        }
    }
    return frame;
}

std::array<std::array<float, 8>, 8> convertToStdArray(float var[8][8])
{
    std::array<std::array<float, 8>, 8> result;
    for (size_t i = 0; i < 8; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            result[i][j] = var[i][j];
        }
    }
    return result;
}