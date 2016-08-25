#pragma once

#if defined(CORE_DLL_EXPORT)
  #define CORE_API __declspec(dllexport)
  #define CORE_TEMPLATE_EXPORT(AggregateType, ...) template AggregateType CORE_API __VA_ARGS__;
#else
  #define CORE_API __declspec(dllimport)
  #define CORE_TEMPLATE_EXPORT(AggregateType, ...) extern template AggregateType CORE_API __VA_ARGS__;
#endif
