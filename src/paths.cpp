#include "paths.h"
#include "platform.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace paths {

bool dir_writable(const std::string& dir) {
    if (dir.empty()) return false;
    if (!platform::make_dir(dir)) return false;
    const std::string probe = dir + "/.write-test";
    {
        std::ofstream out(probe);
        if (!out) return false;
        out << 'x';
        if (!out) return false;
    }
    std::remove(probe.c_str());
    return true;
}

std::string save_dir(const char* argv0) {
    if (const char* env = std::getenv("ANDORS_LOVE_SAVE"))
        if (dir_writable(env)) return env;

    if (dir_writable("saves")) return "saves";

    if (const char* home = std::getenv("HOME")) {
        const std::string d = std::string(home) + "/.andors-love";
        if (dir_writable(d)) return d;
    }
    // Рядом с самим бинарником. Внутри APK это приватный каталог приложения —
    // он доступен для записи, тогда как рабочий каталог может быть любым,
    // а HOME и TMPDIR вообще не заданы.
    const std::string exe = platform::exe_dir(argv0);
    if (!exe.empty()) {
        const std::string d = exe + "/saves";
        if (dir_writable(d)) return d;
    }
    if (const char* tmp = std::getenv("TMPDIR")) {
        const std::string d = std::string(tmp) + "/andors-love";
        if (dir_writable(d)) return d;
    }
    return "saves";   // не нашли — оставим привычный путь ради понятной ошибки
}

namespace {

bool has_maps(const std::string& dir) {
    if (dir.empty()) return false;
    std::ifstream probe(dir + "/village.map");
    return static_cast<bool>(probe);
}

} // namespace

std::string data_root(const char* argv0) {
    if (const char* env = std::getenv("ANDORS_LOVE_DATA"))
        if (has_maps(env)) return env;

    if (has_maps("data/maps")) return "data/maps";

    const std::string exe = platform::exe_dir(argv0);
    if (!exe.empty() && has_maps(exe + "/data/maps")) return exe + "/data/maps";

    return "data/maps";
}

} // namespace paths
