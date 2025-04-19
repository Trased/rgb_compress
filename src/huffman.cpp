#include "utils.h"

#include <thread>
void deleteHuffmanTree(HuffmanNode* root)
{
    if (!root) return;
    deleteHuffmanTree(root->left);
    deleteHuffmanTree(root->right);
    delete root;
}

HuffmanNode* buildHuffmanTree(const std::unordered_map<uint8_t, size_t>& frequencies)
{
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;

    for (const auto& pair : frequencies)
    {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (1 < pq.size())
    {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();

        HuffmanNode* parent = new HuffmanNode(0, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;

        pq.push(parent);
    }

    return pq.top();
}

void buildHuffmanCodes(HuffmanNode* root, std::string currentCode, std::unordered_map<uint8_t, std::string>& codes)
{
    if (!root) return;

    if (!root->left && !root->right)
    {
        codes[root->data] = currentCode;
        return;
    }

    buildHuffmanCodes(root->left, currentCode + '0', codes);
    buildHuffmanCodes(root->right, currentCode + '1', codes);
}

std::string encodeData(const std::vector<uint8_t>& data, const std::unordered_map<uint8_t, std::string>& codes)
{
    std::string encodedData;
    for (uint8_t byte : data)
    {
        encodedData += codes.at(byte);
    }
    return encodedData;
}

std::vector<uint8_t> compressData(const std::string& bitstream)
{
    std::vector<uint8_t> compressedData;
    for (size_t i = 0; i < bitstream.size(); i += 8)
    {
        std::bitset<8> byte(bitstream.substr(i, 8));
        compressedData.push_back(static_cast<uint8_t>(byte.to_ulong()));
    }

    return compressedData;
}

std::vector<uint8_t> encodeHuffman(const std::vector<uint8_t>& data, std::vector<uint8_t>& header)
{
    std::unordered_map<uint8_t, size_t> frequencies;

    for (uint8_t byte : data)
    {
        frequencies[byte]++;
    }

    HuffmanNode* root = buildHuffmanTree(frequencies);

    std::unordered_map<uint8_t, std::string> codes;

    buildHuffmanCodes(root, "", codes);

    header.resize(128, 0);
    for (uint16_t i = 0; i < 255; i += 2)
    {
        uint8_t len1 = codes.count(i) ? codes[i].size() : 0;
        uint8_t len2 = codes.count(i + 1) ? codes[i + 1].size() : 0;
        header[i / 2] = (len1 & 0xF) | ((len2 & 0xF) << 4);
    }
    std::cout << 5 << std::endl;

    std::string bitstream = encodeData(data, codes);

    std::cout << 6 << std::endl;


    std::vector<uint8_t> compressedData = compressData(bitstream);

    std::cout << 7 << std::endl;


    deleteHuffmanTree(root);
    return compressedData;
}