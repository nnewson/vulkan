#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace fire_engine
{

struct ApplicationArgs
{
    std::string_view scenePath{};
    std::string_view skyboxPath{};
};

[[nodiscard]] inline bool isEnvironmentPath(std::string_view path)
{
    const auto dot = path.find_last_of('.');
    if (dot == std::string_view::npos)
    {
        return false;
    }

    std::string extension(path.substr(dot));
    std::ranges::transform(extension, extension.begin(),
                           [](unsigned char c)
                           {
                               return static_cast<char>(std::tolower(c));
                           });
    return extension == ".hdr" || extension == ".exr";
}

[[nodiscard]] inline ApplicationArgs parseApplicationArgs(int argc, char* argv[]) noexcept
{
    ApplicationArgs args;

    if (argc >= 2)
    {
        std::string_view first = argv[1];
        if (isEnvironmentPath(first))
        {
            args.skyboxPath = first;
        }
        else
        {
            args.scenePath = first;
        }
    }

    if (argc >= 3)
    {
        args.skyboxPath = argv[2];
    }

    return args;
}

} // namespace fire_engine
