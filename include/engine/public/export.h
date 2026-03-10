#pragma once

#if defined(_WIN32)
#if defined(ENGINE_PUBLIC_BUILD_DLL)
#define ENGINE_PUBLIC_API __declspec(dllexport)
#elif defined(ENGINE_PUBLIC_USE_DLL)
#define ENGINE_PUBLIC_API __declspec(dllimport)
#else
#define ENGINE_PUBLIC_API
#endif
#else
#define ENGINE_PUBLIC_API
#endif
