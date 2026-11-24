from collections import namedtuple


NO_PARENT = 0
UNIT_ROOT_PREFIX = 'unit.'
LOC_MARKER_SUFFIX = '_loc_marker'
FBX_WRAPPER_SUFFIX = '_fbx'
CM_FOLD_SCALE_CEILING = 0.1
CM_UNIT_SCALE = 0.01
CM_PER_METER = 100.0
EPSILON = 1e-6
METER_RIG_NOTE = 'meter-rig _fbx wrapper (no cm fold)'

PrefabNode = namedtuple('PrefabNode', 'name scale y parent children')


def children(nodes, pid):
    return [child for child in nodes[pid].children if child in nodes]


def subtree(nodes, pid):
    stack, found = [pid], []
    while stack:
        current = stack.pop()
        found.append(current)
        stack.extend(children(nodes, current))
    return found


def carries_skin(nodes, pid, bones):
    return any(node in bones for node in subtree(nodes, pid))


def unit_root(nodes):
    return next((pid for pid, node in nodes.items()
                 if node.parent == NO_PARENT and node.name.startswith(UNIT_ROOT_PREFIX)), None)


def cm_fold_path(nodes, root, bones):
    frontier = [(child, [child]) for child in children(nodes, root)]
    while frontier:
        folds = [(pid, path) for pid, path in frontier
                 if nodes[pid].scale < CM_FOLD_SCALE_CEILING and children(nodes, pid)
                 and carries_skin(nodes, pid, bones)]
        if folds:
            return max(folds, key=lambda fold: len(subtree(nodes, fold[0])))[1]
        frontier = [(child, path + [child]) for pid, path in frontier for child in children(nodes, pid)]
    return None


def meter_rig_scaffolding(nodes, root):
    kids = [child for child in children(nodes, root) if not nodes[child].name.endswith(LOC_MARKER_SUFFIX)]
    wrappers = [child for child in kids if nodes[child].name.endswith(FBX_WRAPPER_SUFFIX)]
    carrier = wrappers[0] if wrappers else max(kids, key=lambda kid: len(subtree(nodes, kid)), default=None)
    if carrier is None:
        return None, None, 'no cm-folded skeleton root under unit root'
    return round(nodes[carrier].scale, 6), round(nodes[carrier].y, 6), METER_RIG_NOTE


def display_scaffolding(nodes, bones):
    root = unit_root(nodes)
    if root is None:
        return None, None, 'no unit.* root transform'
    path = cm_fold_path(nodes, root, bones)
    if path is None:
        return meter_rig_scaffolding(nodes, root)

    scale = 1.0
    for pid in path:
        scale *= nodes[pid].scale
    y = sum(nodes[pid].y for pid in path[:-1])
    if abs(y) < EPSILON:
        siblings = [child for pid in [root] + path[:-1] for child in children(nodes, pid)
                    if child not in path and abs(nodes[child].scale - CM_UNIT_SCALE) < EPSILON
                    and not children(nodes, child)]
        if siblings:
            y = nodes[siblings[0]].y
    note = '' if nodes[path[0]].name.endswith(FBX_WRAPPER_SUFFIX) else 'no _fbx wrapper'
    return round(scale * CM_PER_METER, 6), round(y, 6), note
