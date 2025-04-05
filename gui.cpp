// gui.cpp

#include <iostream>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include <tchar.h>
#include <thread>
#include <unordered_set>
#include <filesystem>
#include <future>
#include <iomanip>

#include "gui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
#include "modules/core/Logger.h"
#include "Globals.h"
#include "modules/settings/Theme.h"
#include "modules/settings/ThemeManager.h"
#include "modules/utilities/PopupHandler.h"
#include "modules/core/Logger.h"
#include "modules/utilities/getFolder.h"
#include "modules/DbD/DbDBloodpoints.h"
#include "modules/DbD/DbDStrategies.h"
#include "modules/DbD/DbDOfferings.h"

static std::vector<int> survivorStatus(DbDBloodpoints::SurvivorObjectives.size(), 0);

// Total Bloodpoints amount
static float totalReached = 0.0f;

// Buffer for Progressbars
char survivorProgressTotalBuffer[32];
char survivorProgressObjectivesBuffer[32];
char survivorProgressAltruismBuffer[32];
char survivorProgressBoldnessBuffer[32];
char survivorProgressSurvivalBuffer[32];

// Progressbars
static float survivorProgressTotal = 0.0f;
static float survivorProgressObjectives = 0.0f;
static float survivorProgressSurvival = 0.0f;
static float survivorProgressAlturism = 0.0f;
static float survivorProgressBoldness = 0.0f;

// Load and Save Strategies
static char saveNameBuffer[128] = "";
static int selectedStrategyIndex = -1;
static std::vector<std::string> availableStrategies;

// Offering Counter
static int totalOfferingCount = 0;
static const int maxTotalOfferings = 5;
static float totalMultiplier = 0.0f;

// Updatebool for Progressbar
bool updated = false;

namespace fs = std::filesystem;

void ScanStrategies()
{
    availableStrategies.clear();
    const std::string folderPath = "Bloodpoints Calculator/Strategies/";

    if (!fs::exists(folderPath))
        fs::create_directories(folderPath);

    for (const auto &entry : fs::directory_iterator(folderPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            availableStrategies.push_back(entry.path().stem().string());
        }
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wideParameter,
    LPARAM longParameter);

LRESULT __stdcall WindowProcess(
    HWND window,
    UINT message,
    WPARAM wideParameter,
    LPARAM longParameter)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
        return true;

    switch (message)
    {

    case WM_SIZE:
    {
        if (gui::device && wideParameter != SIZE_MINIMIZED)
        {
            gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
            gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
            gui::ResetDevice();
        }
    }
        return 0;

    case WM_SYSCOMMAND:
    {
        if ((wideParameter & 0xfff0) == SC_KEYMENU)
            return 0;
    }
    break;

    case WM_DESTROY:
    {
        PostQuitMessage(0);
    }
        return 0;

    case WM_LBUTTONDOWN:
    {
        gui::position = MAKEPOINTS(longParameter);
    }

    case WM_MOUSEMOVE:
    {
        if (wideParameter == MK_LBUTTON)
        {
            const auto points = MAKEPOINTS(longParameter);
            auto rect = ::RECT{};

            GetWindowRect(gui::window, &rect);

            rect.left += points.x - gui::position.x;
            rect.top += points.y - gui::position.y;

            if (gui::position.x >= 0 &&
                gui::position.x <= gui::WIDTH &&
                gui::position.y >= 0 && gui::position.y <= 19)
            {
                SetWindowPos(
                    gui::window,
                    HWND_TOPMOST,
                    rect.left,
                    rect.top,
                    0, 0,
                    SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
            }
        }
        return 0;
    }
    }

    return DefWindowProcW(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(
    const wchar_t *windowName,
    const wchar_t *className) noexcept
{
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_CLASSDC;
    windowClass.lpfnWndProc = WindowProcess;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandleW(NULL);
    windowClass.hIcon = 0;
    windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = className;
    windowClass.hIconSm = 0;

    if (!RegisterClassExW(&windowClass))
    {
        MessageBoxW(NULL, L"Fensterklasse konnte nicht registriert werden.", L"Fehler", MB_ICONERROR);
        return;
    }

    window = CreateWindowW(
        className,
        windowName,
        WS_POPUP,
        100,
        100,
        WIDTH,
        HEIGHT,
        NULL,
        NULL,
        windowClass.hInstance,
        NULL);

    if (!window)
    {
        MessageBoxW(NULL, L"Fenster konnte nicht erstellt werden.", L"Fehler", MB_ICONERROR);
        return;
    }

    ShowWindow(window, SW_SHOWDEFAULT);
    UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
    if (window)
    {
        DestroyWindow(window);
        window = nullptr;
    }
    UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);

    if (!d3d)
        return false;

    ZeroMemory(&presentParameters, sizeof(presentParameters));

    presentParameters.Windowed = TRUE;
    presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
    presentParameters.EnableAutoDepthStencil = TRUE;
    presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
    presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (d3d->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &presentParameters,
            &device) < 0)
        return false;

    return true;
}

void gui::ResetDevice() noexcept
{
    ImGui_ImplDX9_InvalidateDeviceObjects();

    const auto result = device->Reset(&presentParameters);

    if (result == D3DERR_INVALIDCALL)
        IM_ASSERT(0);

    ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
    if (device)
    {
        device->Release();
        device = nullptr;
    }

    if (d3d)
    {
        d3d->Release();
        d3d = nullptr;
    }
}

void gui::CreateImGui() noexcept
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    // Stil und Farbschema anpassen
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 8.f; // Eckenrundung für Fenster
    style.FrameRounding = 8.f;  // Eckenrundung für Buttons, Eingabefelder etc.
    style.TabRounding = 8.f;
    style.GrabRounding = 8.f;
    style.PopupRounding = 8.f;
    style.ItemSpacing = ImVec2(10, 5); // Abstand zwischen Elementen

    // Farbschema anpassen
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);       // Hintergrundfarbe von Fenstern
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.25f, 0.3f, 1.0f);        // Button-Hintergrund
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.35f, 0.4f, 1.0f); // Button-Hintergrund beim Hovern
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.2f, 0.25f, 1.0f); // Button-Hintergrund beim Klicken

    // ImGui für die Verwendung mit DirectX und Win32 initialisieren
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(device);

    io.IniFilename = NULL;
}

void gui::DestroyImGui() noexcept
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    // Start the Dear ImGui frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
    ImGui::EndFrame();

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

    if (device->BeginScene() >= 0)
    {
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();
    }

    const auto result = device->Present(0, 0, 0, 0);

    // Handle loss of D3D9 device
    if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        ResetDevice();
}

void gui::Render() noexcept
{
    RECT clientRect;
    GetClientRect(window, &clientRect);
    float currentWidth = static_cast<float>(clientRect.right - clientRect.left);
    float currentHeight = static_cast<float>(clientRect.bottom - clientRect.top);

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({currentWidth, currentHeight});
    ImGui::Begin("Bloodpoints Calculator", &exit, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    // Update Buffer for Progressbars from Survivor
    snprintf(survivorProgressTotalBuffer, sizeof(survivorProgressTotalBuffer), "%.0f%%", survivorProgressTotal * 100.0f);
    snprintf(survivorProgressObjectivesBuffer, sizeof(survivorProgressObjectivesBuffer), "%.0f%%", survivorProgressObjectives * 100.0f);
    snprintf(survivorProgressAltruismBuffer, sizeof(survivorProgressAltruismBuffer), "%.0f%%", survivorProgressAlturism * 100.0f);
    snprintf(survivorProgressBoldnessBuffer, sizeof(survivorProgressBoldnessBuffer), "%.0f%%", survivorProgressBoldness * 100.0f);
    snprintf(survivorProgressSurvivalBuffer, sizeof(survivorProgressSurvivalBuffer), "%.0f%%", survivorProgressSurvival * 100.0f);

    if (ImGui::BeginTabBar("MainTabBar"))
    {
        // General Menu Information
        if (ImGui::BeginTabItem("Menu"))
        {
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Survivor"))
        {
            ImGui::Text("Survivor Progress:");

            // Neue Gesamte-Progressbar
            ImGui::ProgressBar(survivorProgressTotal, ImVec2(0.f, 0.f), survivorProgressTotalBuffer);

            // ========== OBJECTIVES ==========
            if (ImGui::CollapsingHeader("Objectives"))
            {
                ImGui::ProgressBar(survivorProgressObjectives, ImVec2(0.f, 0.f), survivorProgressObjectivesBuffer);
                ImGui::Separator();

                for (size_t i = 0; i < DbDBloodpoints::SurvivorObjectives.size(); ++i)
                {
                    const auto &objective = DbDBloodpoints::SurvivorObjectives[i];

                    if (objective.category == "Objectives")
                    {
                        ImGui::PushID(static_cast<int>(i)); // WICHTIG: für eindeutige IDs pro Zeile

                        if (objective.noCap)
                        {
                            // --- NoCap: InputInt
                            updated |= ImGui::InputInt(objective.name.c_str(), &survivorStatus[i], 1, 5);
                            if (survivorStatus[i] < 0)
                                survivorStatus[i] = 0; // Kein negativer Input erlaubt
                        }
                        else
                        {
                            // --- Normale Checkbox
                            bool checked = (survivorStatus[i] > 0);
                            if (ImGui::Checkbox(objective.name.c_str(), &checked))
                            {
                                survivorStatus[i] = checked ? 1 : 0;
                                updated = true;
                            }
                        }

                        // --- Tooltip beim Hover
                        if (ImGui::IsItemHovered() && !objective.tooltip.empty())
                            ImGui::SetTooltip("%s", objective.tooltip.c_str());

                        ImGui::PopID();
                    }
                }
            }

            // ========== SURVIVAL ==========
            if (ImGui::CollapsingHeader("Survival"))
            {
                ImGui::ProgressBar(survivorProgressSurvival, ImVec2(0.f, 0.f), survivorProgressSurvivalBuffer);
                ImGui::Separator();

                for (size_t i = 0; i < DbDBloodpoints::SurvivorObjectives.size(); ++i)
                {
                    const auto &objective = DbDBloodpoints::SurvivorObjectives[i];

                    if (objective.category == "Survival")
                    {
                        ImGui::PushID(static_cast<int>(i));

                        if (!objective.noCap)
                        {
                            // --- NoCap: InputInt
                            updated |= ImGui::InputInt(objective.name.c_str(), &survivorStatus[i], 1, 5);
                            if (survivorStatus[i] < 0)
                                survivorStatus[i] = 0;
                        }
                        else
                        {
                            // --- Normale Checkbox
                            bool checked = (survivorStatus[i] > 0);
                            if (ImGui::Checkbox(objective.name.c_str(), &checked))
                            {
                                survivorStatus[i] = checked ? 1 : 0;
                                updated = true;
                            }
                        }

                        // --- Tooltip beim Hover
                        if (ImGui::IsItemHovered() && !objective.tooltip.empty())
                            ImGui::SetTooltip("%s", objective.tooltip.c_str());

                        ImGui::PopID();
                    }
                }
            }

            // ========== ALTRUISM ==========
            if (ImGui::CollapsingHeader("Altruism"))
            {
                ImGui::ProgressBar(survivorProgressAlturism, ImVec2(0.f, 0.f), survivorProgressAltruismBuffer);
                ImGui::Separator();

                for (size_t i = 0; i < DbDBloodpoints::SurvivorObjectives.size(); ++i)
                {
                    const auto &objective = DbDBloodpoints::SurvivorObjectives[i];

                    if (objective.category == "Altruism")
                    {
                        ImGui::PushID(static_cast<int>(i));

                        if (objective.noCap)
                        {
                            updated |= ImGui::InputInt(objective.name.c_str(), &survivorStatus[i], 1, 5);
                            if (survivorStatus[i] < 0)
                                survivorStatus[i] = 0;
                        }
                        else
                        {
                            bool checked = (survivorStatus[i] > 0);
                            if (ImGui::Checkbox(objective.name.c_str(), &checked))
                            {
                                survivorStatus[i] = checked ? 1 : 0;
                                updated = true;
                            }
                        }

                        if (ImGui::IsItemHovered() && !objective.tooltip.empty())
                            ImGui::SetTooltip("%s", objective.tooltip.c_str());

                        ImGui::PopID();
                    }
                }
            }

            // ========== BOLDNESS ==========
            if (ImGui::CollapsingHeader("Boldness"))
            {
                ImGui::ProgressBar(survivorProgressBoldness, ImVec2(0.f, 0.f), survivorProgressBoldnessBuffer);
                ImGui::Separator();

                for (size_t i = 0; i < DbDBloodpoints::SurvivorObjectives.size(); ++i)
                {
                    const auto &objective = DbDBloodpoints::SurvivorObjectives[i];

                    if (objective.category == "Boldness")
                    {
                        ImGui::PushID(static_cast<int>(i));

                        if (objective.noCap)
                        {
                            updated |= ImGui::InputInt(objective.name.c_str(), &survivorStatus[i], 1, 5);
                            if (survivorStatus[i] < 0)
                                survivorStatus[i] = 0;
                        }
                        else
                        {
                            bool checked = (survivorStatus[i] > 0);
                            if (ImGui::Checkbox(objective.name.c_str(), &checked))
                            {
                                survivorStatus[i] = checked ? 1 : 0;
                                updated = true;
                            }
                        }

                        if (ImGui::IsItemHovered() && !objective.tooltip.empty())
                            ImGui::SetTooltip("%s", objective.tooltip.c_str());

                        ImGui::PopID();
                    }
                }
            }

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Strategies"))
            {

                for (size_t i = 0; i < DbDStrategies::strategies.size(); ++i)
                {
                    if (ImGui::Checkbox(DbDStrategies::strategies[i].name.c_str(), &DbDStrategies::strategies[i].active))
                    {
                        updated = true;
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", DbDStrategies::strategies[i].tooltip.c_str());
                    }
                }
            }

            // === Offerings Section ===
            if (ImGui::CollapsingHeader("Offerings"))
            {
                static std::vector<int> offeringCounts(DbDOfferings::offerings.size(), 0);
                static const int maxOfferings = 5;
                bool offeringChanged = false;

                // --- Vor dem Loop: Anzahl aktuell gesetzter Offerings zählen ---
                int currentTotalOfferings = 0;
                for (int count : offeringCounts)
                    currentTotalOfferings += count;

                for (size_t i = 0; i < DbDOfferings::offerings.size(); ++i)
                {
                    ImGui::PushID(static_cast<int>(i));

                    int previousCount = offeringCounts[i];
                    offeringChanged |= ImGui::InputInt(DbDOfferings::offerings[i].name.c_str(), &offeringCounts[i]);

                    // Check for negative value
                    if (offeringCounts[i] < 0)
                        offeringCounts[i] = 0;

                    // Check for exceeding max offerings
                    currentTotalOfferings = 0;
                    for (int count : offeringCounts)
                        currentTotalOfferings += count;

                    if (currentTotalOfferings > maxOfferings)
                    {
                        offeringCounts[i] = previousCount;
                    }

                    // Tooltip
                    if (ImGui::IsItemHovered() && !DbDOfferings::offerings[i].tooltip.empty())
                        ImGui::SetTooltip("%s", DbDOfferings::offerings[i].tooltip.c_str());

                    ImGui::PopID();
                }

                ImGui::Separator();

                if (offeringChanged)
                {
                    totalMultiplier = 0.0f;

                    for (size_t i = 0; i < DbDOfferings::offerings.size(); ++i)
                    {
                        if (offeringCounts[i] > 0)
                        {
                            totalMultiplier += DbDOfferings::offerings[i].multiplier * offeringCounts[i];
                        }
                    }
                }

                // Text aktualisieren
                static char offeringMultiplierBuffer[32];
                snprintf(offeringMultiplierBuffer, sizeof(offeringMultiplierBuffer), "Multiplier: x%.2f", totalMultiplier);
                ImGui::Text("%s", offeringMultiplierBuffer);
            }

            if (totalMultiplier < 0.1f)
            {
                ImGui::Text("Total Bloodpoints: %.0f", totalReached);
            }
            else
            {
                ImGui::Text("Total Bloodpoints: %.0f", totalReached * totalMultiplier);
            }

            ImGui::Text("");
            ImGui::Separator();
            ImGui::Text("");

            if (ImGui::CollapsingHeader("Save & Load"))
            {

                ScanStrategies();

                // Eingabefeld für neuen Strategie-Namen
                ImGui::InputText("Strategy Name", saveNameBuffer, sizeof(saveNameBuffer));

                if (ImGui::Button("Save Strategy"))
                {
                    if (strlen(saveNameBuffer) > 0)
                    {
                        const std::string folderPath = "Strategies/";

                        // Ordner sicherstellen
                        const std::filesystem::path fullFolderPath = std::filesystem::current_path() / "Bloodpoints Calculator" / folderPath;

                        if (!std::filesystem::exists(fullFolderPath))
                        {
                            try
                            {
                                std::filesystem::create_directories(fullFolderPath);
                            }
                            catch (const std::exception &ex)
                            {
                                Logger::instance().log(LogLevel::LOG_ERROR, LogCategory::LOG_CONFIG,
                                                       std::string("Failed to create strategies folder: ") + ex.what(),
                                                       __FUNCTION__, __FILE__, __LINE__);
                                return;
                            }
                        }

                        std::string relativeFilename = folderPath + std::string(saveNameBuffer) + ".json";

                        ConfigHandler config(relativeFilename);
                        config.getConfig()["survivorStatus"] = survivorStatus;

                        if (config.save())
                        {
                            Logger::instance().log(LogLevel::LOG_INFO, LogCategory::LOG_CONFIG,
                                                   "Strategy saved successfully: " + relativeFilename,
                                                   __FUNCTION__, __FILE__, __LINE__);
                        }
                        else
                        {
                            Logger::instance().log(LogLevel::LOG_ERROR, LogCategory::LOG_CONFIG,
                                                   "Failed to save strategy: " + relativeFilename,
                                                   __FUNCTION__, __FILE__, __LINE__);
                        }
                    }
                }

                ImGui::Separator();

                // Dropdown für gespeicherte Strategien
                if (availableStrategies.empty())
                {
                    ImGui::Text("No saved strategies found.");
                }
                else
                {
                    if (ImGui::BeginCombo("Load Strategy", (selectedStrategyIndex >= 0) ? availableStrategies[selectedStrategyIndex].c_str() : "Select..."))
                    {
                        for (int i = 0; i < availableStrategies.size(); ++i)
                        {
                            bool isSelected = (selectedStrategyIndex == i);
                            if (ImGui::Selectable(availableStrategies[i].c_str(), isSelected))
                            {
                                selectedStrategyIndex = i;
                            }
                        }
                        ImGui::EndCombo();
                    }

                    if (selectedStrategyIndex >= 0)
                    {
                        if (ImGui::Button("Load Selected Strategy"))
                        {
                            std::string filename = "Bloodpoints Calculator/Strategies/" + availableStrategies[selectedStrategyIndex] + ".json";

                            ConfigHandler config(filename);
                            if (config.load())
                            {
                                if (config.getConfig().contains("survivorStatus"))
                                {
                                    survivorStatus = config.getConfig()["survivorStatus"].get<std::vector<int>>();
                                }
                            }
                        }

                        ImGui::SameLine(); // <--- sorgt dafür, dass Delete direkt daneben ist!

                        if (ImGui::Button("Delete Selected Strategy"))
                        {
                            std::string filename = "Bloodpoints Calculator/Strategies/" + availableStrategies[selectedStrategyIndex] + ".json";

                            // Existenz prüfen und Datei löschen
                            try
                            {
                                if (std::filesystem::exists(filename))
                                {
                                    std::filesystem::remove(filename);

                                    Logger::instance().log(LogLevel::LOG_INFO, LogCategory::LOG_CONFIG,
                                                           "Strategy deleted successfully: " + filename,
                                                           __FUNCTION__, __FILE__, __LINE__);

                                    ScanStrategies();           // Liste neu laden
                                    selectedStrategyIndex = -1; // Auswahl zurücksetzen
                                }
                                else
                                {
                                    Logger::instance().log(LogLevel::LOG_WARNING, LogCategory::LOG_CONFIG,
                                                           "Strategy file not found for deletion: " + filename,
                                                           __FUNCTION__, __FILE__, __LINE__);
                                }
                            }
                            catch (const std::exception &ex)
                            {
                                Logger::instance().log(LogLevel::LOG_ERROR, LogCategory::LOG_CONFIG,
                                                       std::string("Failed to delete strategy: ") + ex.what(),
                                                       __FUNCTION__, __FILE__, __LINE__);
                            }
                        }
                    }
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Killer"))
        {

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings"))
        {
            if (ImGui::CollapsingHeader("Templates"))
            {
                static std::vector<std::string> allThemes;
                static int selectedThemeIndex = -1;
                static char newThemeName[32] = "";

                // Hilfsfunktion zum Refresh der Theme-Liste
                auto RefreshThemeList = []()
                {
                    allThemes = gThemeManager->getAllThemeNames();
                };

                // Initial-Refresh (nur beim ersten Mal)
                static bool firstRun = true;
                if (firstRun)
                {
                    RefreshThemeList();

                    std::string currentTheme = gThemeManager->getCurrentThemeName();
                    auto it = std::find(allThemes.begin(), allThemes.end(), currentTheme);
                    if (it != allThemes.end())
                        selectedThemeIndex = static_cast<int>(std::distance(allThemes.begin(), it));
                    else
                        selectedThemeIndex = 0; // Fallback, wenn nicht gefunden

                    firstRun = false;
                }

                // Theme-Auswahl per Combo
                if (!allThemes.empty())
                {
                    if (ImGui::Combo("Theme", &selectedThemeIndex, [](void *data, int idx, const char **out_text)
                                     {
                         const auto &names = *static_cast<std::vector<std::string> *>(data);
                         if (idx < 0 || idx >= static_cast<int>(names.size()))
                             return false;
                         *out_text = names[idx].c_str();
                         return true; }, static_cast<void *>(&allThemes), static_cast<int>(allThemes.size())))
                    {
                        gThemeManager->setCurrentTheme(allThemes[selectedThemeIndex]);
                    }
                }

                // Neuer Theme-Name
                ImGui::InputText("New Theme Name", newThemeName, sizeof(newThemeName));

                // Theme speichern
                if (ImGui::Button("Save Theme", ImVec2(100, 0)))
                {
                    if (strlen(newThemeName) > 0)
                    {
                        gThemeManager->saveCurrentThemeAs(newThemeName);
                        RefreshThemeList();

                        // neuen Index setzen
                        selectedThemeIndex = static_cast<int>(allThemes.size()) - 1;
                        memset(newThemeName, 0, sizeof(newThemeName));
                    }
                }

                // Theme löschen (nur wenn kein Standard-Theme)
                bool isDefaultThemeSelected = (selectedThemeIndex >= 4);
                if (isDefaultThemeSelected)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Theme", ImVec2(100, 0)))
                    {
                        gThemeManager->deleteTheme(allThemes[selectedThemeIndex]);
                        RefreshThemeList();
                        selectedThemeIndex = (std::min)(1, static_cast<int>(allThemes.size()) - 1);
                    }
                }

                ImGuiStyle &style = ImGui::GetStyle();

                // --- Individuelle Farb-Anpassungen ---
                if (ImGui::TreeNode("Window"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_WindowBg];
                    if (ImGui::ColorEdit4("WindowBg", (float *)&color))
                    {
                        style.Colors[ImGuiCol_WindowBg] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_WindowBg", color);
                    }

                    color = style.Colors[ImGuiCol_ChildBg];
                    if (ImGui::ColorEdit4("ChildBg", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ChildBg] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ChildBg", color);
                    }

                    color = style.Colors[ImGuiCol_PopupBg];
                    if (ImGui::ColorEdit4("PopupBg", (float *)&color))
                    {
                        style.Colors[ImGuiCol_PopupBg] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_PopupBg", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Title"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_Border];
                    if (ImGui::ColorEdit4("Border", (float *)&color))
                    {
                        style.Colors[ImGuiCol_Border] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_Border", color);
                    }

                    color = style.Colors[ImGuiCol_TitleBg];
                    if (ImGui::ColorEdit4("TitleBg", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TitleBg] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TitleBg", color);
                    }

                    color = style.Colors[ImGuiCol_TitleBgActive];
                    if (ImGui::ColorEdit4("TitleBgActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TitleBgActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TitleBgActive", color);
                    }

                    color = style.Colors[ImGuiCol_TitleBgCollapsed];
                    if (ImGui::ColorEdit4("TitleBgCollapsed", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TitleBgCollapsed] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TitleBgCollapsed", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Header"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_Header];
                    if (ImGui::ColorEdit4("Header", (float *)&color))
                    {
                        style.Colors[ImGuiCol_Header] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_Header", color);
                    }

                    color = style.Colors[ImGuiCol_HeaderHovered];
                    if (ImGui::ColorEdit4("HeaderHovered", (float *)&color))
                    {
                        style.Colors[ImGuiCol_HeaderHovered] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_HeaderHovered", color);
                    }

                    color = style.Colors[ImGuiCol_HeaderActive];
                    if (ImGui::ColorEdit4("HeaderActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_HeaderActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_HeaderActive", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Buttons"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_Button];
                    if (ImGui::ColorEdit4("Button", (float *)&color))
                    {
                        style.Colors[ImGuiCol_Button] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_Button", color);
                    }

                    color = style.Colors[ImGuiCol_ButtonHovered];
                    if (ImGui::ColorEdit4("ButtonHovered", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ButtonHovered] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ButtonHovered", color);
                    }

                    color = style.Colors[ImGuiCol_ButtonActive];
                    if (ImGui::ColorEdit4("ButtonActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ButtonActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ButtonActive", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Frames"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_FrameBg];
                    if (ImGui::ColorEdit4("FrameBg", (float *)&color))
                    {
                        style.Colors[ImGuiCol_FrameBg] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_FrameBg", color);
                    }

                    color = style.Colors[ImGuiCol_FrameBgHovered];
                    if (ImGui::ColorEdit4("FrameBgHovered", (float *)&color))
                    {
                        style.Colors[ImGuiCol_FrameBgHovered] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_FrameBgHovered", color);
                    }

                    color = style.Colors[ImGuiCol_FrameBgActive];
                    if (ImGui::ColorEdit4("FrameBgActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_FrameBgActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_FrameBgActive", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Tabs"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_Tab];
                    if (ImGui::ColorEdit4("Tab", (float *)&color))
                    {
                        style.Colors[ImGuiCol_Tab] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_Tab", color);
                    }

                    color = style.Colors[ImGuiCol_TabHovered];
                    if (ImGui::ColorEdit4("TabHovered", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TabHovered] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TabHovered", color);
                    }

                    color = style.Colors[ImGuiCol_TabActive];
                    if (ImGui::ColorEdit4("TabActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TabActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TabActive", color);
                    }

                    color = style.Colors[ImGuiCol_TabUnfocused];
                    if (ImGui::ColorEdit4("TabUnfocused", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TabUnfocused] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TabUnfocused", color);
                    }

                    color = style.Colors[ImGuiCol_TabUnfocusedActive];
                    if (ImGui::ColorEdit4("TabUnfocusedActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TabUnfocusedActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TabUnfocusedActive", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Slider"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_SliderGrab];
                    if (ImGui::ColorEdit4("SliderGrab", (float *)&color))
                    {
                        style.Colors[ImGuiCol_SliderGrab] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_SliderGrab", color);
                    }

                    color = style.Colors[ImGuiCol_SliderGrabActive];
                    if (ImGui::ColorEdit4("SliderGrabActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_SliderGrabActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_SliderGrabActive", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Scrollbar"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_ScrollbarBg];
                    if (ImGui::ColorEdit4("ScrollbarBg", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ScrollbarBg] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ScrollbarBg", color);
                    }

                    color = style.Colors[ImGuiCol_ScrollbarGrab];
                    if (ImGui::ColorEdit4("ScrollbarGrab", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ScrollbarGrab] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ScrollbarGrab", color);
                    }

                    color = style.Colors[ImGuiCol_ScrollbarGrabHovered];
                    if (ImGui::ColorEdit4("ScrollbarGrabHovered", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ScrollbarGrabHovered] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ScrollbarGrabHovered", color);
                    }

                    color = style.Colors[ImGuiCol_ScrollbarGrabActive];
                    if (ImGui::ColorEdit4("ScrollbarGrabActive", (float *)&color))
                    {
                        style.Colors[ImGuiCol_ScrollbarGrabActive] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_ScrollbarGrabActive", color);
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Text"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_Text];
                    if (ImGui::ColorEdit4("Text", (float *)&color))
                    {
                        style.Colors[ImGuiCol_Text] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_Text", color);
                    }

                    color = style.Colors[ImGuiCol_TextDisabled];
                    if (ImGui::ColorEdit4("TextDisabled", (float *)&color))
                    {
                        style.Colors[ImGuiCol_TextDisabled] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_TextDisabled", color);
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Progress Bars"))
                {
                    ImVec4 color = style.Colors[ImGuiCol_PlotHistogram];
                    if (ImGui::ColorEdit4("ProgressBar", (float *)&color))
                    {
                        style.Colors[ImGuiCol_PlotHistogram] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_PlotHistogram", color);
                    }

                    color = style.Colors[ImGuiCol_PlotHistogramHovered];
                    if (ImGui::ColorEdit4("ProgressBarHovered", (float *)&color))
                    {
                        style.Colors[ImGuiCol_PlotHistogramHovered] = color;
                        gThemeManager->updateCustomColor("ImGuiCol_PlotHistogramHovered", color);
                    }

                    ImGui::TreePop();
                }
            }

            // Einklappbarer Bereich für die Log-Dateien
            if (ImGui::CollapsingHeader("Logging"))
            {
                static int currentLevel = 0; // 0: DEBUG, 1: INFO, 2: WARNING, 3: ERROR
                const char *levels[] = {"DEBUG", "INFO", "WARNING", "ERROR"};

                if (ImGui::Combo("Log Level", &currentLevel, levels, IM_ARRAYSIZE(levels)))
                {
                    switch (currentLevel)
                    {
                    case 0:
                        Logger::instance().setMinLogLevel(LogLevel::LOG_DEBUG);
                        break;
                    case 1:
                        Logger::instance().setMinLogLevel(LogLevel::LOG_INFO);
                        break;
                    case 2:
                        Logger::instance().setMinLogLevel(LogLevel::LOG_WARNING);
                        break;
                    case 3:
                        Logger::instance().setMinLogLevel(LogLevel::LOG_ERROR);
                        break;
                    }
                }

                // Checkboxen für die einzelnen Log-Kategorien
                static bool enabledGeneral = true, enabledSystem = true, enabledFiles = true, enabledCleaning = true, enabledConfig = true;
                if (ImGui::Checkbox("GENERAL", &enabledGeneral))
                    Logger::instance().enableCategory(LogCategory::LOG_GENERAL, enabledGeneral);
                if (ImGui::Checkbox("SYSTEM", &enabledSystem))
                    Logger::instance().enableCategory(LogCategory::LOG_SYSTEM, enabledSystem);
                if (ImGui::Checkbox("FILES", &enabledFiles))
                    Logger::instance().enableCategory(LogCategory::LOG_FILES, enabledFiles);
                if (ImGui::Checkbox("CLEANING", &enabledCleaning))
                    Logger::instance().enableCategory(LogCategory::LOG_CLEANING, enabledCleaning);
                if (ImGui::Checkbox("CONFIG", &enabledConfig))
                    Logger::instance().enableCategory(LogCategory::LOG_CONFIG, enabledConfig);
            }

            ImGui::EndTabItem();
        }

        // Updating all Progressbars if needed
        if (updated)
        {
            int reachedObjectives = 0;
            int reachedAltruism = 0;
            int reachedBoldness = 0;
            int reachedSurvival = 0;

            // 1x sauber durchgehen
            for (size_t i = 0; i < DbDBloodpoints::SurvivorObjectives.size(); ++i)
            {
                const auto &objective = DbDBloodpoints::SurvivorObjectives[i];
                int value = 0;

                if (objective.noCap)
                    value = survivorStatus[i] * static_cast<int>(objective.bloodpoints);
                else if (survivorStatus[i])
                    value = static_cast<int>(objective.cap);

                if (objective.category == "Objectives")
                    reachedObjectives += value;
                else if (objective.category == "Altruism")
                    reachedAltruism += value;
                else if (objective.category == "Boldness")
                    reachedBoldness += value;
                else if (objective.category == "Survival")
                    reachedSurvival += value;
            }

            // Strategien anwenden (wenn aktiviert)
            for (const auto &strat : DbDStrategies::strategies)
            {
                if (strat.active)
                {
                    reachedObjectives += strat.objectivesBonus;
                    reachedAltruism += strat.altruismBonus;
                    reachedBoldness += strat.boldnessBonus;
                    reachedSurvival += strat.survivalBonus;
                }
            }

            // Maximalpunkte aus deiner Bloodpoints.h
            const float maxPoints = static_cast<float>(DbDBloodpoints::MaxPointsPerCategory);

            survivorProgressObjectives = reachedObjectives / maxPoints;
            survivorProgressAlturism = reachedAltruism / maxPoints;
            survivorProgressBoldness = reachedBoldness / maxPoints;
            survivorProgressSurvival = reachedSurvival / maxPoints;

            int cappedObjectives = reachedObjectives;
            if (cappedObjectives > DbDBloodpoints::MaxPointsPerCategory)
                cappedObjectives = DbDBloodpoints::MaxPointsPerCategory;

            int cappedAltruism = reachedAltruism;
            if (cappedAltruism > DbDBloodpoints::MaxPointsPerCategory)
                cappedAltruism = DbDBloodpoints::MaxPointsPerCategory;

            int cappedBoldness = reachedBoldness;
            if (cappedBoldness > DbDBloodpoints::MaxPointsPerCategory)
                cappedBoldness = DbDBloodpoints::MaxPointsPerCategory;

            int cappedSurvival = reachedSurvival;
            if (cappedSurvival > DbDBloodpoints::MaxPointsPerCategory)
                cappedSurvival = DbDBloodpoints::MaxPointsPerCategory;

            totalReached = static_cast<float>(cappedObjectives + cappedAltruism + cappedBoldness + cappedSurvival);
            survivorProgressTotal = totalReached / (maxPoints * 4.0f);
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
