
#include "resman.h"
#include <unordered_map>
#include <iostream>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(assets);

std::unordered_map<std::string, Resource> g_ResourceRegistry;

static void IndexVirtualDirectory(const cmrc::embedded_filesystem& fs, const std::string& path) {
    for (const auto& entry : fs.iterate_directory(path)) {
        std::string full_path = path.empty() ? entry.filename() : path + "/" + entry.filename();
        
        if (entry.is_directory()) {
            IndexVirtualDirectory(fs, full_path);
        } else if (entry.is_file()) {
            auto file = fs.open(full_path);
            
            Resource res;
            res.data = (void*)(file.begin());
            res.size = file.size();
            
            g_ResourceRegistry[full_path] = res;
        }
        std::cout << "Resource found: " << full_path << "\n";
    }
}

void InitializeResourceManager() {
    try {
        auto fs = cmrc::assets::get_filesystem();
        IndexVirtualDirectory(fs, "res");
        std::cout << "[Resource Manager] Initialized " << g_ResourceRegistry.size() << " assets.\n";
    } catch(std::exception e) {
        std::cout << "Resource Allocation Failed\n";
    }
}

Resource GetResource(const std::string& identifier) {
    auto it = g_ResourceRegistry.find(identifier);
    if (it != g_ResourceRegistry.end()) {
        return it->second;
    }
    std::cerr << "[Resource Manager] ERROR: Resource not found: " << identifier << "\n";
    return Resource{ nullptr, 0 };
}