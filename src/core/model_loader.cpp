#include <fire_engine/core/model_loader.hpp>

#include <fstream>
#include <stdexcept>

namespace fire_engine
{

void ModelLoader::trim_comment(std::string& line)
{
    const auto pos = line.find('#');
    if (pos != std::string::npos)
    {
        line.erase(pos);
    }
}

void ModelLoader::trim(std::string& s)
{
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        s.clear();
        return;
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    s = s.substr(first, last - first + 1);
}

void ModelLoader::load_from_file(const std::string& path, const LineHandler& handler)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::string line;
    while (std::getline(input, line))
    {
        trim_comment(line);
        trim(line);

        if (line.empty())
        {
            continue;
        }

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        handler(keyword, iss);
    }
}

} // namespace fire_engine
