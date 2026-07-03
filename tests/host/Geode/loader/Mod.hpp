#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace geode {
    class VersionInfo {
    public:
        explicit VersionInfo(std::string v = "v0.0.0") : m_v(std::move(v)) {}

        std::string toVString(bool = true) const {
            return m_v;
        }

    private:
        std::string m_v;
    };

    class Mod {
    public:
        explicit Mod(std::filesystem::path resourcesDir) :
            m_resourcesDir(std::move(resourcesDir)), m_version("v0.1.0-beta.8") {}

        explicit Mod(
            std::filesystem::path resourcesDir, std::string id, std::string version = "v0.0.0"
        ) :
            m_resourcesDir(std::move(resourcesDir)), m_id(std::move(id)),
            m_version(std::move(version)) {}

        std::string const& getID() const {
            return m_id;
        }

        VersionInfo const& getVersion() const {
            return m_version;
        }

        std::filesystem::path const& getResourcesDir() const {
            return m_resourcesDir;
        }

        std::filesystem::path getSaveDir() const {
            return m_resourcesDir / "save";
        }

        std::filesystem::path getConfigDir() const {
            return m_resourcesDir / "config";
        }

        std::filesystem::path getPersistentDir() const {
            return m_resourcesDir / "persistent";
        }

        static Mod* get() {
            return s_fallbackMod;
        }

        static void setFallbackMod(Mod* mod) {
            s_fallbackMod = mod;
        }

        static Mod* create(std::filesystem::path resourcesDir) {
            auto* mod = new Mod(std::move(resourcesDir));
            s_allMods.push_back(mod);
            return mod;
        }

        static Mod* create(
            std::filesystem::path resourcesDir, std::string id, std::string version = "v0.0.0"
        ) {
            auto* mod = new Mod(std::move(resourcesDir), std::move(id), std::move(version));
            s_allMods.push_back(mod);
            return mod;
        }

        static void destroy(Mod* mod) {
            auto it = std::find(s_allMods.begin(), s_allMods.end(), mod);
            if (it != s_allMods.end()) {
                s_allMods.erase(it);
            }
            if (s_fallbackMod == mod) {
                s_fallbackMod = nullptr;
            }
            delete mod;
        }

        static void resetForTests() {
            for (auto* mod : s_allMods) {
                delete mod;
            }
            s_allMods.clear();
            s_fallbackMod = nullptr;
        }

        static std::vector<Mod*> const& allMods() {
            return s_allMods;
        }

    private:
        std::filesystem::path m_resourcesDir;
        std::string m_id;
        VersionInfo m_version;
        inline static std::vector<Mod*> s_allMods{};
        inline static Mod* s_fallbackMod = nullptr;
    };
} // namespace geode
