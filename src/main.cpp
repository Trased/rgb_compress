#include <utils.h>

int main(int argc, char *argv[])
{
    if (1 >= argc)
    {
        std::cout << "There should be at least one argument!" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    CommandUsed usedCommand = findCommand(command);

    if (CommandUsed::HELP == usedCommand)
    {
        printHelp();
    }
    else if(CommandUsed::COMPRESS == usedCommand)
    {
        if (5 != argc)
        {
            std::cerr << "Usage: -c [quality 1-100] [input path] [output path]" <<std::endl;
            return 1;
        }
        int quality = 0;
        try
        {
            quality = std::stoi(argv[2]);
        }
        catch (...)
        {
            std::cerr << "Invalid quality parameter. It must be an integer!" << std::endl;
            return 1;
        }

        if (1 > quality || 100 < quality)
        {
            std::cerr << "Quality must be between 1 and 100." << std::endl;
            return 1;
        }

        std::string inputFile = argv[3];
        std::string outputFile = argv[4];

        fs::path inputPath(inputFile);
        fs::path outputPath(outputFile);

        if (!inputPath.is_absolute())
        {
            std::cerr << "You need to use absolute path!" << std::endl;
            return 1;
        }
        if (!fs::exists(inputPath))
        {
            std::cerr << "Input file does not exist: " << inputPath << std::endl;
            return 1;
        }
        if (".rgb" != inputPath.extension())
        {
            std::cerr << "Input file must have .rgb extension." << std::endl;
            return 1;
        }

        if (!outputPath.is_absolute())
        {
            std::cerr << "Output file path must be absolute." << std::endl;
            return 1;
        }
        if (".rgb" != outputPath.extension())
        {
            std::cerr << "Output file must have .rgb extension." << std::endl;
            return 1;
        }

        std::cout << "Compressing..." << std::endl
                << "Quality: " << quality << std::endl
                << "Input: " << inputFile << std::endl
                << "Output: " << outputFile << "\n";

        compress(inputPath, outputPath, quality);
    }
    else if (CommandUsed::DECOMPRESS == usedCommand)
    {
        if (argc != 4)
        {
            std::cerr << "Usage: -u [input path] [output path]" << std::endl;
            return 1;
        }

        std::string inputFile = argv[2];
        std::string outputFile = argv[3];

        fs::path inputPath(inputFile);
        fs::path outputPath(outputFile);

        if (!inputPath.is_absolute())
        {
            std::cerr << "You need to use absolute path!" << std::endl;
            return 1;
        }

        if(".rgb" != inputPath.extension())
        {
            std::cerr << "Input file must have .rgb extension." << std::endl;
            return 1;
        }

        if (!fs::exists(inputPath))
        {
            std::cerr << "Input file does not exist: " << inputPath << std::endl;
            return 1;
        }

        if (!outputPath.is_absolute())
        {
            std::cerr << "Output file path must be absolute." << std::endl;
            return 1;
        }

        if (outputPath.extension() != ".rgb")
        {
            std::cerr << "Output file must have .rgb extension.\n";
            return 1;
        }

        std::cout << "Decompressing...\n";
        std::cout << "Input file: " << inputPath << "\n";
        std::cout << "Output file: " << outputPath << "\n";

        // TODO: Call decompression logic here
    }
    else
    {
        std::cout << "Invalid command received! Please try again!" << std::endl;
    }

    return 0;
}

