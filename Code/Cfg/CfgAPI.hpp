#pragma once

#if defined(CFG_DLL_EXPORT)
  #define CFG_API __declspec(dllexport)
#else
  #define CFG_API __declspec(dllimport)
#endif
