#ifndef __FILE_ENUM_H__
#define __FILE_ENUM_H__

#include <string>

typedef bool (*EnumCallback)(const std::string& path, void* param);

bool enumFile(const std::string& searchPath, EnumCallback callback, void* param);

bool addPackFile(const std::string& filename, void* param);

bool countFile(const std::string& filename, void* param);

#endif
