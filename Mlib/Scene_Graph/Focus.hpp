#pragma once
#include <Mlib/Threads/Checked_Mutex.hpp>
#include <Mlib/Threads/Containers/Thread_Safe_String.hpp>
#include <atomic>
#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace Mlib {

struct FocusFilter;

enum class Focus {
    NONE = 0,
    BASE = 1 << 0,
    MAIN_MENU = 1 << 1,
    NEW_GAME_MENU = 1 << 2,
    SETTINGS_MENU = 1 << 3,
    CONTROLS_MENU = 1 << 4,
    LOADING = 1 << 5,
    COUNTDOWN_PENDING = 1 << 6,
    COUNTDOWN_COUNTING = 1 << 7,
    GAME_OVER_COUNTDOWN_PENDING = 1 << 8,
    GAME_OVER_COUNTDOWN_COUNTING = 1 << 9,
    SCENE = 1 << 10,
    GAME_OVER = 1 << 11,  // currently not in use, countdown is used instead.
    MENU_ANY = MAIN_MENU | NEW_GAME_MENU | SETTINGS_MENU | CONTROLS_MENU,
    COUNTDOWN_ANY = COUNTDOWN_PENDING | COUNTDOWN_COUNTING,
    GAME_OVER_COUNTDOWN_ANY = GAME_OVER_COUNTDOWN_PENDING | GAME_OVER_COUNTDOWN_COUNTING,
    ALWAYS =
        BASE |
        MAIN_MENU |
        NEW_GAME_MENU |
        SETTINGS_MENU |
        CONTROLS_MENU |
        LOADING |
        COUNTDOWN_PENDING |
        COUNTDOWN_COUNTING |
        GAME_OVER_COUNTDOWN_PENDING |
        GAME_OVER_COUNTDOWN_COUNTING |
        SCENE |
        GAME_OVER
};

inline Focus operator | (Focus a, Focus b) {
    return Focus((unsigned int)a | (unsigned int)b);
}

inline Focus operator & (Focus a, Focus b) {
    return Focus((unsigned int)a & (unsigned int)b);
}

inline Focus& operator |= (Focus& a, Focus b) {
    a = a | b;
    return a;
}

inline bool any(Focus f) {
    return f != Focus::NONE;
}

class Focuses {
    friend std::ostream& operator << (std::ostream& ostr, const Focuses& focuses);
public:
    Focuses();
    ~Focuses();
    Focuses(const std::initializer_list<Focus>& focuses);
    Focuses(const Focuses&) = delete;
    Focuses& operator = (const Focuses&) = delete;
    void set_focuses(const std::initializer_list<Focus>& focuses);
    void set_focuses(const std::vector<Focus>& focuses);
    Focus focus() const;
    std::list<Focus>::const_iterator find(Focus focus) const;
    std::list<Focus>::iterator find(Focus focus);
    std::list<Focus>::const_iterator end() const;
    std::list<Focus>::iterator end();
    void erase(const std::list<Focus>::iterator& it);
    void pop_back();
    void push_back(Focus focus);
    bool contains(Focus focus) const;
    bool countdown_active() const;
    bool game_over_countdown_active() const;
    size_t size() const;
    mutable CheckedMutex mutex;
private:
    std::list<Focus> focuses_;
};

std::ostream& operator << (std::ostream& ostr, const Focuses& focuses);

struct SubmenuHeader {
    std::string title;
    std::string icon;
    std::vector<std::string> requires_;
};

enum class PersistedValueType {
    DEFAULT,
    CUSTOM
};

class UiFocus {
public:
    explicit UiFocus(std::string filename);
    ~UiFocus();
    UiFocus(const UiFocus&) = delete;
    UiFocus& operator = (const UiFocus&) = delete;
    Focuses focuses;
    std::map<std::string, std::atomic_size_t> menu_selection_ids;
    std::map<std::string, size_t> submenu_numbers;
    std::vector<SubmenuHeader> submenu_headers;
    std::vector<Focus> focus_masks;
    std::map<std::string, std::atomic_size_t> all_selection_ids;
    void set_persisted_selection_id(const std::string& submenu, const std::string& s, PersistedValueType cb);
    std::string get_persisted_selection_id(const std::string& submenu) const;
    void set_requires_reload(std::string submenu, std::string reason);
    void clear_requires_reload(const std::string& submenu);
    const std::map<std::string, std::string>& requires_reload() const;
    void insert_submenu(
        const std::string& id,
        const SubmenuHeader& header,
        Focus focus_mask,
        size_t default_selection);
    bool has_focus(const FocusFilter& focus_filter) const;
    void clear();
    bool can_load() const;
    bool can_save() const;
    bool has_changes() const;
    void load();
    void save();
private:
    bool get_has_changes() const;
    std::map<std::string, ThreadSafeString> loaded_persistent_selection_ids;
    std::map<std::string, ThreadSafeString> current_persistent_selection_ids;
    std::string filename_;
    bool has_changes_;
    std::map<std::string, std::string> requires_reload_;
};

Focus single_focus_from_string(const std::string& str);
Focus focus_from_string(const std::string& str);
std::string focus_to_string(Focus focus);

}
