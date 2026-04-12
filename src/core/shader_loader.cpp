#include <fire_engine/core/shader_loader.hpp>

#include <fstream>
#include <stdexcept>

namespace fire_engine
{

std::vector<char> ShaderLoader::load_from_file(const std::string& path)
{
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f.is_open())
    {
        throw std::runtime_error("failed to open file: " + path);
    }
    size_t sz = static_cast<size_t>(f.tellg());
    std::vector<char> buf(sz);
    f.seekg(0);
    f.read(buf.data(), static_cast<std::streamsize>(sz));
    return buf;
}

} // namespace fire_engine
