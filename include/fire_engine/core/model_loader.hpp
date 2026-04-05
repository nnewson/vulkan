#pragma once

#include <functional>
#include <sstream>
#include <string>

namespace fire_engine
{

class ModelLoader
{
public:
    ModelLoader() = delete;

    static void trim_comment(std::string& line);
    static void trim(std::string& s);

    using LineHandler = std::function<void(const std::string& keyword, std::istringstream& iss)>;
    static void load_from_file(const std::string& path, const LineHandler& handler);
};

} // namespace fire_engine
