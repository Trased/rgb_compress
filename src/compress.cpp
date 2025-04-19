#include "utils.h"

void compress(const std::string& inputFilePath, const std::string& outputFilePath, int quality)
{
    uint16_t idx = 0;

    std::cout << "Processing frames..." << std::endl;
    std::vector<YCbCr> yuvFrames = processFramesForCompression(inputFilePath);

    std::cout << "Processing blocks..." << std::endl;
    std::vector<std::vector<float>> blocks = segmentFramesToBlocks(yuvFrames);

    std::cout << "Quantizing blocks..." << std::endl;
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
    std::cout << "Recomposing frame..." << std::endl;
    std::vector<uint8_t> largeBlock = recomposeFrame(quantizedBlocks);

    std::cout << "Encoding..." << std::endl;
    std::vector<uint8_t> header;
    std::vector<uint8_t> compressedData = encodeHuffman(largeBlock, header);

    std::cout << "Writting to file..." << std::endl;
    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile.is_open())
    {
        std::cerr << "Failed to open output file: " << outputFilePath << std::endl;
        return;
    }

    outputFile.write(reinterpret_cast<const char*>(header.data()), header.size());
    outputFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
    outputFile.close();
    std::cout << "Compression completed successfully!" << std::endl
                << "Output file: " << outputFilePath << std::endl;

}

std::vector<YCbCr> processFramesForCompression(const std::string& inputFilePath)
{
    std::ifstream inputFile(inputFilePath, std::ios::binary);

    std::vector<uint8_t> currentFrame(RGB_CIF_SIZE);
    std::vector<uint8_t> previousFrame(RGB_CIF_SIZE, 0);
    std::vector<YCbCr> yuvFrames(RGB_CIF_SIZE / 3);

    if (!inputFile.is_open())
    {
        std::cerr << "Failed to open input file: " << inputFilePath << std::endl;
    }
    else
    {
        size_t frameIndex = 1;

        while (inputFile.read(reinterpret_cast<char*>(currentFrame.data()), RGB_CIF_SIZE))
        {
            for (size_t i = 0; i < RGB_CIF_SIZE; i += 3)
            {
                RGB rgb = {currentFrame[i], currentFrame[i + 1], currentFrame[i + 2]};
                yuvFrames[i / 3] = rgbToYuv(rgb);
            }

            /// TODO :: DPCM for each non Key frame.
            /// REANALYZE THIS FUNCTION

            if (frameIndex % 32 == 0)
            {
                std::cout << "Processing key-frame: " << frameIndex << std::endl;
            }
            else
            {
                std::vector<int8_t> deltaFrame(RGB_CIF_SIZE / 3 * 3);
                for (size_t i = 0; i < yuvFrames.size(); ++i)
                {
                    deltaFrame[i * 3 + 0] = static_cast<int8_t>(yuvFrames[i].y - previousFrame[i * 3 + 0]);
                    deltaFrame[i * 3 + 1] = static_cast<int8_t>(yuvFrames[i].cb - previousFrame[i * 3 + 1]);
                    deltaFrame[i * 3 + 2] = static_cast<int8_t>(yuvFrames[i].cr - previousFrame[i * 3 + 2]);
                }

            }
            std::memcpy(previousFrame.data(), currentFrame.data(), RGB_CIF_SIZE);

            ++frameIndex;
        }
        inputFile.close();
    }
    return yuvFrames;
}

std::vector<std::vector<float>> segmentFramesToBlocks(const std::vector<YCbCr>& yuvFrames)
{
    std::vector<std::vector<float>> blocks;

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
                        blockY.push_back(yuvFrames[pixelIndex].y);
                        blockCb.push_back(yuvFrames[pixelIndex].cb);
                        blockCr.push_back(yuvFrames[pixelIndex].cr);
                    }
                }
            }
            blocks.push_back(blockY);
            blocks.push_back(blockCb);
            blocks.push_back(blockCr);
        }
    }

    return blocks;
}

void quantizeBlock(float block[8][8], const unsigned char quantTable[8][8], int quality) {
    unsigned char localQuantTable[8][8];
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

void FDCT_2D(float block[8][8]) {
	float p1[8], p2[8], p3[8], p4[8], p5[8], p6[8];
	size_t i;

	for (i = 0; i < 8; i++) {
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
	for (i = 0; i < 8; i++) {
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



