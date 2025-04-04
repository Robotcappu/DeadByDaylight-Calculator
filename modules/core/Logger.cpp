#include "Logger.h"
#include <iostream>


// Singleton-Instanz
Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

#include <filesystem>
#include <windows.h>

Logger::Logger() {
    try {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        // Pfad zur EXE extrahieren
        std::string fullPath(exePath);
        size_t lastSlash = fullPath.find_last_of("\\/");
        std::string exeDir = (lastSlash != std::string::npos) ? fullPath.substr(0, lastSlash) : ".";

        // Neuer Log-Ordner im EXE-Verzeichnis
        std::string logDir = exeDir + "\\Bloodpoints Calculator\\logs\\";
        std::filesystem::create_directories(logDir);

        // Logdatei vorbereiten
        std::string timestamp = getTimestampForFilename();
        std::string fullLogPath = logDir + "log_" + timestamp + ".json";

        // Datei öffnen
        logFile.open(fullLogPath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Logger Fehler: Logdatei konnte nicht geöffnet werden!" << std::endl;
        }

    } catch (const std::exception& ex) {
        std::cerr << "Logger Init-Fehler: " << ex.what() << std::endl;
    }
}


Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::setLogFile(const std::string &filePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filePath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Fehler: Log-Datei '" << filePath << "' konnte nicht geöffnet werden!" << std::endl;
    }
}

void Logger::setMinLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    m_minLogLevel = level;
}

void Logger::enableCategory(LogCategory category, bool enable) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (enable)
        m_enabledCategories.insert(category);
    else
        m_enabledCategories.erase(category);
}

void Logger::log(LogLevel level, LogCategory category, const std::string &message,
                 const std::string &functionName, const std::string &fileName, int line) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Prüfen, ob der Eintrag gemäß den aktuellen Einstellungen geloggt werden soll
    if (level < m_minLogLevel || m_enabledCategories.find(category) == m_enabledCategories.end()) {
        return;
    }
    
    // JSON-formatierter Log-Eintrag
    std::ostringstream oss;
    oss << "{"
        << "\"timestamp\": \"" << getTimestamp() << "\", "
        << "\"level\": \"" << logLevelToString(level) << "\", "
        << "\"category\": \"" << logCategoryToString(category) << "\", "
        << "\"file\": \"" << fileName << "\", "
        << "\"function\": \"" << functionName << "\", "
        << "\"line\": " << line << ", "
        << "\"message\": \"" << message << "\""
        << "}";
    
    logFile << oss.str() << std::endl;
    logFile.flush();
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::LOG_DEBUG:   return "LOG_DEBUG";
        case LogLevel::LOG_INFO:    return "LOG_INFO";
        case LogLevel::LOG_WARNING: return "LOG_WARNING";
        case LogLevel::LOG_ERROR:   return "LOG_ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::logCategoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::LOG_GENERAL:  return "LOG_GENERAL";
        case LogCategory::LOG_SYSTEM:   return "LOG_SYSTEM";
        case LogCategory::LOG_FILES:    return "LOG_FILES";
        case LogCategory::LOG_CLEANING: return "LOG_CLEANING";
        case LogCategory::LOG_CONFIG:   return "LOG_CONFIG";
        default:                    return "UNKNOWN";
    }
}

std::string Logger::getTimestampForFilename() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}
