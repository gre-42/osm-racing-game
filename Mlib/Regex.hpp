#pragma once
#include <functional>
#include <list>
#include <regex>
#include <string>

namespace Mlib {

static const std::string substitute_pattern = "(?:\\S+:\\S*)?(?:(?:\\r?\\n|\\s)+\\S+:\\S*)*";

std::string substitute(
    const std::string& str,
    const std::string& replacements);

std::string merge_replacements(const std::initializer_list<const std::string>& replacements);

void findall(
    const std::string& str,
    const std::regex& pattern,
    const std::function<void(const std::smatch&)>& f);

std::list<std::pair<std::string, std::string>> find_all_name_values(
    const std::string& str,
    const std::string& name_pattern,
    const std::string& value_pattern);

class SubstitutionString {
public:
    SubstitutionString();
    explicit SubstitutionString(const std::string& s);
    std::string substitute(const std::string& t) const;
    void merge(const SubstitutionString& other);
    void clear();
private:
    std::string s_;
};

}
