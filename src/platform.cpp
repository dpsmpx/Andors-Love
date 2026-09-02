#include "platform.h"

#include <cstdio>

#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#  include <conio.h>
#  include <windows.h>
#else
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace platform {

static bool g_eof = false;

bool input_closed() { return g_eof; }

#if defined(_WIN32)

RawMode::RawMode() {
    // Включаем обработку ANSI-последовательностей (Windows 10+).
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode))
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);
}
RawMode::~RawMode() {}

int read_key() {
    if (g_eof) return KEY_EOF;
    int ch = _getch();
    if (ch == EOF) { g_eof = true; return KEY_EOF; }
    if (ch == 0 || ch == 224) {           // префикс расширенной клавиши
        switch (_getch()) {
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 75: return KEY_LEFT;
            case 77: return KEY_RIGHT;
            default: return 0;
        }
    }
    if (ch == '\r') return KEY_ENTER;
    return ch;
}

#else

static termios g_saved;
static bool g_raw = false;

RawMode::RawMode() {
    // Если ввод перенаправлен (тесты, пайп), raw-режим не нужен и невозможен.
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_saved) != 0) return;
    termios raw = g_saved;
    raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) g_raw = true;
}

RawMode::~RawMode() {
    if (g_raw) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved);
        g_raw = false;
    }
}

int read_key() {
    if (g_eof) return KEY_EOF;
    int ch = getchar();
    if (ch == EOF) { g_eof = true; return KEY_EOF; }
    if (ch != 27) return ch;

    // Возможная escape-последовательность стрелки: ESC [ A..D
    if (!g_raw) return KEY_ESC;
    int a = getchar();
    if (a != '[') { ungetc(a, stdin); return KEY_ESC; }
    switch (getchar()) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return KEY_ESC;
    }
}

#endif

void term_size(int* cols, int* rows) {
    int c = 0, r = 0;

#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        c = info.srWindow.Right  - info.srWindow.Left + 1;
        r = info.srWindow.Bottom - info.srWindow.Top  + 1;
    }
#else
    winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        c = ws.ws_col;
        r = ws.ws_row;
    }
#endif

    if (c <= 0) if (const char* e = std::getenv("COLUMNS")) c = std::atoi(e);
    if (r <= 0) if (const char* e = std::getenv("LINES"))   r = std::atoi(e);
    if (c <= 0) c = 80;
    if (r <= 0) r = 24;

    if (cols) *cols = c;
    if (rows) *rows = r;
}

std::string exe_dir(const char* argv0) {
    if (!argv0) return std::string();
    std::string p(argv0);
    std::size_t slash = p.find_last_of("/\\");
    if (slash == std::string::npos) return std::string();
    return p.substr(0, slash);
}

bool make_dir(const std::string& path) {
#if defined(_WIN32)
    return CreateDirectoryA(path.c_str(), nullptr) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
#else
    if (::mkdir(path.c_str(), 0755) == 0) return true;
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

void clear_screen() { std::fputs("\033[2J\033[H", stdout); }
void hide_cursor()  { std::fputs("\033[?25l", stdout); }
void show_cursor()  { std::fputs("\033[?25h", stdout); }

} // namespace platform
