#include "png.h"

#include <cstdio>
#include <cstring>

namespace gfx {

void Image::clear(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    for (std::size_t i = 0; i + 3 < px.size(); i += 4) {
        px[i] = r; px[i + 1] = g; px[i + 2] = b; px[i + 3] = a;
    }
}

namespace {

void fail(std::string* err, const char* what) { if (err) *err = what; }

// ------------------------------------------------------------ контрольные суммы

unsigned crc32_of(const unsigned char* p, std::size_t n, unsigned start) {
    static unsigned table[256];
    static bool built = false;
    if (!built) {
        built = true;
        for (unsigned i = 0; i < 256; ++i) {
            unsigned c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
    }
    unsigned c = start ^ 0xFFFFFFFFu;
    for (std::size_t i = 0; i < n; ++i) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

unsigned adler32_of(const unsigned char* p, std::size_t n) {
    unsigned a = 1, b = 0;
    for (std::size_t i = 0; i < n; ++i) {
        a = (a + p[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

// ------------------------------------------------------------------- inflate
//
// Разбор по одному биту: тайлсет — считанные килобайты, и скорость здесь
// не стоит ни строчки лишней сложности.

struct BitReader {
    const unsigned char* p;
    std::size_t n, pos;
    unsigned buf;
    int cnt;
    bool bad;

    BitReader(const unsigned char* pp, std::size_t nn)
        : p(pp), n(nn), pos(0), buf(0), cnt(0), bad(false) {}

    int bit() {
        if (cnt == 0) {
            if (pos >= n) { bad = true; return 0; }
            buf = p[pos++];
            cnt = 8;
        }
        const int b = static_cast<int>(buf & 1u);
        buf >>= 1;
        --cnt;
        return b;
    }

    unsigned bits(int need) {
        unsigned v = 0;
        for (int i = 0; i < need; ++i) v |= static_cast<unsigned>(bit()) << i;
        return v;
    }

    void align() { cnt = 0; buf = 0; }
};

// Канонический код Хаффмана: сколько кодов каждой длины и какие символы им
// соответствуют, по возрастанию длины и номера.
struct Huffman {
    short count[16];
    std::vector<short> symbol;
};

void huff_build(Huffman* h, const unsigned char* lengths, int n) {
    for (int i = 0; i < 16; ++i) h->count[i] = 0;
    for (int i = 0; i < n; ++i) ++h->count[lengths[i]];
    h->count[0] = 0;

    short offs[16];
    offs[0] = offs[1] = 0;
    for (int len = 1; len < 15; ++len)
        offs[len + 1] = static_cast<short>(offs[len] + h->count[len]);

    h->symbol.assign(static_cast<std::size_t>(n), 0);
    for (int i = 0; i < n; ++i)
        if (lengths[i]) h->symbol[static_cast<std::size_t>(offs[lengths[i]]++)] =
            static_cast<short>(i);
}

int huff_decode(BitReader& br, const Huffman& h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; ++len) {
        code |= br.bit();
        const int count = h.count[len];
        if (code - count < first)
            return h.symbol[static_cast<std::size_t>(index + (code - first))];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

const unsigned short LEN_BASE[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
    67, 83, 99, 115, 131, 163, 195, 227, 258
};
const unsigned char LEN_EXTRA[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5, 0
};
const unsigned short DIST_BASE[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
    769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
const unsigned char DIST_EXTRA[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

void fixed_tables(Huffman* lit, Huffman* dist) {
    unsigned char l[288];
    for (int i = 0; i < 144; ++i) l[i] = 8;
    for (int i = 144; i < 256; ++i) l[i] = 9;
    for (int i = 256; i < 280; ++i) l[i] = 7;
    for (int i = 280; i < 288; ++i) l[i] = 8;
    huff_build(lit, l, 288);

    unsigned char d[30];
    for (int i = 0; i < 30; ++i) d[i] = 5;
    huff_build(dist, d, 30);
}

bool inflate_block(BitReader& br, const Huffman& lit, const Huffman& dist,
                   std::vector<unsigned char>* out) {
    for (;;) {
        const int sym = huff_decode(br, lit);
        if (br.bad || sym < 0) return false;
        if (sym < 256) { out->push_back(static_cast<unsigned char>(sym)); continue; }
        if (sym == 256) return true;

        const int li = sym - 257;
        if (li >= 29) return false;
        const unsigned len = LEN_BASE[li] + br.bits(LEN_EXTRA[li]);

        const int di = huff_decode(br, dist);
        if (di < 0 || di >= 30) return false;
        const unsigned d = DIST_BASE[di] + br.bits(DIST_EXTRA[di]);
        if (br.bad || d == 0 || d > out->size()) return false;

        // Совпадение может перекрывать себя — копируем строго по байту.
        std::size_t from = out->size() - d;
        for (unsigned i = 0; i < len; ++i) out->push_back((*out)[from + i]);
    }
}

bool inflate(const unsigned char* p, std::size_t n, std::vector<unsigned char>* out) {
    BitReader br(p, n);
    for (;;) {
        const int final_block = br.bit();
        const unsigned type = br.bits(2);
        if (br.bad) return false;

        if (type == 0) {
            br.align();
            if (br.pos + 4 > br.n) return false;
            const unsigned len = static_cast<unsigned>(br.p[br.pos]) |
                                 (static_cast<unsigned>(br.p[br.pos + 1]) << 8);
            br.pos += 4;                       // LEN уже взяли, NLEN не нужен
            if (br.pos + len > br.n) return false;
            out->insert(out->end(), br.p + br.pos, br.p + br.pos + len);
            br.pos += len;
        } else if (type == 1) {
            Huffman lit, dist;
            fixed_tables(&lit, &dist);
            if (!inflate_block(br, lit, dist, out)) return false;
        } else if (type == 2) {
            const unsigned hlit  = br.bits(5) + 257;
            const unsigned hdist = br.bits(5) + 1;
            const unsigned hclen = br.bits(4) + 4;
            if (br.bad || hlit > 286 || hdist > 30) return false;

            static const unsigned char ORDER[19] = {
                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
            };
            unsigned char clen[19];
            std::memset(clen, 0, sizeof clen);
            for (unsigned i = 0; i < hclen; ++i)
                clen[ORDER[i]] = static_cast<unsigned char>(br.bits(3));
            Huffman cl;
            huff_build(&cl, clen, 19);

            unsigned char lengths[320];
            std::memset(lengths, 0, sizeof lengths);
            unsigned i = 0;
            while (i < hlit + hdist) {
                const int sym = huff_decode(br, cl);
                if (br.bad || sym < 0) return false;
                if (sym < 16) {
                    lengths[i++] = static_cast<unsigned char>(sym);
                } else if (sym == 16) {
                    if (i == 0) return false;
                    const unsigned char prev = lengths[i - 1];
                    unsigned rep = 3 + br.bits(2);
                    while (rep-- && i < hlit + hdist) lengths[i++] = prev;
                } else if (sym == 17) {
                    unsigned rep = 3 + br.bits(3);
                    while (rep-- && i < hlit + hdist) lengths[i++] = 0;
                } else {
                    unsigned rep = 11 + br.bits(7);
                    while (rep-- && i < hlit + hdist) lengths[i++] = 0;
                }
            }
            if (br.bad) return false;

            Huffman lit, dist;
            huff_build(&lit, lengths, static_cast<int>(hlit));
            huff_build(&dist, lengths + hlit, static_cast<int>(hdist));
            if (!inflate_block(br, lit, dist, out)) return false;
        } else {
            return false;                      // тип 3 не существует
        }

        if (final_block) return true;
        if (br.bad) return false;
    }
}

// ------------------------------------------------------------------- deflate
//
// Фиксированные коды Хаффмана и жадный поиск совпадений по хеш-цепочкам.
// Динамических кодов здесь нет намеренно: на пиксельной графике с большими
// одноцветными пятнами почти весь выигрыш даёт именно поиск повторов, а
// таблицы кодов — это ещё сотня строк ради последних процентов.

struct BitWriter {
    std::vector<unsigned char>* out;
    unsigned buf;
    int cnt;

    explicit BitWriter(std::vector<unsigned char>* o) : out(o), buf(0), cnt(0) {}

    // Обычные биты идут младшим вперёд — так устроен deflate.
    void bits(unsigned v, int n) {
        for (int i = 0; i < n; ++i) {
            buf |= ((v >> i) & 1u) << cnt;
            if (++cnt == 8) { out->push_back(static_cast<unsigned char>(buf)); buf = 0; cnt = 0; }
        }
    }
    // А коды Хаффмана — старшим: единственное исключение во всём формате.
    void code(unsigned v, int n) {
        for (int i = n - 1; i >= 0; --i) {
            buf |= ((v >> i) & 1u) << cnt;
            if (++cnt == 8) { out->push_back(static_cast<unsigned char>(buf)); buf = 0; cnt = 0; }
        }
    }
    void flush() {
        if (cnt) { out->push_back(static_cast<unsigned char>(buf)); buf = 0; cnt = 0; }
    }
};

void emit_symbol(BitWriter& bw, int sym) {
    if (sym < 144) bw.code(0x30u + static_cast<unsigned>(sym), 8);
    else if (sym < 256) bw.code(0x190u + static_cast<unsigned>(sym) - 144u, 9);
    else if (sym < 280) bw.code(static_cast<unsigned>(sym) - 256u, 7);
    else bw.code(0xC0u + static_cast<unsigned>(sym) - 280u, 8);
}

const int WBITS = 15;
const int WSIZE = 1 << WBITS;
const int HBITS = 15;
const int HSIZE = 1 << HBITS;
const int MAX_MATCH = 258;
const int MIN_MATCH = 3;
const int MAX_CHAIN = 128;

int hash3(const unsigned char* p) {
    return ((static_cast<int>(p[0]) << 10) ^ (static_cast<int>(p[1]) << 5) ^
            static_cast<int>(p[2])) & (HSIZE - 1);
}

void deflate_fixed(const std::vector<unsigned char>& in, std::vector<unsigned char>* out) {
    BitWriter bw(out);
    bw.bits(1, 1);          // единственный блок, он же последний
    bw.bits(1, 2);          // фиксированные коды

    const std::size_t n = in.size();
    std::vector<int> head(static_cast<std::size_t>(HSIZE), -1);
    std::vector<int> prev(n ? n : 1, -1);

    std::size_t i = 0;
    while (i < n) {
        int best_len = 0, best_dist = 0;
        if (i + MIN_MATCH <= n) {
            const int h = hash3(&in[i]);
            int cand = head[static_cast<std::size_t>(h)];
            int chain = MAX_CHAIN;
            while (cand >= 0 && chain-- > 0) {
                const std::size_t d = i - static_cast<std::size_t>(cand);
                if (d == 0 || d > static_cast<std::size_t>(WSIZE)) break;
                std::size_t len = 0;
                const std::size_t cap = (n - i) < static_cast<std::size_t>(MAX_MATCH)
                                        ? (n - i) : static_cast<std::size_t>(MAX_MATCH);
                while (len < cap && in[static_cast<std::size_t>(cand) + len] == in[i + len]) ++len;
                if (static_cast<int>(len) > best_len) {
                    best_len = static_cast<int>(len);
                    best_dist = static_cast<int>(d);
                    if (best_len >= MAX_MATCH) break;
                }
                cand = prev[static_cast<std::size_t>(cand)];
            }
        }

        if (best_len >= MIN_MATCH) {
            int li = 0;
            while (li < 28 && LEN_BASE[li + 1] <= static_cast<unsigned>(best_len)) ++li;
            emit_symbol(bw, 257 + li);
            bw.bits(static_cast<unsigned>(best_len) - LEN_BASE[li], LEN_EXTRA[li]);

            int di = 0;
            while (di < 29 && DIST_BASE[di + 1] <= static_cast<unsigned>(best_dist)) ++di;
            bw.code(static_cast<unsigned>(di), 5);
            bw.bits(static_cast<unsigned>(best_dist) - DIST_BASE[di], DIST_EXTRA[di]);

            for (int k = 0; k < best_len; ++k) {
                if (i + MIN_MATCH <= n) {
                    const int h = hash3(&in[i]);
                    prev[i] = head[static_cast<std::size_t>(h)];
                    head[static_cast<std::size_t>(h)] = static_cast<int>(i);
                }
                ++i;
            }
        } else {
            emit_symbol(bw, in[i]);
            if (i + MIN_MATCH <= n) {
                const int h = hash3(&in[i]);
                prev[i] = head[static_cast<std::size_t>(h)];
                head[static_cast<std::size_t>(h)] = static_cast<int>(i);
            }
            ++i;
        }
    }

    emit_symbol(bw, 256);
    bw.flush();
}

// ------------------------------------------------------------------ PNG: чтение

unsigned be32(const unsigned char* p) {
    return (static_cast<unsigned>(p[0]) << 24) | (static_cast<unsigned>(p[1]) << 16) |
           (static_cast<unsigned>(p[2]) << 8) | static_cast<unsigned>(p[3]);
}

int channels_of(int color_type) {
    switch (color_type) {
        case 0: return 1;   // серый
        case 2: return 3;   // RGB
        case 3: return 1;   // палитра
        case 4: return 2;   // серый + альфа
        case 6: return 4;   // RGBA
        default: return 0;
    }
}

int paeth(int a, int b, int c) {
    const int p = a + b - c;
    const int pa = p > a ? p - a : a - p;
    const int pb = p > b ? p - b : b - p;
    const int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

// Значение канала из потока: глубина 16 бит ужимается до восьми, меньшая —
// растягивается, чтобы белое осталось белым, а не потускнело.
unsigned sample_at(const unsigned char* row, int index, int depth) {
    if (depth == 8)  return row[index];
    if (depth == 16) return row[index * 2];
    const int per_byte = 8 / depth;
    const unsigned byte = row[index / per_byte];
    const int shift = 8 - depth * (index % per_byte + 1);
    return (byte >> shift) & ((1u << depth) - 1u);
}

unsigned scale_to_8(unsigned v, int depth) {
    switch (depth) {
        case 1:  return v ? 255u : 0u;
        case 2:  return v * 85u;
        case 4:  return v * 17u;
        default: return v;
    }
}

bool decode(const unsigned char* data, std::size_t n, Image* out, std::string* err) {
    static const unsigned char SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (n < 8 || std::memcmp(data, SIG, 8) != 0) {
        fail(err, "это не PNG: не та подпись в начале файла");
        return false;
    }

    int w = 0, h = 0, depth = 0, color_type = 0, interlace = 0;
    bool have_ihdr = false;
    std::vector<unsigned char> idat, palette, trns;

    std::size_t pos = 8;
    for (;;) {
        if (pos + 8 > n) { fail(err, "PNG обрывается посреди заголовка блока"); return false; }
        const unsigned len = be32(data + pos);
        const unsigned char* type = data + pos + 4;
        if (len > n || pos + 12 + len > n) { fail(err, "PNG обрывается посреди блока"); return false; }
        const unsigned char* body = data + pos + 8;

        const unsigned want = be32(data + pos + 8 + len);
        if (crc32_of(data + pos + 4, len + 4, 0) != want) {
            fail(err, "PNG повреждён: не сходится контрольная сумма блока");
            return false;
        }

        if (std::memcmp(type, "IHDR", 4) == 0) {
            if (len < 13) { fail(err, "PNG: короткий IHDR"); return false; }
            w = static_cast<int>(be32(body));
            h = static_cast<int>(be32(body + 4));
            depth = body[8];
            color_type = body[9];
            interlace = body[12];
            have_ihdr = true;
            if (w <= 0 || h <= 0) { fail(err, "PNG: нулевой размер"); return false; }
            if (interlace != 0) {
                fail(err, "чересстрочный PNG (Adam7) не читается — сохраните обычным");
                return false;
            }
            if (channels_of(color_type) == 0) { fail(err, "PNG: неизвестный тип цвета"); return false; }
            if (depth != 1 && depth != 2 && depth != 4 && depth != 8 && depth != 16) {
                fail(err, "PNG: неподдерживаемая глубина цвета");
                return false;
            }
            if (color_type != 0 && color_type != 3 && depth < 8) {
                fail(err, "PNG: такая глубина бывает только у серого и палитры");
                return false;
            }
        } else if (std::memcmp(type, "PLTE", 4) == 0) {
            palette.assign(body, body + len);
        } else if (std::memcmp(type, "tRNS", 4) == 0) {
            trns.assign(body, body + len);
        } else if (std::memcmp(type, "IDAT", 4) == 0) {
            idat.insert(idat.end(), body, body + len);
        } else if (std::memcmp(type, "IEND", 4) == 0) {
            break;
        }

        pos += 12 + len;
    }

    if (!have_ihdr) { fail(err, "PNG без IHDR"); return false; }
    if (idat.size() < 3) { fail(err, "PNG без данных изображения"); return false; }
    if (color_type == 3 && palette.empty()) { fail(err, "PNG с палитрой, но без PLTE"); return false; }

    // zlib: два байта заголовка, потом deflate, потом adler32.
    if ((idat[0] & 0x0F) != 8) { fail(err, "PNG сжат неизвестным способом"); return false; }
    if (idat[1] & 0x20) { fail(err, "PNG со словарём не читается"); return false; }

    std::vector<unsigned char> raw;
    const int chans = channels_of(color_type);
    const std::size_t stride = (static_cast<std::size_t>(w) * static_cast<std::size_t>(chans) *
                                static_cast<std::size_t>(depth) + 7) / 8;
    raw.reserve((stride + 1) * static_cast<std::size_t>(h));
    if (!inflate(&idat[2], idat.size() - 2, &raw)) {
        fail(err, "PNG не распаковывается: битый поток deflate");
        return false;
    }
    if (raw.size() < (stride + 1) * static_cast<std::size_t>(h)) {
        fail(err, "PNG короче, чем обещает заголовок");
        return false;
    }

    // Снятие фильтров: каждая строка начинается байтом с номером фильтра.
    const std::size_t bpp = (static_cast<std::size_t>(chans) *
                             static_cast<std::size_t>(depth) + 7) / 8;
    std::vector<unsigned char> lines(stride * static_cast<std::size_t>(h), 0);
    for (int y = 0; y < h; ++y) {
        const unsigned char filter = raw[(stride + 1) * static_cast<std::size_t>(y)];
        const unsigned char* src = &raw[(stride + 1) * static_cast<std::size_t>(y) + 1];
        unsigned char* cur = &lines[stride * static_cast<std::size_t>(y)];
        const unsigned char* up = y ? &lines[stride * static_cast<std::size_t>(y - 1)] : 0;

        for (std::size_t x = 0; x < stride; ++x) {
            const int a = x >= bpp ? cur[x - bpp] : 0;
            const int b = up ? up[x] : 0;
            const int c = (up && x >= bpp) ? up[x - bpp] : 0;
            int v = src[x];
            switch (filter) {
                case 0: break;
                case 1: v += a; break;
                case 2: v += b; break;
                case 3: v += (a + b) / 2; break;
                case 4: v += paeth(a, b, c); break;
                default: fail(err, "PNG: неизвестный фильтр строки"); return false;
            }
            cur[x] = static_cast<unsigned char>(v & 0xFF);
        }
    }

    Image img(w, h);
    for (int y = 0; y < h; ++y) {
        const unsigned char* row = &lines[stride * static_cast<std::size_t>(y)];
        for (int x = 0; x < w; ++x) {
            unsigned r = 0, g = 0, b = 0, a = 255;
            if (color_type == 3) {
                const unsigned idx = sample_at(row, x, depth);
                const std::size_t o = static_cast<std::size_t>(idx) * 3;
                if (o + 2 < palette.size()) { r = palette[o]; g = palette[o + 1]; b = palette[o + 2]; }
                if (idx < trns.size()) a = trns[idx];
            } else if (color_type == 0 || color_type == 4) {
                const unsigned v = scale_to_8(sample_at(row, x * chans, depth), depth);
                r = g = b = v;
                if (color_type == 4)
                    a = scale_to_8(sample_at(row, x * chans + 1, depth), depth);
                else if (trns.size() >= 2 && depth <= 8) {
                    // tRNS для серого хранит образец в двух байтах.
                    const unsigned key = scale_to_8(trns[1], depth);
                    if (v == key) a = 0;
                }
            } else {
                r = scale_to_8(sample_at(row, x * chans, depth), depth);
                g = scale_to_8(sample_at(row, x * chans + 1, depth), depth);
                b = scale_to_8(sample_at(row, x * chans + 2, depth), depth);
                if (color_type == 6)
                    a = scale_to_8(sample_at(row, x * chans + 3, depth), depth);
                else if (trns.size() >= 6 && depth == 8 &&
                         r == trns[1] && g == trns[3] && b == trns[5]) {
                    a = 0;
                }
            }
            img.set(x, y, static_cast<unsigned char>(r), static_cast<unsigned char>(g),
                    static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        }
    }

    *out = img;
    return true;
}

// ------------------------------------------------------------------ PNG: запись

void put_be32(std::vector<unsigned char>* v, unsigned x) {
    v->push_back(static_cast<unsigned char>((x >> 24) & 0xFF));
    v->push_back(static_cast<unsigned char>((x >> 16) & 0xFF));
    v->push_back(static_cast<unsigned char>((x >> 8) & 0xFF));
    v->push_back(static_cast<unsigned char>(x & 0xFF));
}

void put_chunk(std::vector<unsigned char>* v, const char* type,
               const unsigned char* body, std::size_t n) {
    put_be32(v, static_cast<unsigned>(n));
    const std::size_t at = v->size();
    v->insert(v->end(), type, type + 4);
    if (n) v->insert(v->end(), body, body + n);
    put_be32(v, crc32_of(&(*v)[at], n + 4, 0));
}

} // namespace

bool png_write_mem(const Image& img, std::vector<unsigned char>* out, std::string* err) {
    if (!out) return false;
    if (img.empty()) { fail(err, "нечего записывать: пустая картинка"); return false; }

    // Фильтр 0 на каждой строке: искать лучший смысла нет — повторы всё равно
    // ловит deflate, а с фильтром запись стала бы вдвое длиннее.
    std::vector<unsigned char> raw;
    raw.reserve((static_cast<std::size_t>(img.w) * 4 + 1) * static_cast<std::size_t>(img.h));
    for (int y = 0; y < img.h; ++y) {
        raw.push_back(0);
        const unsigned char* row = img.at(0, y);
        raw.insert(raw.end(), row, row + static_cast<std::size_t>(img.w) * 4);
    }

    std::vector<unsigned char> z;
    z.push_back(0x78);          // CM=8, окно 32K
    z.push_back(0x01);          // проверочные биты: (0x78<<8|0x01) кратно 31
    deflate_fixed(raw, &z);
    put_be32(&z, adler32_of(raw.empty() ? 0 : &raw[0], raw.size()));

    static const unsigned char SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    out->assign(SIG, SIG + 8);

    unsigned char ihdr[13];
    ihdr[0] = static_cast<unsigned char>((img.w >> 24) & 0xFF);
    ihdr[1] = static_cast<unsigned char>((img.w >> 16) & 0xFF);
    ihdr[2] = static_cast<unsigned char>((img.w >> 8) & 0xFF);
    ihdr[3] = static_cast<unsigned char>(img.w & 0xFF);
    ihdr[4] = static_cast<unsigned char>((img.h >> 24) & 0xFF);
    ihdr[5] = static_cast<unsigned char>((img.h >> 16) & 0xFF);
    ihdr[6] = static_cast<unsigned char>((img.h >> 8) & 0xFF);
    ihdr[7] = static_cast<unsigned char>(img.h & 0xFF);
    ihdr[8] = 8;                // глубина
    ihdr[9] = 6;                // RGBA
    ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    put_chunk(out, "IHDR", ihdr, sizeof ihdr);
    put_chunk(out, "IDAT", z.empty() ? 0 : &z[0], z.size());
    put_chunk(out, "IEND", 0, 0);
    return true;
}

bool png_write(const std::string& path, const Image& img, std::string* err) {
    std::vector<unsigned char> bytes;
    if (!png_write_mem(img, &bytes, err)) return false;

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { fail(err, "не открыть файл на запись"); return false; }
    const std::size_t wrote = std::fwrite(&bytes[0], 1, bytes.size(), f);
    const bool ok = (std::fclose(f) == 0) && wrote == bytes.size();
    if (!ok) fail(err, "не записать файл целиком");
    return ok;
}

bool png_read_mem(const unsigned char* data, std::size_t n, Image* out, std::string* err) {
    if (!data || !out) return false;
    return decode(data, n, out, err);
}

bool png_read(const std::string& path, Image* out, std::string* err) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { fail(err, "файл не открывается"); return false; }

    std::vector<unsigned char> bytes;
    unsigned char chunk[8192];
    for (;;) {
        const std::size_t got = std::fread(chunk, 1, sizeof chunk, f);
        if (!got) break;
        bytes.insert(bytes.end(), chunk, chunk + got);
    }
    std::fclose(f);

    if (bytes.empty()) { fail(err, "файл пуст"); return false; }
    return decode(&bytes[0], bytes.size(), out, err);
}

} // namespace gfx
