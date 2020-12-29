#include "Settings.h"

#include <fstream>
#include <mutex>

Settings &Settings::getInstance()
{
    static std::once_flag onceFlag;
    static Settings instance;

    std::call_once(onceFlag, [] {
        instance.initialize();
    });

    return instance;
}

void Settings::initialize()
{
    std::ifstream file("config.json");
    file >> m_json;
}

const nlohmann::json::const_reference Settings::get(const std::string &key) const
{
    return m_json.at(key);
}
