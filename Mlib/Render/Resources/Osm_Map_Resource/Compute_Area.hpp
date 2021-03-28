#pragma once
#include <list>
#include <map>
#include <string>

namespace Mlib {

struct Node;

float compute_area(
    const std::list<std::string>& nd,
    const std::map<std::string, Node>& nodes,
    float scale);

}
