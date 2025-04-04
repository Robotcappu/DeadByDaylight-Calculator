#pragma once

#include <string>
#include <vector>
#include "modules/core/ConfigHandler.h"
#include "imgui/imgui.h"
#include "json/json.hpp"

// Built-in Themes
enum class BuiltinTheme
{
    Light,
    Dark,
    DbD
};

class ThemeManager
{
public:
    explicit ThemeManager(const std::string& configPath);

    void loadFromConfig();

    void saveToConfig();

    std::string getCurrentThemeName() const;

    void setCurrentTheme(const std::string& themeName);

    std::vector<std::string> getAllThemeNames() const;

    std::vector<std::string> getCustomThemeNames() const;

    void createCustomTheme(const std::string& themeName);

    bool deleteTheme(const std::string& themeName);

    void updateCustomColor(const std::string& colorName, const ImVec4& color);

    void saveCurrentThemeAs(const std::string& themeName);

private:

    void ensureDefaultConfig();
    
    void applyTheme(const std::string& themeName);

    bool isBuiltinTheme(const std::string& themeName) const;

    void applyBuiltinTheme(BuiltinTheme theme);

    void applyCustomTheme(const nlohmann::json& colorData);

    BuiltinTheme builtinThemeFromName(const std::string& name) const;

private:
    ConfigHandler configHandler;
    nlohmann::json config;
    std::string currentThemeName;
};
