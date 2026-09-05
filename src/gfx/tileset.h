#pragma once
#include "draw.h"
#include "tiles.h"

#include <map>
#include <string>

struct SDL_Renderer;
struct SDL_Texture;

// Лист тайлов на видеокарте. Всё, что можно посчитать без окна — формат,
// раскладка слотов, вид по умолчанию, — лежит в tiles.h и проверяется тестами;
// здесь остаётся только текстура и рисование.
//
// Лист необязателен: если файла нет, он не читается или нужный слот в нём
// пустой, игра рисует по-старому — заливкой и знаком шрифта. Поэтому набор
// можно рисовать по одному тайлу: нарисованное появляется в игре сразу,
// а ненарисованное продолжает выглядеть как раньше.

namespace gfx {

class Tileset {
public:
    Tileset();
    ~Tileset();

    // Собирает графику: сперва лист одним файлом, если он есть, потом
    // отдельные файлы каталога поверх него — нарисованный отдельно тайл
    // важнее того же тайла из листа. Возвращает false, если не нашлось
    // ни одного тайла; причину нечитаемого файла кладёт в err.
    //
    // named — имена дополнительных тайлов (npc_elder, mob_rat...). Они ищутся
    // в том же каталоге по <имя>.png и ложатся в тот же атлас: своя текстура
    // на каждое существо стоила бы дороже, чем несколько лишних ячеек.
    bool load(SDL_Renderer* ren, const std::string& dir, const std::string& sheet,
              std::string* err);
    bool load(SDL_Renderer* ren, const std::string& dir, const std::string& sheet,
              const std::vector<std::string>& named, std::string* err);
    // Готовые картинки по слотам — этим пользуется и load, и тесты.
    bool build(SDL_Renderer* ren, const std::vector<Image>& tiles, std::string* err);
    bool build(SDL_Renderer* ren, const std::vector<Image>& tiles,
               const std::vector<std::string>& names,
               const std::vector<Image>& named, std::string* err);
    void unload();

    bool ok() const { return tex_ != 0; }
    int  tile_w() const { return tw_; }
    int  tile_h() const { return th_; }
    // Нарисован ли слот. Пустая ячейка листа — «этот тайл ещё не рисовали».
    bool has(int slot) const;
    // Нарисован ли именованный тайл. Ненайденное имя — не ошибка: существо
    // рисуется общим тайлом своего вида.
    bool has_named(const std::string& name) const;

    // Рисует слот в прямоугольник. false — слота нет, рисуйте по-старому.
    bool draw(SDL_Renderer* ren, int slot, const Rect& dst) const;
    bool draw_named(SDL_Renderer* ren, const std::string& name, const Rect& dst) const;

private:
    // Общий путь отрисовки: у слота и у именованного тайла разница только
    // в том, как искали номер ячейки.
    bool draw_index(SDL_Renderer* ren, int idx, const Rect& dst) const;

    SDL_Texture* tex_;
    int  cw_, ch_;                 // ячейка атласа
    int  tw_, th_;                 // самый крупный тайл в листе
    int  cols_;                    // столбцов в атласе
    // Первые TJTM_SLOTS ячеек — слоты, дальше именованные тайлы.
    // Нулевая ширина — ячейка пуста.
    std::vector<Rect>          src_;
    std::map<std::string, int> named_;

    Tileset(const Tileset&);
    Tileset& operator=(const Tileset&);
};

} // namespace gfx
