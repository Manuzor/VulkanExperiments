#pragma once

#if defined(CORE_DLL_EXPORT)
  #define CORE_API __declspec(dllexport)
#else
  #define CORE_API __declspec(dllimport)
#endif
