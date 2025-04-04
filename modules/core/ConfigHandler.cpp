#include "ConfigHandler.h"
#include <fstream>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#include <filesystem>
#endif

#ifdef _WIN32
// Checks for directory existence on Windows
static bool directoryExists(const std::string& dirName)
{
    struct _stat info;
    if (_stat(dirName.c_str(), &info) != 0)
        return false;
    return (info.st_mode & _S_IFDIR) != 0;
}
#endif

ConfigHandler::ConfigHandler(const std::string& configFileName)
{
    try
    {
        std::filesystem::path exePath = std::filesystem::current_path();
        
        std::filesystem::path configDir = exePath / "Bloodpoints Calculator";

        // create Folder if Folder does not exist
        if (!std::filesystem::exists(configDir))
        {
            std::filesystem::create_directories(configDir);
        }

        path = (configDir / configFileName).string();

        // default json if json des not exist
        std::ifstream testFile(path);
        if (!testFile.good())
        {
            configData = nlohmann::json::object();
            save();
        }
    }
    catch (const std::exception& e)
    {
        // Fallback
        path = configFileName;
        configData = nlohmann::json::object();
        save();
    }
}


bool ConfigHandler::load()
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    try
    {
        file >> configData;
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool ConfigHandler::save()
{
    std::ofstream file(std::filesystem::absolute(path));
    if (!file.is_open())
        return false;

    try
    {
        file << configData.dump(4);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

nlohmann::json& ConfigHandler::getConfig()
{
    return configData;
}

const nlohmann::json& ConfigHandler::getConfig() const
{
    return configData;
}

void ConfigHandler::setConfig(const nlohmann::json& config)
{
    configData = config;
}
