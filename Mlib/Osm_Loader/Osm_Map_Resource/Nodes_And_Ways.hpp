#pragma once
#include <Mlib/Map/Map.hpp>
#include <map>
#include <set>
#include <string>

namespace Mlib {

struct Node;
struct Way;

struct NodesAndWays {
    Map<std::string, Node> nodes;
    std::map<std::string, Way> ways;
};

}
