#pragma once
#include "draw.h"
#include "tiles.h"

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
    bool load(SDL_Renderer* ren, const std::string& dir, const std::string& sheet,
              std::string* err);
    // Готовые картинки по слотам — этим пользуется и load, и тесты.
    bool build(SDL_Renderer* ren, const std::vector<Image>& tiles, std::string* err);
    void unload();

    bool ok() const { return tex_ != 0; }
    int  tile_w() const { return tw_; }
    int  tile_h() const { return th_; }
    // Нарисован ли слот. Пустая ячейка листа — «этот тайл ещё не рисовали».
    bool has(int slot) const;

    // Рисует слот в прямоугольник. false — слота нет, рисуйте по-старому.
    bool draw(SDL_Renderer* ren, int slot, const Rect& dst) const;

private:
    SDL_Texture* tex_;
    int  cw_, ch_;                 // ячейка атласа
    int  tw_, th_;                 // самый крупный тайл в листе
    Rect src_[TJTM_SLOTS];         // нулевая ширина — слот пуст

    Tileset(const Tileset&);
    Tileset& operator=(const Tileset&);
};

} // namespace gfx
