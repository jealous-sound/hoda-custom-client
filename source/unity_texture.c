typedef enum {
    UNITY_TEXTURE_RGB24 = 3,
    UNITY_TEXTURE_RGBA32 = 4,
    UNITY_TEXTURE_ARGB32 = 5,
    UNITY_TEXTURE_DXT5 = 12,
    UNITY_TEXTURE_RGBA4444 = 13,
    UNITY_TEXTURE_PVRTC_RGB4 = 32,
    UNITY_TEXTURE_PVRTC_RGBA4 = 33,
} UnityTextureFormat;

#define TEXTURE_BLOCK_SIZE 4
#define TEXTURE_BLOCK_PIXELS (TEXTURE_BLOCK_SIZE * TEXTURE_BLOCK_SIZE)
#define PVRTC_BLOCK_BYTES 8
#define DXT5_BLOCK_BYTES 16

typedef struct {
    int r, g, b, a;
} PvrtcColor;

typedef struct {
    PvrtcColor a, b;
} PvrtcEndpoints;

static const int PVRTC_STANDARD_WEIGHTS[4] = {0, 3, 5, 8};
static const int PVRTC_PUNCHTHROUGH_WEIGHTS[4] = {0, 4, 4, 8};
static const int PVRTC_PUNCHTHROUGH_INDEX = 2;
static const int PVRTC_NEIGHBOUR_WEIGHTS[TEXTURE_BLOCK_SIZE][3] = {{2, 2, 0}, {1, 3, 0}, {0, 4, 0}, {0, 3, 1}};

static int TextureBlocksAcross(int pixels) {
    return (pixels + TEXTURE_BLOCK_SIZE - 1) / TEXTURE_BLOCK_SIZE;
}

static unsigned int ReadU16(const unsigned char *p) {
    return (unsigned int)p[0] | (unsigned int)p[1] << 8;
}

static unsigned int ReadU32(const unsigned char *p) {
    return ReadU16(p) | ReadU16(p + 2) << 16;
}

static void TexturePutPixel(Image *image, int x, int y, Color color) {
    if (x >= image->width || y >= image->height) return;
    ((Color *)image->data)[(image->height - 1 - y) * image->width + x] = color;
}

static void TextureCopyBlock(Image *image, int block_x, int block_y, const Color block[TEXTURE_BLOCK_PIXELS]) {
    for (int y = 0; y < TEXTURE_BLOCK_SIZE; y++)
        for (int x = 0; x < TEXTURE_BLOCK_SIZE; x++)
            TexturePutPixel(
                image,
                block_x * TEXTURE_BLOCK_SIZE + x,
                block_y * TEXTURE_BLOCK_SIZE + y,
                block[y * TEXTURE_BLOCK_SIZE + x]
            );
}

static int PvrtcMortonIndex(int x, int y, int min_dimension) {
    int offset = 0, shift = 0;
    for (int mask = 1; mask < min_dimension; mask <<= 1, shift++) offset |= ((y & mask) | ((x & mask) << 1)) << shift;
    return offset | ((x | y) >> shift) << (shift * 2);
}

static PvrtcEndpoints PvrtcReadEndpoints(const unsigned char *block) {
    unsigned int ca = ReadU16(block + 4), cb = ReadU16(block + 6);
    PvrtcEndpoints e;
    if (ca & 0x8000) {
        e.a = (PvrtcColor){ca >> 10 & 0x1f, ca >> 5 & 0x1f, (ca & 0x1e) | (ca >> 4 & 1), 0xf};
    } else {
        e.a = (PvrtcColor){
            (ca >> 7 & 0x1e) | (ca >> 11 & 1),
            (ca >> 3 & 0x1e) | (ca >> 7 & 1),
            (ca << 1 & 0x1c) | (ca >> 2 & 3),
            ca >> 11 & 0xe,
        };
    }
    if (cb & 0x8000) {
        e.b = (PvrtcColor){cb >> 10 & 0x1f, cb >> 5 & 0x1f, cb & 0x1f, 0xf};
    } else {
        e.b = (PvrtcColor){
            (cb >> 7 & 0x1e) | (cb >> 11 & 1),
            (cb >> 3 & 0x1e) | (cb >> 7 & 1),
            (cb << 1 & 0x1e) | (cb >> 3 & 1),
            cb >> 11 & 0xe,
        };
    }
    return e;
}

static PvrtcColor PvrtcExpand(PvrtcColor sum) {
    return (PvrtcColor){
        (sum.r >> 1) + (sum.r >> 6),
        (sum.g >> 1) + (sum.g >> 6),
        (sum.b >> 1) + (sum.b >> 6),
        sum.a + (sum.a >> 4),
    };
}

static int PvrtcBlend(int a, int b, int weight) {
    return (a * (8 - weight) + b * weight) / 8;
}

static void
PvrtcDecodeBlock(const unsigned char *block, const PvrtcEndpoints *neighbours[9], Color out[TEXTURE_BLOCK_PIXELS]) {
    bool punchthrough_mode = block[4] & 1;
    unsigned int modulation = ReadU32(block);
    for (int y = 0, i = 0; y < TEXTURE_BLOCK_SIZE; y++) {
        for (int x = 0; x < TEXTURE_BLOCK_SIZE; x++, i++, modulation >>= 2) {
            PvrtcColor a = {0}, b = {0};
            for (int ny = 0, n = 0; ny < 3; ny++) {
                for (int nx = 0; nx < 3; nx++, n++) {
                    int w = PVRTC_NEIGHBOUR_WEIGHTS[x][nx] * PVRTC_NEIGHBOUR_WEIGHTS[y][ny];
                    a.r += neighbours[n]->a.r * w, a.g += neighbours[n]->a.g * w;
                    a.b += neighbours[n]->a.b * w, a.a += neighbours[n]->a.a * w;
                    b.r += neighbours[n]->b.r * w, b.g += neighbours[n]->b.g * w;
                    b.b += neighbours[n]->b.b * w, b.a += neighbours[n]->b.a * w;
                }
            }
            a = PvrtcExpand(a);
            b = PvrtcExpand(b);
            int index = modulation & 3;
            int weight = punchthrough_mode ? PVRTC_PUNCHTHROUGH_WEIGHTS[index] : PVRTC_STANDARD_WEIGHTS[index];
            bool transparent = punchthrough_mode && index == PVRTC_PUNCHTHROUGH_INDEX;
            out[i] = (Color){
                (unsigned char)PvrtcBlend(a.r, b.r, weight),
                (unsigned char)PvrtcBlend(a.g, b.g, weight),
                (unsigned char)PvrtcBlend(a.b, b.b, weight),
                transparent ? 0 : (unsigned char)PvrtcBlend(a.a, b.a, weight),
            };
        }
    }
}

static int PvrtcDataSize(int width, int height) {
    return TextureBlocksAcross(width) * TextureBlocksAcross(height) * PVRTC_BLOCK_BYTES;
}

static bool PvrtcDecode(const unsigned char *data, Image *image) {
    int blocks_x = TextureBlocksAcross(image->width), blocks_y = TextureBlocksAcross(image->height);
    if ((blocks_x & (blocks_x - 1)) || (blocks_y & (blocks_y - 1))) return false;
    int min_dimension = blocks_x < blocks_y ? blocks_x : blocks_y;
    PvrtcEndpoints *endpoints = MemAlloc(sizeof(*endpoints) * blocks_x * blocks_y);
    for (int i = 0; i < blocks_x * blocks_y; i++) endpoints[i] = PvrtcReadEndpoints(data + i * PVRTC_BLOCK_BYTES);
    Color block[TEXTURE_BLOCK_PIXELS];
    for (int by = 0; by < blocks_y; by++) {
        int rows[3] = {(by + blocks_y - 1) % blocks_y, by, (by + 1) % blocks_y};
        for (int bx = 0; bx < blocks_x; bx++) {
            int columns[3] = {(bx + blocks_x - 1) % blocks_x, bx, (bx + 1) % blocks_x};
            const PvrtcEndpoints *neighbours[9];
            for (int ny = 0, n = 0; ny < 3; ny++)
                for (int nx = 0; nx < 3; nx++, n++)
                    neighbours[n] = &endpoints[PvrtcMortonIndex(columns[nx], rows[ny], min_dimension)];
            PvrtcDecodeBlock(data + PvrtcMortonIndex(bx, by, min_dimension) * PVRTC_BLOCK_BYTES, neighbours, block);
            TextureCopyBlock(image, bx, by, block);
        }
    }
    MemFree(endpoints);
    return true;
}

static Color Rgb565ToColor(unsigned int v) {
    return (Color){
        (unsigned char)((v >> 8 & 0xf8) | (v >> 13)),
        (unsigned char)((v >> 3 & 0xfc) | (v >> 9 & 3)),
        (unsigned char)((v << 3) | (v >> 2 & 7)),
        255,
    };
}

static void Dxt5DecodeColors(const unsigned char *block, Color out[TEXTURE_BLOCK_PIXELS]) {
    unsigned int q0 = ReadU16(block), q1 = ReadU16(block + 2);
    Color c0 = Rgb565ToColor(q0), c1 = Rgb565ToColor(q1);
    Color palette[4] = {c0, c1};
    if (q0 > q1) {
        palette[2] = (Color){(2 * c0.r + c1.r) / 3, (2 * c0.g + c1.g) / 3, (2 * c0.b + c1.b) / 3, 255};
        palette[3] = (Color){(c0.r + 2 * c1.r) / 3, (c0.g + 2 * c1.g) / 3, (c0.b + 2 * c1.b) / 3, 255};
    } else {
        palette[2] = (Color){(c0.r + c1.r) / 2, (c0.g + c1.g) / 2, (c0.b + c1.b) / 2, 255};
        palette[3] = (Color){0, 0, 0, 255};
    }
    unsigned int indices = ReadU32(block + 4);
    for (int i = 0; i < TEXTURE_BLOCK_PIXELS; i++, indices >>= 2) out[i] = palette[indices & 3];
}

static void Dxt5DecodeAlpha(const unsigned char *block, Color out[TEXTURE_BLOCK_PIXELS]) {
    int a[8] = {block[0], block[1]};
    if (a[0] > a[1]) {
        for (int i = 1; i <= 6; i++) a[i + 1] = (a[0] * (7 - i) + a[1] * i) / 7;
    } else {
        for (int i = 1; i <= 4; i++) a[i + 1] = (a[0] * (5 - i) + a[1] * i) / 5;
        a[6] = 0;
        a[7] = 255;
    }
    unsigned long long indices = 0;
    for (int i = 7; i >= 2; i--) indices = indices << 8 | block[i];
    for (int i = 0; i < TEXTURE_BLOCK_PIXELS; i++, indices >>= 3) out[i].a = (unsigned char)a[indices & 7];
}

static int Dxt5DataSize(int width, int height) {
    return TextureBlocksAcross(width) * TextureBlocksAcross(height) * DXT5_BLOCK_BYTES;
}

static bool Dxt5Decode(const unsigned char *data, Image *image) {
    Color block[TEXTURE_BLOCK_PIXELS];
    int blocks_x = TextureBlocksAcross(image->width), blocks_y = TextureBlocksAcross(image->height);
    for (int by = 0; by < blocks_y; by++) {
        for (int bx = 0; bx < blocks_x; bx++, data += DXT5_BLOCK_BYTES) {
            Dxt5DecodeColors(data + 8, block);
            Dxt5DecodeAlpha(data, block);
            TextureCopyBlock(image, bx, by, block);
        }
    }
    return true;
}

static Color UncompressedPixel(UnityTextureFormat format, const unsigned char *p) {
    switch (format) {
        case UNITY_TEXTURE_RGB24:
            return (Color){p[0], p[1], p[2], 255};
        case UNITY_TEXTURE_RGBA32:
            return (Color){p[0], p[1], p[2], p[3]};
        case UNITY_TEXTURE_ARGB32:
            return (Color){p[1], p[2], p[3], p[0]};
        default: {
            unsigned int v = ReadU16(p);
            return (Color){(v >> 12 & 0xf) * 17, (v >> 8 & 0xf) * 17, (v >> 4 & 0xf) * 17, (v & 0xf) * 17};
        }
    }
}

static int UncompressedPixelBytes(UnityTextureFormat format) {
    switch (format) {
        case UNITY_TEXTURE_RGB24:
            return 3;
        case UNITY_TEXTURE_RGBA32:
        case UNITY_TEXTURE_ARGB32:
            return 4;
        case UNITY_TEXTURE_RGBA4444:
            return 2;
        default:
            return 0;
    }
}

static int UnityTextureDataSize(UnityTextureFormat format, int width, int height) {
    switch (format) {
        case UNITY_TEXTURE_PVRTC_RGB4:
        case UNITY_TEXTURE_PVRTC_RGBA4:
            return PvrtcDataSize(width, height);
        case UNITY_TEXTURE_DXT5:
            return Dxt5DataSize(width, height);
        default:
            return width * height * UncompressedPixelBytes(format);
    }
}

static Image UnityDecodeTextureData(UnityTextureFormat format, int width, int height, const unsigned char *data) {
    Image image = {
        .data = MemAlloc(width * height * sizeof(Color)),
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    bool decoded = false;
    if (format == UNITY_TEXTURE_PVRTC_RGB4 || format == UNITY_TEXTURE_PVRTC_RGBA4) {
        decoded = PvrtcDecode(data, &image);
    } else if (format == UNITY_TEXTURE_DXT5) {
        decoded = Dxt5Decode(data, &image);
    } else if (UncompressedPixelBytes(format) > 0) {
        int stride = UncompressedPixelBytes(format);
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                TexturePutPixel(&image, x, y, UncompressedPixel(format, data + (y * width + x) * stride));
        decoded = true;
    }
    if (!decoded) {
        MemFree(image.data);
        return (Image){0};
    }
    return image;
}
