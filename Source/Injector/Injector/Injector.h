/*
 * Dark Souls 3 - Open Server
 * Copyright (C) 2021 Tim Leonard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

#include "Shared/Platform/Platform.h"
#include "Config/RuntimeConfig.h"

#include "Injector/Hooks/Hook.h"

#include <memory>
#include <vector>
#include <filesystem>
#include <queue>
#include <unordered_map>
#include <optional>

// Core of this application, manages all the 
// network services that ds3 uses. 

class Injector
{
public:
    static Injector& Instance();

    Injector();
    ~Injector();

    bool Init();
    bool Term();
    void RunUntilQuit();

    void SaveConfig();

    const RuntimeConfig& GetConfig()    { return Config; }

    intptr_t GetBaseAddress();
    
    using AOBByte = std::optional<uint8_t>;
    std::vector<intptr_t> SearchAOB(const std::vector<AOBByte>& pattern);

private:

    bool QuitReceived = false;

    static inline Injector* s_instance = nullptr;

    std::filesystem::path DllPath;
    std::filesystem::path ConfigPath;

    std::pair<intptr_t, size_t> ModuleRegion;

    RuntimeConfig Config;

    std::vector<std::unique_ptr<Hook>> Hooks;
    std::vector<Hook*> InstalledHooks;

};