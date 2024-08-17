#pragma once
#include "platform_detection.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <string>
#include <fstream>

#if defined(E_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(E_PLATFORM_LINUX)
#include <unistd.h>
#elif defined(E_PLATFORM_MACOS)
#include <libgen.h>
#include <mach-o/dyld.h>
#endif

#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <linux/limits.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

class FileSystemUtil
{
public:
	static fs::path GetExecutablePath()
	{
#if defined(_WIN32)
		wchar_t path[MAX_PATH] = {0};
		GetModuleFileNameW(NULL, path, MAX_PATH);
		return {path};
#elif defined(__linux__)
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		return fs::path(std::string(result, (count > 0) ? count : 0));
#elif defined(__APPLE__)
		char path[1024];
		uint32_t size = sizeof(path);
		if (_NSGetExecutablePath(path, &size) == 0)
		{
			return fs::canonical(path);
		}
		throw std::runtime_error("Failed to get executable path");
#else
		throw std::runtime_error("Unsupported platform");
#endif
	}

	static fs::path GetProjectRoot()
	{
		fs::path execPath = GetExecutablePath();
		fs::path projectRoot = execPath.parent_path().parent_path();

		// Validate that we've found the correct project root
		if (!fs::exists(projectRoot / "assets"))
		{
			throw std::runtime_error("Failed to locate project root directory");
		}

		return projectRoot;
	}

	static fs::path GetAssetDirectory()
	{
		return GetProjectRoot() / "assets";
	}

	static fs::path GetShaderDirectory()
	{
		return GetAssetDirectory() / "shaders";
	}

	static fs::path GetFontDirectory()
	{
		return GetAssetDirectory() / "fonts";
	}

	static fs::path GetTextureDirectory()
	{
		return GetAssetDirectory() / "textures";
	}

	static fs::path GetModelDirectory()
	{
		return GetAssetDirectory() / "models";
	}

	static std::string PathToString(const fs::path& path)
	{
		return path.string();
	}

	static fs::path StringToPath(const std::string& str)
	{
		return {str};
	}

	static std::string ReadFileToString(const fs::path& filePath)
	{
		if (!fs::exists(filePath))
		{
			throw std::runtime_error("File does not exist: " + filePath.string());
		}

		std::ifstream file(filePath, std::ios::in | std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("Failed to open file: " + filePath.string());
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return content;
	}

	static std::string ReadFileToString(const std::string& filePath)
	{
		return ReadFileToString(StringToPath(filePath));
	}
};