#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>
#include <unistd.h>

#define TEST_GAP 16
#define TEST_FILE_BYTES 64
#define TEST_TEMPORARY_TEMPLATE "hod_resource_read_XXXXXX"
#define TEST_MEGABYTE (1024u * 1024)
#define TEST_BUNDLE_READS 6
#define TEST_PATH_CAP 1024

static char test_path[TEST_PATH_CAP];

static void TestRangesWithinTheGapShareOneSpan(void) {
    PlatformFileRange ranges[] = {
        {.offset = 40, .size = 8},
        {.offset = 0, .size = 10},
        {.offset = 20, .size = 4},
        {.offset = 100, .size = 1},
    };
    PlatformByteSpan spans[4];
    assert(PlatformCoalesceRanges(ranges, 4, TEST_GAP, spans) == 2);
    assert(spans[0].start == 0 && spans[0].end == 48);
    assert(spans[1].start == 100 && spans[1].end == 101);
}

static void TestOverlappingAndRepeatedRangesMerge(void) {
    PlatformFileRange ranges[] = {{.offset = 0, .size = 10}, {.offset = 5, .size = 10}, {.offset = 0, .size = 10}};
    PlatformByteSpan spans[3];
    assert(PlatformCoalesceRanges(ranges, 3, 0, spans) == 1);
    assert(spans[0].start == 0 && spans[0].end == 15);
}

static void TestOnlyTouchingRangesMergeWithoutAGap(void) {
    PlatformFileRange ranges[] = {{.offset = 10, .size = 5}, {.offset = 0, .size = 10}, {.offset = 16, .size = 1}};
    PlatformByteSpan spans[3];
    assert(PlatformCoalesceRanges(ranges, 3, 0, spans) == 2);
    assert(spans[0].start == 0 && spans[0].end == 15);
    assert(spans[1].start == 16 && spans[1].end == 17);
}

static void TestLocalReadsMarkEachRange(void) {
    unsigned char head[2], middle[4], past_end[8];
    PlatformFileRange ranges[] = {
        {.offset = 8, .size = sizeof(middle), .out = middle},
        {.offset = TEST_FILE_BYTES - 4, .size = sizeof(past_end), .out = past_end},
        {.offset = 0, .size = sizeof(head), .out = head},
    };
    assert(!PlatformReadFileRanges(test_path, ranges, 3));
    assert(ranges[0].read && middle[0] == 8 && middle[3] == 11);
    assert(!ranges[1].read);
    assert(ranges[2].read && head[0] == 0 && head[1] == 1);
}

static void TestAMissingFileReadsNothing(void) {
    unsigned char byte;
    PlatformFileRange range = {.offset = 0, .size = 1, .out = &byte, .read = true};
    assert(!PlatformReadFileRanges("no/such/bundle", &range, 1));
    assert(!range.read);
}

static void TestImageReadsGroupByBundleInOffsetOrder(void) {
    ResourceImageRead reads[] = {
        {.ref = 0, .container = 2, .offset = 500, .size = 10},
        {.ref = 1, .container = RESOURCE_UNGROUPED},
        {.ref = 2, .container = 2, .offset = 100, .size = 10},
        {.ref = 3, .container = 0, .offset = 0, .size = 5},
        {.ref = 4, .container = RESOURCE_UNGROUPED},
    };
    ResourceReadGroup groups[5];
    assert(ResourceGroupImageReads(reads, 5, groups) == 4);
    assert(groups[0].count == 2 && groups[0].bytes == 20);
    assert(reads[groups[0].first].ref == 2 && reads[groups[0].first + 1].ref == 0);
    assert(groups[1].count == 1 && reads[groups[1].first].ref == 3);
    for (int i = 2; i < 4; i++) {
        assert(groups[i].count == 1 && groups[i].bytes == 0);
        assert(reads[groups[i].first].container == RESOURCE_UNGROUPED);
    }
}

static void TestTheBundleNeedingMostBytesIsReadFirst(void) {
    ResourceImageRead reads[TEST_BUNDLE_READS];
    for (int i = 0; i < TEST_BUNDLE_READS; i++)
        reads[i] = (ResourceImageRead){
            .ref = i,
            .container = i % 2,
            .offset = (unsigned int)(TEST_BUNDLE_READS - i) * TEST_MEGABYTE,
            .size = TEST_MEGABYTE * (unsigned int)(1 + i % 2),
        };
    ResourceReadGroup groups[TEST_BUNDLE_READS];
    assert(ResourceGroupImageReads(reads, TEST_BUNDLE_READS, groups) == 2);
    assert(reads[groups[0].first].container == 1 && groups[0].count == TEST_BUNDLE_READS / 2);
    assert(groups[0].bytes > groups[1].bytes);
    for (int i = 1; i < groups[0].count; i++)
        assert(reads[groups[0].first + i].offset > reads[groups[0].first + i - 1].offset);
}

int main(void) {
    SetTraceLogLevel(LOG_ERROR);
    const char *temporary = getenv("TMPDIR");
    snprintf(
        test_path, sizeof(test_path), "%s/%s", temporary && temporary[0] ? temporary : "/tmp", TEST_TEMPORARY_TEMPLATE
    );
    int descriptor = mkstemp(test_path);
    assert(descriptor >= 0);
    unsigned char bytes[TEST_FILE_BYTES];
    for (int i = 0; i < TEST_FILE_BYTES; i++) bytes[i] = (unsigned char)i;
    assert(write(descriptor, bytes, sizeof(bytes)) == (ssize_t)sizeof(bytes));
    close(descriptor);

    TestRangesWithinTheGapShareOneSpan();
    TestOverlappingAndRepeatedRangesMerge();
    TestOnlyTouchingRangesMergeWithoutAGap();
    TestLocalReadsMarkEachRange();
    TestAMissingFileReadsNothing();
    TestImageReadsGroupByBundleInOffsetOrder();
    TestTheBundleNeedingMostBytesIsReadFirst();

    remove(test_path);
    printf("resource read tests passed\n");
    return 0;
}
