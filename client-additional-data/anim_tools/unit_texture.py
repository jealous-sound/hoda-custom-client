from pathlib import PurePosixPath

from PIL import Image


UNLIT_TEXTURE_SHADER = ("unity_builtin_extra", 10752)
ADDITIVE_COLOR_SHADER = ("9be333ebb9f7d2249a99c06fe10d85cf", 1)
ADDITIVE_COLOR_GAIN = 2.0
SHADER_DEFAULT_TINT = (1.0, 1.0, 1.0)
FULL_STRENGTH = 1.0
OPAQUE_ALPHA = 255
CHANNEL_MAX = 255
UNLIT_TEXTURE_NOTE = "opaque Unlit/Texture material, _MainTex alpha dropped"


def shader_reference(external_path, path_id):
    return PurePosixPath(external_path.replace("\\", "/").lower()).name, path_id


def shader_ignores_alpha(external_path, path_id):
    return shader_reference(external_path, path_id) == UNLIT_TEXTURE_SHADER


def shader_adds_tinted_light(external_path, path_id):
    return shader_reference(external_path, path_id) == ADDITIVE_COLOR_SHADER


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


def material_color(material_tt, name):
    for row in material_tt.get("m_SavedProperties", {}).get("m_Colors", []):
        key, value = row if isinstance(row, (list, tuple)) else (row.get("first"), row.get("second"))
        if isinstance(key, dict):
            key = key.get("name", "")
        if key == name:
            return value["r"], value["g"], value["b"]
    return None


def opaque_additive_gain(tint):
    gain = tuple(ADDITIVE_COLOR_GAIN * channel for channel in tint)
    brightest = min(max(gain), FULL_STRENGTH) or FULL_STRENGTH
    return tuple(channel / brightest for channel in gain)


def tinted(image, gain):
    bands = [
        band.point(lambda value, factor=factor: min(CHANNEL_MAX, round(value * factor)))
        for band, factor in zip(image.convert("RGB").split(), gain)
    ]
    return opaque(Image.merge("RGB", bands))


def runtime_main_texture(image, material_obj, material_tt):
    shader = material_shader(material_obj, material_tt)
    if shader_ignores_alpha(*shader):
        return opaque(image), UNLIT_TEXTURE_NOTE
    if shader_adds_tinted_light(*shader):
        gain = opaque_additive_gain(material_color(material_tt, "_TintColor") or SHADER_DEFAULT_TINT)
        rounded = ", ".join(f"{channel:.3f}" for channel in gain)
        return tinted(image, gain), f"additive _TintColor baked opaque as gain ({rounded})"
    return image, None
