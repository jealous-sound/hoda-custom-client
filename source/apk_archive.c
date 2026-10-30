#define ZIP_END_OF_DIRECTORY_SIGNATURE 0x06054b50u
#define ZIP_DIRECTORY_ENTRY_SIGNATURE 0x02014b50u
#define ZIP_LOCAL_HEADER_SIGNATURE 0x04034b50u
#define ZIP_END_OF_DIRECTORY_SIZE 22
#define ZIP_MAX_COMMENT_SIZE 0xffff
#define ZIP_DIRECTORY_ENTRY_SIZE 46
#define ZIP_LOCAL_HEADER_SIZE 30
#define ZIP_METHOD_STORED 0
#define ZIP_METHOD_DEFLATE 8

typedef struct {
    char *name;
    unsigned int method;
    unsigned int compressed_size;
    unsigned int size;
    unsigned int local_header_offset;
} ApkEntry;

typedef struct {
    FILE *file;
    ApkEntry *entries;
    int count;
} ApkArchive;

static bool ApkReadAt(FILE *file, long offset, void *out, size_t size) {
    return fseek(file, offset, SEEK_SET) == 0 && fread(out, 1, size, file) == size;
}

static long ApkFindEndOfDirectory(FILE *file, unsigned char *tail, long *tail_start) {
    if (fseek(file, 0, SEEK_END) != 0) return -1;
    long file_size = ftell(file);
    long tail_size = file_size < ZIP_END_OF_DIRECTORY_SIZE + ZIP_MAX_COMMENT_SIZE
                         ? file_size
                         : ZIP_END_OF_DIRECTORY_SIZE + ZIP_MAX_COMMENT_SIZE;
    *tail_start = file_size - tail_size;
    if (!ApkReadAt(file, *tail_start, tail, (size_t)tail_size)) return -1;
    for (long i = tail_size - ZIP_END_OF_DIRECTORY_SIZE; i >= 0; i--)
        if (ReadU32(tail + i) == ZIP_END_OF_DIRECTORY_SIGNATURE) return i;
    return -1;
}

static void ApkClose(ApkArchive *apk) {
    for (int i = 0; i < apk->count; i++) MemFree(apk->entries[i].name);
    MemFree(apk->entries);
    if (apk->file) fclose(apk->file);
    *apk = (ApkArchive){0};
}

static bool ApkOpen(ApkArchive *apk, const char *path) {
    *apk = (ApkArchive){.file = fopen(path, "rb")};
    if (!apk->file) return false;
    unsigned char *tail = MemAlloc(ZIP_END_OF_DIRECTORY_SIZE + ZIP_MAX_COMMENT_SIZE);
    long tail_start = 0;
    long end = ApkFindEndOfDirectory(apk->file, tail, &tail_start);
    unsigned int entry_count = end >= 0 ? ReadU16(tail + end + 10) : 0;
    unsigned int directory_size = end >= 0 ? ReadU32(tail + end + 12) : 0;
    unsigned int directory_offset = end >= 0 ? ReadU32(tail + end + 16) : 0;
    MemFree(tail);
    if (end < 0) {
        ApkClose(apk);
        return false;
    }
    unsigned char *directory = MemAlloc(directory_size);
    bool ok = ApkReadAt(apk->file, directory_offset, directory, directory_size);
    apk->entries = MemAlloc(sizeof(*apk->entries) * (entry_count > 0 ? entry_count : 1));
    for (unsigned int i = 0, p = 0; ok && i < entry_count; i++) {
        ok = p + ZIP_DIRECTORY_ENTRY_SIZE <= directory_size && ReadU32(directory + p) == ZIP_DIRECTORY_ENTRY_SIGNATURE;
        if (!ok) break;
        const unsigned char *h = directory + p;
        unsigned int name_size = ReadU16(h + 28);
        unsigned int record_size = ZIP_DIRECTORY_ENTRY_SIZE + name_size + ReadU16(h + 30) + ReadU16(h + 32);
        ok = p + record_size <= directory_size;
        if (!ok) break;
        ApkEntry *entry = &apk->entries[apk->count++];
        *entry = (ApkEntry){
            .name = MemAlloc(name_size + 1),
            .method = ReadU16(h + 10),
            .compressed_size = ReadU32(h + 20),
            .size = ReadU32(h + 24),
            .local_header_offset = ReadU32(h + 42),
        };
        memcpy(entry->name, h + ZIP_DIRECTORY_ENTRY_SIZE, name_size);
        p += record_size;
    }
    MemFree(directory);
    if (!ok) ApkClose(apk);
    return ok;
}

static unsigned char *ApkReadEntry(const ApkArchive *apk, const ApkEntry *entry, unsigned int *size) {
    unsigned char header[ZIP_LOCAL_HEADER_SIZE];
    if (!ApkReadAt(apk->file, entry->local_header_offset, header, sizeof(header)) ||
        ReadU32(header) != ZIP_LOCAL_HEADER_SIGNATURE)
        return NULL;
    long data_offset =
        (long)entry->local_header_offset + ZIP_LOCAL_HEADER_SIZE + ReadU16(header + 26) + ReadU16(header + 28);
    unsigned char *stored = MemAlloc(entry->compressed_size > 0 ? entry->compressed_size : 1);
    if (!ApkReadAt(apk->file, data_offset, stored, entry->compressed_size)) {
        MemFree(stored);
        return NULL;
    }
    if (entry->method == ZIP_METHOD_STORED) {
        *size = entry->compressed_size;
        return stored;
    }
    unsigned char *inflated = NULL;
    int inflated_size = 0;
    if (entry->method == ZIP_METHOD_DEFLATE)
        inflated = DecompressData(stored, (int)entry->compressed_size, &inflated_size);
    MemFree(stored);
    if (!inflated || (unsigned int)inflated_size != entry->size) {
        MemFree(inflated);
        return NULL;
    }
    *size = (unsigned int)inflated_size;
    return inflated;
}
