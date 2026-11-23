from pathlib import PurePosixPath


UNLIT_TEXTURE_SHADER = ("unity_builtin_extra", 10752)
OPAQUE_ALPHA = 255


def shader_reference(external_path, path_id):
    return PurePosixPath(external_path.replace("\\", "/").lower()).name, path_id


def shader_ignores_alpha(external_path, path_id):
    return shader_reference(external_path, path_id) == UNLIT_TEXTURE_SHADER


def opaque(image):
    result = image.convert("RGB").convert("RGBA")
    result.putalpha(OPAQUE_ALPHA)
    return result


def material_shader(material_obj, material_tt):
    shader = material_tt.get("m_Shader", {})
    file_id = shader.get("m_FileID", 0)
    externals = material_obj.assets_file.externals
    path = externals[file_id - 1].path if 0 < file_id <= len(externals) else ""
    return path, shader.get("m_PathID", 0)


def runtime_main_texture(image, material_obj, material_tt):
    if shader_ignores_alpha(*material_shader(material_obj, material_tt)):
        return opaque(image), True
    return image, False
