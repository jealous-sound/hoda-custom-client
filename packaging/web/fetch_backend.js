addToLibrary({
  $hodFetchedFiles: {},

  _wasmfs_create_fetch_backend_js__deps: ["_wasmfs_fetch_get_file_url", "$hodFetchedFiles"],
  _wasmfs_create_fetch_backend_js: async function (backend) {
    function singleSlashUrl(file) {
      var url = new URL(UTF8ToString(__wasmfs_fetch_get_file_url(file)), self.location.origin);
      url.pathname = url.pathname.replace(/\/{2,}/g, "/");
      return url.href;
    }

    async function download(file) {
      var response = await fetch(singleSlashUrl(file));
      if (!response.ok) throw response;
      return new Uint8Array(await response.arrayBuffer());
    }

    function wholeFile(file) {
      if (!hodFetchedFiles[file]) {
        hodFetchedFiles[file] = download(file).catch(function (failure) {
          delete hodFetchedFiles[file];
          throw failure;
        });
      }
      return hodFetchedFiles[file];
    }

    wasmFS$backends[backend] = {
      allocFile: async (file) => {},
      freeFile: async (file) => {
        delete hodFetchedFiles[file];
      },
      write: async (file, buffer, length, offset) => -{{{ cDefs.EROFS }}},
      read: async (file, buffer, length, offset) => {
        if (offset < 0 || length <= 0) return 0;
        var bytes;
        try {
          bytes = await wholeFile(file);
        } catch (failure) {
          return failure.status === 404 ? -{{{ cDefs.ENOENT }}} : -{{{ cDefs.EBADF }}};
        }
        var end = Math.min(bytes.length, offset + length);
        if (end <= offset) return 0;
        HEAPU8.set(bytes.subarray(offset, end), buffer);
        return end - offset;
      },
      getSize: async (file) => {
        try {
          return (await wholeFile(file)).length;
        } catch (failure) {
          return 0;
        }
      },
    };
  },
});
