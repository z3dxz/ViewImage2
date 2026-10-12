
#pragma once
#include <string>
#include <cstddef>

struct Resource {
    void* data = nullptr;
    size_t size = 0;
};

void InitializeResourceManager();
Resource GetResource(const std::string& identifier);