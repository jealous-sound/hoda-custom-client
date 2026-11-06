#define APP_BUNDLE_EXECUTABLE_SUFFIX ".app/Contents/MacOS/"

static void AppBundleEnterParentDirectory(void) {
    char directory[4096];
    snprintf(directory, sizeof(directory), "%s", GetApplicationDirectory());
    size_t length = strlen(directory);
    size_t suffix_length = strlen(APP_BUNDLE_EXECUTABLE_SUFFIX);
    if (length < suffix_length || strcmp(directory + length - suffix_length, APP_BUNDLE_EXECUTABLE_SUFFIX) != 0) return;
    char *bundle_start = directory + length - suffix_length;
    while (bundle_start > directory && bundle_start[-1] != '/') bundle_start--;
    *bundle_start = '\0';
    if (directory[0]) ChangeDirectory(directory);
}
