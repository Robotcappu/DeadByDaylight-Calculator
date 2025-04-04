#include "Theme.h"
#include "ThemeDefinition.h"

void SetTheme(Theme theme)
{
    ImGuiStyle &style = ImGui::GetStyle();
    switch (theme)
    {
    case Theme::Light:
        ImGui::StyleColorsLight();
        break;
    case Theme::Dark:
        ImGui::StyleColorsDark();
        break;
    case Theme::DbD:
    {
        for (size_t i = 0; i < std::size_t(DbDThemeColors); ++i)
        {
            style.Colors[DbDThemeColors[i].index] = DbDThemeColors[i].color;
        }
        break;
    }
    }
}