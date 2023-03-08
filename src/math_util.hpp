#pragma once

#include <glm/glm.hpp>

template <typename T>
inline T floor_to(T x, typename T::value_type multiple) 
{
  return multiple * glm::floor(x / multiple);
}

template <typename T>
inline T ceil_to(T x, typename T::value_type multiple) 
{
  return multiple * glm::ceil(x / multiple);
}
