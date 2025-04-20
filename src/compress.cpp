#include "utils.h"

void compress(const std::string& inputFilePath, const std::string& outputFilePath, int quality)
{
    uint16_t idx = 0;

    std::cout << "Processing frames..." << std::endl;
    std::vector<std::vector<YCbCr>> yuvFrames = processFramesForCompression(inputFilePath);

#ifdef DEBUG_PROCESS
    std::ofstream processFile("/home/user/Projects/SMM/debug/yuv_frames_output.txt");
    if (!processFile.is_open())
    {
        std::cerr << "Failed to open file for writing YUV frames!" << std::endl;
        return;
    }

    for (size_t frameIndex = 0; frameIndex < yuvFrames.size(); ++frameIndex)
    {
        processFile << "Frame " << frameIndex + 1 << ":\n";

        for (size_t pixelIndex = 0; pixelIndex < yuvFrames[frameIndex].size(); ++pixelIndex)
        {
            const YCbCr& pixel = yuvFrames[frameIndex][pixelIndex];
            processFile << "Pixel " << pixelIndex << ": Y = " << static_cast<int>(pixel.y)
                       << ", Cb = " << static_cast<int>(pixel.cb)
                       << ", Cr = " << static_cast<int>(pixel.cr) << "\n";
        }

        processFile << "----------------------------------------\n";
    }
    
    processFile.close();

    std::cout << "Total YUV frames: " << yuvFrames.size() << std::endl;
    for(size_t i = 0; i < yuvFrames.size(); i++)
    {
        std::cout << "Frame " << i << "   \thas\t" << yuvFrames[i].size() << "\tpixels" << std::endl;
    }
#endif // DEBUG_PROCESS

    std::cout << "Processing blocks..." << std::endl;
    std::vector<std::vector<float>> blocks = segmentFramesToBlocks(yuvFrames);

#ifdef DEBUG_BLOCKS
    std::ofstream blocksFile("/home/user/Projects/SMM/debug/blocks_output.txt");
    if (!blocksFile.is_open())
    {
        std::cerr << "Failed to open file for writing blocks!" << std::endl;
        return;
    }

    for (size_t blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
    {
        blocksFile << "Block " << blockIndex + 1 << ":\n";

        for (size_t valueIndex = 0; valueIndex < blocks[blockIndex].size(); ++valueIndex)
        {
            blocksFile << blocks[blockIndex][valueIndex] << " ";

            if ((valueIndex + 1) % 8 == 0)
            {
                blocksFile << "\n";
            }
        }

        blocksFile << "----------------------------------------\n";
    }

    blocksFile.close();
#endif // DEBUG_BLOCKS

    std::vector<std::array<std::array<float, 8>, 8>> quantizedBlocks;

    std::cout << "Aplying FDCT on blocks..." << std::endl;
    for(const auto& block: blocks)
    {
        float block2D[8][8];
        vectorTo2DArray(block, block2D);
        FDCT_2D(block2D);
        if(0 == idx % 3)
        {
            // FDCT_2D Y
            quantizeBlock(block2D, TABEL_QUANTIZARE_Y, quality);
        }
        else
        {
            // FDCT_2D CbCr
            quantizeBlock(block2D, TABEL_QUANTIZARE_CbCr, quality);
        }
        std::array<std::array<float, 8>, 8> block2DArray = convertToStdArray(block2D);
        quantizedBlocks.push_back(block2DArray);
        idx++;
    }

#ifdef DEBUG_QUANTIZED_BLOCKS
    std::ofstream quantized_blocks("/home/user/Projects/SMM/debug/quantized_blocks_output.txt");
    if (!quantized_blocks.is_open())
    {
        std::cerr << "Failed to open file for writing quantized blocks!" << std::endl;
        return;
    }

    for (size_t blockIndex = 0; blockIndex < quantizedBlocks.size(); ++blockIndex)
    {
        quantized_blocks << "Quantized Block " << blockIndex + 1 << ":\n";

        for (size_t i = 0; i < 8; ++i)
        {
            for (size_t j = 0; j < 8; ++j)
            {
                quantized_blocks << quantizedBlocks[blockIndex][i][j] << " ";
            }
            quantized_blocks << "\n";
        }

        quantized_blocks << "----------------------------------------\n";
    }
    quantized_blocks.close();
#endif //DEBUG_QUANTIZED_BLOCKS

    std::cout << "Recomposing frame..." << std::endl;
    std::vector<uint8_t> largeBlock = recomposeFrame(quantizedBlocks);

#ifdef DEBUG_LARGE_BLOCK
    std::ofstream lBlockFile("/home/user/Projects/SMM/debug/large_block_output.txt");
    if (!lBlockFile.is_open())
    {
        std::cerr << "Failed to open file for writing large block!" << std::endl;
        return;
    }

    for (size_t i = 0; i < largeBlock.size(); ++i)
    {
        lBlockFile << "Byte " << i << ": " << static_cast<int>(largeBlock[i]) << "\n";
    }

    lBlockFile.close();
#endif //DEBUG_LARGE_BLOCK

    std::cout << "Encoding..." << std::endl;
    uint32_t numFrames = largeBlock.size() / RGB_CIF_SIZE;
    std::vector<uint8_t> header;

    std::cout << "Writting to file..." << std::endl;
    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile.is_open())
    {
        std::cerr << "Failed to open output file: " << outputFilePath << std::endl;
        return;
    }
    outputFile.write("SMP", 3);

    uint16_t width = CIF_X;
    uint16_t height = CIF_Y;
    outputFile.write(reinterpret_cast<const char*>(&width), sizeof(uint16_t));
    outputFile.write(reinterpret_cast<const char*>(&height), sizeof(uint16_t));
    outputFile.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
    outputFile.write(reinterpret_cast<const char*>(&quality), sizeof(quality));

    uint64_t nextFrameOffset = 0;
    uint8_t frameType = 0;
    for (size_t frameIndex = 0; frameIndex < numFrames; ++frameIndex)
    {
        std::vector<uint8_t> currentFrame(
            largeBlock.begin() + frameIndex * RGB_CIF_SIZE,
            largeBlock.begin() + (frameIndex + 1) * RGB_CIF_SIZE
        );
        std::vector<uint8_t> compressedData = encodeHuffman(currentFrame, header);

#ifdef DEBUG_HUFFMAN
            std::ofstream headerFile("/home/user/Projects/SMM/debug/header_output.txt");
            if (!headerFile.is_open())
            {
                std::cerr << "Failed to open file for writing header!" << std::endl;
                return;
            }

            for (size_t i = 0; i < header.size(); ++i)
            {
                headerFile << "Byte " << i << ": " << static_cast<int>(header[i]) << "\n";
            }
            headerFile.close();

            std::ofstream compressedFile("/home/user/Projects/SMM/debug/compressed_data_output.txt");
            if (!compressedFile.is_open())
            {
                std::cerr << "Failed to open file for writing compressed data!" << std::endl;
                return;
            }

            for (size_t i = 0; i < compressedData.size(); ++i)
            {
                compressedFile << "Byte " << i << ": " << static_cast<int>(compressedData[i]) << "\n";
            }
            compressedFile.close();

            std::cout << "Compressed data size: " << compressedData.size() << " bytes" << std::endl;
            std::cout << "Header size: " << header.size() << " bytes" << std::endl;
            std::cout << "Total size: " << compressedData.size() + header.size() << " bytes" << std::endl;
            std::cout << "Total frames: " << numFrames << std::endl;
#endif //DEBUG_HUFFMAN

        frameType = (frameIndex % 32 == 0) ? 0 : 1;
        nextFrameOffset = (frameIndex + 1 < numFrames)
                            ? (outputFile.tellp() + static_cast<std::streamoff>(compressedData.size() + sizeof(uint64_t) + 1))
                            : std::streampos(0);

        outputFile.write(reinterpret_cast<const char*>(&nextFrameOffset), sizeof(nextFrameOffset));
        outputFile.write(reinterpret_cast<const char*>(&frameType), sizeof(frameType));
        outputFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());

#ifdef DEBUG_HUFFMAN
    std::cout << "Frame " << frameIndex << ": Compressed data size = " << compressedData.size() << " bytes" << std::endl;
    std::cout << "Frame " << frameIndex << ": Next frame offset = " << nextFrameOffset << std::endl;
#endif
    }

    outputFile.close();

    std::cout << "Compression completed successfully!" << std::endl
                << "Output file: " << outputFilePath << std::endl;

}

std::vector<std::vector<YCbCr>> processFramesForCompression(const std::string& inputFilePath)
{
    std::ifstream inputFile(inputFilePath, std::ios::binary);

    std::vector<uint8_t> currentFrame(RGB_CIF_SIZE);
    std::vector<YCbCr> currFrame(CIF_SIZE);
    std::vector<YCbCr> prevFrame(CIF_SIZE);
    std::vector<std::vector<YCbCr>> yuvFrames;
    YCbCr pixels = {0, 0, 0};

    if (!inputFile.is_open())
    {
        std::cerr << "Failed to open input file: " << inputFilePath << std::endl;
    }
    else
    {
        size_t frameIndex = 0;

#ifdef DEBUG_COMPRESS
        std::ofstream compressFile("/home/user/Projects/SMM/debug/compress.txt", std::ios::app);
        if (!compressFile.is_open())
        {
            std::cerr << "Failed to open compressFile file!" << std::endl;
        }
#endif // DEBUG_COMPRESS
        while (inputFile.read(reinterpret_cast<char*>(currentFrame.data()), RGB_CIF_SIZE))
        {
#ifdef DEBUG_COMPRESS
            std::streamsize bytesRead = inputFile.gcount();
            std::cout << "Bytes read for frame " << frameIndex << ": " << bytesRead << std::endl;
#endif //DEBUG_COMPRESS
            currFrame.clear();
            for (size_t i = 0; i < CIF_SIZE; i++)
            {
                RGB rgb = {currentFrame[i], currentFrame[i + CIF_SIZE], currentFrame[i + 2*CIF_SIZE]};
                pixels = rgbToYuv(rgb);
                if ((0 != frameIndex % 32 )  &&  (0 != frameIndex))
                {
                    pixels.y = DPCM_8BIT(pixels.y, prevFrame[i].y);
                    pixels.cb = DPCM_8BIT(pixels.cb, prevFrame[i].cb);
                    pixels.cr = DPCM_8BIT(pixels.cr, prevFrame[i].cr);
#ifdef DEBUG_COMPRESS
                    compressFile <<"Y: " << static_cast<int>(pixels.y) << " Cb: " << static_cast<int>(pixels.cb) << " Cr: " << static_cast<int>(pixels.cr) << std::endl;
#endif // DEBUG_COMPRESS
                }
#ifdef DEBUG_COMPRESS
                else
                {
                    compressFile << frameIndex << std::endl;
                    compressFile <<"Y: " << static_cast<int>(pixels.y) << " Cb: " << static_cast<int>(pixels.cb) << " Cr: " << static_cast<int>(pixels.cr) << std::endl;
                }
#endif // DEBUG_COMPRESS
                currFrame.push_back(pixels);
            }
            yuvFrames.push_back(currFrame);
            prevFrame = currFrame;

            ++frameIndex;
        }
#ifdef DEBUG_COMPRESS
        compressFile.close();
#endif // DEBUG_COMPRESS
        inputFile.close();
    }
    return yuvFrames;
}

std::vector<std::vector<float>> segmentFramesToBlocks(const std::vector<std::vector<YCbCr>>& yuvFrames)
{
    std::vector<std::vector<float>> blocks;

    for (size_t frameIndex = 0; frameIndex < yuvFrames.size(); ++frameIndex)
    {
        const auto& yuvFrame = yuvFrames[frameIndex];

        for (size_t y = 0; y < CIF_Y; y += BLOCK_SIZE)
        {
            for (size_t x = 0; x < CIF_X; x += BLOCK_SIZE)
            {
                std::vector<float> blockY;
                std::vector<float> blockCb;
                std::vector<float> blockCr;

                for (size_t i = 0; i < BLOCK_SIZE; ++i)
                {
                    for (size_t j = 0; j < BLOCK_SIZE; ++j)
                    {
                        size_t pixelIndex = (y + i) * CIF_X + (x + j);
                        if (y + i < CIF_Y && x + j < CIF_X)
                        {
                            blockY.push_back(yuvFrame[pixelIndex].y);
                            blockCb.push_back(yuvFrame[pixelIndex].cb);
                            blockCr.push_back(yuvFrame[pixelIndex].cr);
                        }
                    }
                }
                blocks.push_back(blockY);
                blocks.push_back(blockCb);
                blocks.push_back(blockCr);
            }
        }
    }
    return blocks;
}

void quantizeBlock(float block[8][8], const unsigned char quantTable[8][8], int quality)
{
    uint32_t localQuantTable[8][8];
    if(quality < 50)
    {
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                localQuantTable[i][j] = std::max(1, (int)(quantTable[i][j] * (5000 / quality)));
            }
        }
    }
    else if (quality > 50)
    {
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                localQuantTable[i][j] = std::max(1, (int)(quantTable[i][j] * (200 - quality * 2)));
            }
        }
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                localQuantTable[i][j] = quantTable[i][j];
            }
        }
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            block[i][j] = round(block[i][j] / localQuantTable[i][j]);
            block[i][j] = std::max(8.0f, std::min(255.0f, block[i][j]));
        }
    }
}

void FDCT_2D(float block[8][8])
{
	float p1[8], p2[8], p3[8], p4[8], p5[8], p6[8];
	size_t i;

	for (i = 0; i < 8; i++)
    {
		// FDCT 1D Phase 1
        p1[0] = block[i][0] + block[i][7];
        p1[1] = block[i][1] + block[i][6];
        p1[2] = block[i][2] + block[i][5];
        p1[3] = block[i][3] + block[i][4];
        p1[4] = block[i][3] - block[i][4];
        p1[5] = block[i][2] - block[i][5];
        p1[6] = block[i][1] - block[i][6];
        p1[7] = block[i][0] - block[i][7];
        // FDCT 1D Phase 2
        p2[0] = p1[0] + p1[3];
        p2[1] = p1[1] + p1[2];
        p2[2] = p1[1] - p1[2];
        p2[3] = p1[0] - p1[3];
        p2[4] = -( p1[4] + p1[5] );
        p2[5] = p1[5] + p1[6];
        p2[6] = p1[6] + p1[7];
        p2[7] = p1[7];
        // FDCT 1D Phase 3
        p3[0] = p2[0] + p2[1];
        p3[1] = p2[0] - p2[1];
        p3[2] = p2[2] + p2[3];
        p3[3] = p2[3];
        p3[4] = p2[4];
        p3[5] = p2[5];
        p3[6] = p2[6];
        p3[7] = p2[7];
        // FDCT 1D Phase 4
        p4[0] = p3[0];
        p4[1] = p3[1];
        p4[2] = p3[2] * c4;
        p4[3] = p3[3];
        p4[4] = -( ( p3[4] + p3[6] ) * c6 + p3[4] * ( c2 - c6 ) );
        p4[5] = p3[5] * c4;
        p4[6] = p3[6] * ( c2 + c6 ) - ( p3[4] + p3[6] ) * c6;
        p4[7] = p3[7];
        // FDCT 1D Phase 5
        p5[0] = p4[0];
        p5[1] = p4[1];
        p5[2] = p4[2] + p4[3];
        p5[3] = p4[3] - p4[2];
        p5[4] = p4[4];
        p5[5] = p4[5] + p4[7];
        p5[6] = p4[6];
        p5[7] = p4[7] - p4[5];
        // FDCT 1D Phase 6
        p6[0] = p5[0];
        p6[1] = p5[1];
        p6[2] = p5[2];
        p6[3] = p5[3];
        p6[4] = p5[4] + p5[7];
        p6[5] = p5[5] + p5[6];
        p6[6] = p5[5] - p5[6];
        p6[7] = p5[7] - p5[4];
        // FDCT 1D Phase 7
        block[i][0] = p6[0] * 1 / ( 2 * sqrt(2) );
        block[i][1] = p6[1] * 1 / ( 4 * c4 );
        block[i][2] = p6[2] * 1 / ( 4 * c2 );
        block[i][3] = p6[3] * 1 / ( 4 * c6 );
        block[i][4] = p6[4] * 1 / ( 4 * c5 );
        block[i][5] = p6[5] * 1 / ( 4 * c1 );
        block[i][6] = p6[6] * 1 / ( 4 * c7 );
        block[i][7] = p6[7] * 1 / ( 4 * c3 );
	}
	// then process columns
	for (i = 0; i < 8; i++)
    {
		// FDCT 1D Phase 1
        p1[0] = block[0][i] + block[7][i];
        p1[1] = block[1][i] + block[6][i];
        p1[2] = block[2][i] + block[5][i];
        p1[3] = block[3][i] + block[4][i];
        p1[4] = block[3][i] - block[4][i];
        p1[5] = block[2][i] - block[5][i];
        p1[6] = block[1][i] - block[6][i];
        p1[7] = block[0][i] - block[7][i];
        // FDCT 1D Phase 2
        p2[0] = p1[0] + p1[3];
        p2[1] = p1[1] + p1[2];
        p2[2] = p1[1] - p1[2];
        p2[3] = p1[0] - p1[3];
        p2[4] = -( p1[4] + p1[5] );
        p2[5] = p1[5] + p1[6];
        p2[6] = p1[6] + p1[7];
        p2[7] = p1[7];
        // FDCT 1D Phase 3
        p3[0] = p2[0] + p2[1];
        p3[1] = p2[0] - p2[1];
        p3[2] = p2[2] + p2[3];
        p3[3] = p2[3];
        p3[4] = p2[4];
        p3[5] = p2[5];
        p3[6] = p2[6];
        p3[7] = p2[7];
        // FDCT 1D Phase 4
        p4[0] = p3[0];
        p4[1] = p3[1];
        p4[2] = p3[2] * c4;
        p4[3] = p3[3];
        p4[4] = -( ( p3[4] + p3[6] ) * c6 + p3[4] * ( c2 - c6 ) );
        p4[5] = p3[5] * c4;
        p4[6] = p3[6] * ( c2 + c6 ) - ( p3[4] + p3[6] ) * c6;
        p4[7] = p3[7];
        // FDCT 1D Phase 5
        p5[0] = p4[0];
        p5[1] = p4[1];
        p5[2] = p4[2] + p4[3];
        p5[3] = p4[3] - p4[2];
        p5[4] = p4[4];
        p5[5] = p4[5] + p4[7];
        p5[6] = p4[6];
        p5[7] = p4[7] - p4[5];
        // FDCT 1D Phase 6
        p6[0] = p5[0];
        p6[1] = p5[1];
        p6[2] = p5[2];
        p6[3] = p5[3];
        p6[4] = p5[4] + p5[7];
        p6[5] = p5[5] + p5[6];
        p6[6] = p5[5] - p5[6];
        p6[7] = p5[7] - p5[4];
        // FDCT 1D Phase 7
        block[0][i] = p6[0] * 1 / ( 2 * sqrt(2) );
        block[1][i] = p6[1] * 1 / ( 4 * c4 );
        block[2][i] = p6[2] * 1 / ( 4 * c2 );
        block[3][i] = p6[3] * 1 / ( 4 * c6 );
        block[4][i] = p6[4] * 1 / ( 4 * c5 );
        block[5][i] = p6[5] * 1 / ( 4 * c1 );
        block[6][i] = p6[6] * 1 / ( 4 * c7 );
        block[7][i] = p6[7] * 1 / ( 4 * c3 );
	}
}



