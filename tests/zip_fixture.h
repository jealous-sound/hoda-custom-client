#ifndef HOD_TEST_ZIP_FIXTURE_H
#define HOD_TEST_ZIP_FIXTURE_H

#include <assert.h>
#include <stdarg.h>

#define TEST_PATH_CAP 1024
#define TEST_MAX_ENTRIES 8
#define TEST_ZIP_VERSION 20

typedef struct {
    const char *name;
    const char *text;
    bool deflate;
} TestEntry;

static void TestFormat(char *out, size_t capacity, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(out, capacity, format, arguments);
    va_end(arguments);
    assert(length >= 0 && (size_t)length < capacity);
}

static void TestPutU16(FILE *file, unsigned int value) {
    fputc((int)(value & 0xffu), file);
    fputc((int)((value >> 8) & 0xffu), file);
}

static void TestPutU32(FILE *file, unsigned int value) {
    TestPutU16(file, value & 0xffffu);
    TestPutU16(file, value >> 16);
}

static void TestWriteZip(const char *path, const TestEntry *entries, int count) {
    FILE *file = fopen(path, "wb");
    assert(file);
    unsigned int offsets[TEST_MAX_ENTRIES], stored_sizes[TEST_MAX_ENTRIES], crcs[TEST_MAX_ENTRIES];
    unsigned int methods[TEST_MAX_ENTRIES];
    for (int i = 0; i < count; i++) {
        unsigned char *raw = (unsigned char *)entries[i].text;
        int raw_size = (int)strlen(entries[i].text);
        int stored_size = raw_size;
        unsigned char *stored = entries[i].deflate ? CompressData(raw, raw_size, &stored_size) : raw;
        methods[i] = entries[i].deflate ? ZIP_METHOD_DEFLATE : ZIP_METHOD_STORED;
        offsets[i] = (unsigned int)ftell(file);
        stored_sizes[i] = (unsigned int)stored_size;
        crcs[i] = ComputeCRC32(raw, raw_size);
        TestPutU32(file, ZIP_LOCAL_HEADER_SIGNATURE);
        TestPutU16(file, TEST_ZIP_VERSION);
        TestPutU16(file, 0);
        TestPutU16(file, methods[i]);
        TestPutU32(file, 0);
        TestPutU32(file, crcs[i]);
        TestPutU32(file, stored_sizes[i]);
        TestPutU32(file, (unsigned int)raw_size);
        TestPutU16(file, (unsigned int)strlen(entries[i].name));
        TestPutU16(file, 0);
        fwrite(entries[i].name, 1, strlen(entries[i].name), file);
        fwrite(stored, 1, (size_t)stored_size, file);
        if (entries[i].deflate) MemFree(stored);
    }
    unsigned int directory_start = (unsigned int)ftell(file);
    for (int i = 0; i < count; i++) {
        TestPutU32(file, ZIP_DIRECTORY_ENTRY_SIGNATURE);
        TestPutU16(file, TEST_ZIP_VERSION);
        TestPutU16(file, TEST_ZIP_VERSION);
        TestPutU16(file, 0);
        TestPutU16(file, methods[i]);
        TestPutU32(file, 0);
        TestPutU32(file, crcs[i]);
        TestPutU32(file, stored_sizes[i]);
        TestPutU32(file, (unsigned int)strlen(entries[i].text));
        TestPutU16(file, (unsigned int)strlen(entries[i].name));
        TestPutU16(file, 0);
        TestPutU16(file, 0);
        TestPutU16(file, 0);
        TestPutU16(file, 0);
        TestPutU32(file, 0);
        TestPutU32(file, offsets[i]);
        fwrite(entries[i].name, 1, strlen(entries[i].name), file);
    }
    unsigned int directory_size = (unsigned int)ftell(file) - directory_start;
    TestPutU32(file, ZIP_END_OF_DIRECTORY_SIGNATURE);
    TestPutU16(file, 0);
    TestPutU16(file, 0);
    TestPutU16(file, (unsigned int)count);
    TestPutU16(file, (unsigned int)count);
    TestPutU32(file, directory_size);
    TestPutU32(file, directory_start);
    TestPutU16(file, 0);
    fclose(file);
}

#endif
