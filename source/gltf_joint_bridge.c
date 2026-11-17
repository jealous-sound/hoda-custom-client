#define GLB_MAGIC 0x46546C67u
#define GLB_JSON_CHUNK 0x4E4F534Au
#define GLB_BIN_CHUNK 0x004E4942u
#define GLB_HEADER_SIZE 12
#define GLB_CHUNK_HEADER_SIZE 8
#define GLTF_FLOAT_COMPONENT 5126
#define GLTF_CUBIC_SPLINE_KEY_PARTS 3
#define RAYLIB_GLTF_FRAME_SECONDS 0.017f
#define GLTF_BRIDGE_NODES_MAX 8

typedef struct {
    int bone;
    int anchor;
    int nodes[GLTF_BRIDGE_NODES_MAX];
    int node_count;
} GltfJointBridge;

typedef struct {
    cJSON *root;
    const unsigned char *bin;
    int bin_size;
    GltfJointBridge *bridges;
    int bridge_count;
} GltfSkinDoc;

static uint32_t GlbWord(const unsigned char *data, int offset) {
    uint32_t word;
    memcpy(&word, data + offset, sizeof(word));
    return word;
}

static bool GlbSplit(const unsigned char *data, int size, cJSON **root, const unsigned char **bin, int *bin_size) {
    *root = NULL;
    *bin = NULL;
    *bin_size = 0;
    if (!data || size < GLB_HEADER_SIZE + GLB_CHUNK_HEADER_SIZE || GlbWord(data, 0) != GLB_MAGIC) return false;
    for (int offset = GLB_HEADER_SIZE; offset + GLB_CHUNK_HEADER_SIZE <= size;) {
        int length = (int)GlbWord(data, offset);
        uint32_t type = GlbWord(data, offset + 4);
        const unsigned char *chunk = data + offset + GLB_CHUNK_HEADER_SIZE;
        if (length < 0 || length > size - offset - GLB_CHUNK_HEADER_SIZE) break;
        if (type == GLB_JSON_CHUNK && !*root) *root = cJSON_ParseWithLength((const char *)chunk, (size_t)length);
        if (type == GLB_BIN_CHUNK && !*bin) {
            *bin = chunk;
            *bin_size = length;
        }
        offset += GLB_CHUNK_HEADER_SIZE + length;
    }
    return *root != NULL;
}

static int GltfIndex(const cJSON *item, const char *key) {
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(item, key);
    return cJSON_IsNumber(value) ? value->valueint : -1;
}

static const cJSON *GltfAt(const cJSON *root, const char *list, int index) {
    return index < 0 ? NULL : cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root, list), index);
}

static void GltfFloats(const cJSON *array, float *out, int count) {
    for (int i = 0; i < count; i++) {
        const cJSON *value = cJSON_GetArrayItem(array, i);
        if (cJSON_IsNumber(value)) out[i] = (float)value->valuedouble;
    }
}

static Transform GltfNodeRest(const cJSON *node) {
    float t[3] = {0.0f, 0.0f, 0.0f}, r[4] = {0.0f, 0.0f, 0.0f, 1.0f}, s[3] = {1.0f, 1.0f, 1.0f};
    const cJSON *matrix = cJSON_GetObjectItemCaseSensitive(node, "matrix");
    if (cJSON_IsArray(matrix)) {
        float m[16] = {0};
        GltfFloats(matrix, m, 16);
        Matrix columns = {
            m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]
        };
        Transform rest;
        MatrixDecompose(columns, &rest.translation, &rest.rotation, &rest.scale);
        return rest;
    }
    GltfFloats(cJSON_GetObjectItemCaseSensitive(node, "translation"), t, 3);
    GltfFloats(cJSON_GetObjectItemCaseSensitive(node, "rotation"), r, 4);
    GltfFloats(cJSON_GetObjectItemCaseSensitive(node, "scale"), s, 3);
    return (Transform){{t[0], t[1], t[2]}, {r[0], r[1], r[2], r[3]}, {s[0], s[1], s[2]}};
}

static bool GltfAccessorRead(const GltfSkinDoc *doc, int accessor_index, int element, int width, float *out) {
    const cJSON *accessor = GltfAt(doc->root, "accessors", accessor_index);
    const cJSON *view = GltfAt(doc->root, "bufferViews", GltfIndex(accessor, "bufferView"));
    if (!view || !doc->bin || JsonInt(accessor, "componentType") != GLTF_FLOAT_COMPONENT) return false;
    if (element < 0 || element >= JsonInt(accessor, "count")) return false;
    int stride = JsonInt(view, "byteStride");
    if (stride <= 0) stride = width * (int)sizeof(float);
    long offset = (long)JsonInt(view, "byteOffset") + JsonInt(accessor, "byteOffset") + (long)element * stride;
    if (offset < 0 || offset + width * (long)sizeof(float) > doc->bin_size) return false;
    memcpy(out, doc->bin + offset, (size_t)width * sizeof(float));
    return true;
}

static int GltfKeyBefore(const GltfSkinDoc *doc, int input, int key_count, float time, float *blend) {
    float start = 0.0f, end = 0.0f;
    *blend = 0.0f;
    if (!GltfAccessorRead(doc, input, 0, 1, &start) || time <= start) return 0;
    for (int key = 0; key + 1 < key_count; key++) {
        if (!GltfAccessorRead(doc, input, key, 1, &start) || !GltfAccessorRead(doc, input, key + 1, 1, &end)) return 0;
        if (time >= end) continue;
        *blend = end > start ? (time - start) / (end - start) : 0.0f;
        return key;
    }
    return key_count - 1;
}

static void
GltfChannelSample(const GltfSkinDoc *doc, const cJSON *sampler, const char *path, float time, Transform *local) {
    int input = GltfIndex(sampler, "input"), output = GltfIndex(sampler, "output");
    int key_count = JsonInt(GltfAt(doc->root, "accessors", input), "count");
    if (key_count <= 0) return;
    const char *interpolation = JsonStr(sampler, "interpolation");
    bool step = strcmp(interpolation, "STEP") == 0;
    int parts = strcmp(interpolation, "CUBICSPLINE") == 0 ? GLTF_CUBIC_SPLINE_KEY_PARTS : 1;
    int value_part = parts / 2;
    int width = strcmp(path, "rotation") == 0 ? 4 : 3;
    float blend;
    int key = GltfKeyBefore(doc, input, key_count, time, &blend);
    int next = step || key + 1 >= key_count ? key : key + 1;
    float a[4] = {0}, b[4] = {0};
    if (!GltfAccessorRead(doc, output, key * parts + value_part, width, a) ||
        !GltfAccessorRead(doc, output, next * parts + value_part, width, b))
        return;
    if (width == 4) {
        local->rotation =
            QuaternionSlerp((Quaternion){a[0], a[1], a[2], a[3]}, (Quaternion){b[0], b[1], b[2], b[3]}, blend);
        return;
    }
    Vector3 value = Vector3Lerp((Vector3){a[0], a[1], a[2]}, (Vector3){b[0], b[1], b[2]}, blend);
    if (strcmp(path, "translation") == 0) local->translation = value;
    if (strcmp(path, "scale") == 0) local->scale = value;
}

static Transform GltfNodeLocalAt(const GltfSkinDoc *doc, int animation, int node, float time) {
    Transform local = GltfNodeRest(GltfAt(doc->root, "nodes", node));
    const cJSON *clip = GltfAt(doc->root, "animations", animation);
    const cJSON *samplers = cJSON_GetObjectItemCaseSensitive(clip, "samplers");
    const cJSON *channel = NULL;
    cJSON_ArrayForEach(channel, cJSON_GetObjectItemCaseSensitive(clip, "channels")) {
        const cJSON *target = cJSON_GetObjectItemCaseSensitive(channel, "target");
        if (GltfIndex(target, "node") != node) continue;
        const cJSON *sampler = cJSON_GetArrayItem(samplers, GltfIndex(channel, "sampler"));
        GltfChannelSample(doc, sampler, JsonStr(target, "path"), time, &local);
    }
    return local;
}

static Transform TransformCompose(Transform parent, Transform child) {
    return (Transform){
        Vector3Add(
            parent.translation,
            Vector3RotateByQuaternion(Vector3Multiply(parent.scale, child.translation), parent.rotation)
        ),
        QuaternionNormalize(QuaternionMultiply(parent.rotation, child.rotation)),
        Vector3Multiply(parent.scale, child.scale),
    };
}

static Transform PoseChildOf(Transform parent, Transform local) {
    return (Transform){
        Vector3Add(parent.translation, Vector3RotateByQuaternion(local.translation, parent.rotation)),
        QuaternionMultiply(parent.rotation, local.rotation),
        Vector3Multiply(parent.scale, local.scale),
    };
}

static Transform PoseLocalTo(Transform parent, Transform pose) {
    Quaternion to_parent = QuaternionInvert(parent.rotation);
    return (Transform){
        Vector3RotateByQuaternion(Vector3Subtract(pose.translation, parent.translation), to_parent),
        QuaternionMultiply(to_parent, pose.rotation),
        Vector3Divide(pose.scale, parent.scale),
    };
}

static void GltfBridgesFind(GltfSkinDoc *doc) {
    const cJSON *nodes = cJSON_GetObjectItemCaseSensitive(doc->root, "nodes");
    const cJSON *joints = cJSON_GetObjectItemCaseSensitive(GltfAt(doc->root, "skins", 0), "joints");
    int node_count = cJSON_GetArraySize(nodes), joint_count = cJSON_GetArraySize(joints);
    if (node_count <= 0 || joint_count <= 0) return;
    int *parent = MemAlloc(node_count * sizeof(int));
    int *bone_of = MemAlloc(node_count * sizeof(int));
    for (int n = 0; n < node_count; n++) parent[n] = bone_of[n] = -1;
    for (int n = 0; n < node_count; n++) {
        const cJSON *child = NULL;
        cJSON_ArrayForEach(child, cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(nodes, n), "children")) {
            if (cJSON_IsNumber(child) && child->valueint >= 0 && child->valueint < node_count)
                parent[child->valueint] = n;
        }
    }
    for (int b = 0; b < joint_count; b++) {
        int node = cJSON_GetArrayItem(joints, b)->valueint;
        if (node >= 0 && node < node_count) bone_of[node] = b;
    }
    doc->bridges = MemAlloc(joint_count * sizeof(GltfJointBridge));
    for (int b = 0; b < joint_count; b++) {
        int node = cJSON_GetArrayItem(joints, b)->valueint;
        if (node < 0 || node >= node_count) continue;
        GltfJointBridge bridge = {.bone = b, .anchor = -1};
        int above = parent[node];
        while (above >= 0 && bone_of[above] < 0 && bridge.node_count < GLTF_BRIDGE_NODES_MAX) {
            bridge.nodes[bridge.node_count++] = above;
            above = parent[above];
        }
        if (bridge.node_count == 0 || above < 0 || bone_of[above] < 0) continue;
        bridge.anchor = bone_of[above];
        for (int i = 0; i < bridge.node_count / 2; i++) {
            int swap = bridge.nodes[i];
            bridge.nodes[i] = bridge.nodes[bridge.node_count - 1 - i];
            bridge.nodes[bridge.node_count - 1 - i] = swap;
        }
        doc->bridges[doc->bridge_count++] = bridge;
    }
    MemFree(parent);
    MemFree(bone_of);
}

static bool GltfSkinDocParse(GltfSkinDoc *doc, const unsigned char *data, int size) {
    *doc = (GltfSkinDoc){0};
    if (!GlbSplit(data, size, &doc->root, &doc->bin, &doc->bin_size)) return false;
    GltfBridgesFind(doc);
    return true;
}

static void GltfSkinDocFree(GltfSkinDoc *doc) {
    cJSON_Delete(doc->root);
    MemFree(doc->bridges);
    *doc = (GltfSkinDoc){0};
}

static void BonesBridgeAnchors(const GltfSkinDoc *doc, BoneInfo *bones, int bone_count) {
    for (int i = 0; i < doc->bridge_count; i++) {
        const GltfJointBridge *bridge = &doc->bridges[i];
        if (bridge->bone < bone_count && bridge->anchor < bone_count) bones[bridge->bone].parent = bridge->anchor;
    }
}

static void PoseRecompose(Transform *pose, const BoneInfo *bones, int bone_count, const Transform *locals) {
    bool *done = MemAlloc(bone_count * sizeof(bool));
    for (int placed = 0, pass = 0; placed < bone_count && pass < bone_count; pass++) {
        for (int b = 0; b < bone_count; b++) {
            int parent = bones[b].parent;
            if (done[b] || (parent >= 0 && !done[parent])) continue;
            pose[b] = parent < 0 ? locals[b] : PoseChildOf(pose[parent], locals[b]);
            done[b] = true;
            placed++;
        }
    }
    MemFree(done);
}

static void ClipBridgeJoints(const GltfSkinDoc *doc, int animation, ModelAnimation clip) {
    if (doc->bridge_count == 0 || clip.frameCount <= 0) return;
    Transform *locals = MemAlloc(clip.boneCount * sizeof(Transform));
    BoneInfo *bridged = MemAlloc(clip.boneCount * sizeof(BoneInfo));
    memcpy(bridged, clip.bones, clip.boneCount * sizeof(BoneInfo));
    BonesBridgeAnchors(doc, bridged, clip.boneCount);
    for (int frame = 0; frame < clip.frameCount; frame++) {
        Transform *pose = clip.framePoses[frame];
        for (int b = 0; b < clip.boneCount; b++) {
            int parent = clip.bones[b].parent;
            locals[b] = parent < 0 ? pose[b] : PoseLocalTo(pose[parent], pose[b]);
        }
        float time = (float)frame * RAYLIB_GLTF_FRAME_SECONDS;
        for (int i = 0; i < doc->bridge_count; i++) {
            const GltfJointBridge *bridge = &doc->bridges[i];
            if (bridge->bone >= clip.boneCount || bridge->anchor >= clip.boneCount) continue;
            Transform span = {Vector3Zero(), QuaternionIdentity(), Vector3One()};
            for (int n = 0; n < bridge->node_count; n++)
                span = TransformCompose(span, GltfNodeLocalAt(doc, animation, bridge->nodes[n], time));
            locals[bridge->bone] = TransformCompose(span, locals[bridge->bone]);
        }
        PoseRecompose(pose, bridged, clip.boneCount, locals);
    }
    BonesBridgeAnchors(doc, clip.bones, clip.boneCount);
    MemFree(bridged);
    MemFree(locals);
}

static void ClipsBridgeJoints(const GltfSkinDoc *doc, ModelAnimation *clips, int clip_count) {
    for (int i = 0; i < clip_count; i++) ClipBridgeJoints(doc, i, clips[i]);
}

static ModelAnimation *
SkinnedClipsLoad(const char *path, BoneInfo *model_bones, int model_bone_count, int *clip_count) {
    ModelAnimation *clips = LoadModelAnimations(path, clip_count);
    int size = 0;
    unsigned char *data = LoadFileData(path, &size);
    GltfSkinDoc doc;
    if (GltfSkinDocParse(&doc, data, size)) {
        if (clips) ClipsBridgeJoints(&doc, clips, *clip_count);
        if (model_bones) BonesBridgeAnchors(&doc, model_bones, model_bone_count);
        GltfSkinDocFree(&doc);
    }
    UnloadFileData(data);
    return clips;
}
