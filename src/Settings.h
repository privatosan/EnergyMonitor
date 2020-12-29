#ifndef SETTINGS_H
#define SETTINGS_H

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

class Settings
{
public:
    static Settings &getInstance();

    const nlohmann::json::const_reference get(const std::string &key) const; 

private:
    // this is a singleton, hide copy constructor etc.
    Settings(const Settings&);
    Settings& operator=(const Settings&);
    // a singleton constructor/destructor is not thread safe and need to be empty
    Settings() { };
    ~Settings() { };

    void initialize();

private:
    nlohmann::json m_json;
};

#endif // SETTINGS_H
