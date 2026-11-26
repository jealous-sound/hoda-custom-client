function hodWithSingleSlashes(resource) {
  if (typeof resource !== "string" || !/^https?:\/\//i.test(resource)) return resource;
  var url = new URL(resource);
  url.pathname = url.pathname.replace(/\/{2,}/g, "/");
  return url.href;
}

function hodFetchWithSingleSlashes(fetchAsAsked) {
  return function (resource, options) {
    return fetchAsAsked(hodWithSingleSlashes(resource), options);
  };
}

if (typeof globalThis.fetch === "function") globalThis.fetch = hodFetchWithSingleSlashes(globalThis.fetch);
