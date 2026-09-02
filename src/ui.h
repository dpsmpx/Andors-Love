#pragma once
#include "game.h"

#include <string>
#include <vector>

namespace ui {

// Экран мира: карта + панель героя + журнал.
void draw_world(Game& g);

// Универсальный список: стрелки/WS — выбор, Enter/пробел — принять,
// Esc — отмена. Возвращает индекс, -1 при отмене или -2, если нажата одна
// из hotkeys (её код кладётся в hotkey_out) — так вызывающий добавляет свои
// клавиши, например переключение вкладок.
constexpr int CHOOSE_CANCEL = -1;
constexpr int CHOOSE_HOTKEY = -2;

int choose(const std::string& title, const std::vector<std::string>& items,
           const std::string& footer = "",
           const std::vector<int>& hotkeys = {}, int* hotkey_out = nullptr);

// Ввод строки в raw-режиме (работает и с перенаправленным вводом).
std::string read_line(const std::string& prompt, const std::string& def);

void message_box(const std::string& title, const std::string& body);

// Экраны героя.
void screen_character(Game& g);
void screen_inventory(Game& g);
void screen_quests(Game& g);
void screen_skills(Game& g);

// Диалог с NPC; при выборе торговли откроет магазин.
void run_dialogue(Game& g, const std::string& npc_id);
void run_shop(Game& g, const std::string& shop_id);

// Бой идёт до победы, бегства или смерти игрока.
void run_combat(Game& g);

void help_screen();

} // namespace ui
