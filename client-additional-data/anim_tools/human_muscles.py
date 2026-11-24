CHEST_SLOT = 8
SPINE_MUSCLE_ATTRS = (42, 43, 44)
CHEST_MUSCLE_ATTRS = (45, 46, 47)


def chest_bend_on_spine(dof):
    folded = dict(dof)
    for spine, chest in zip(SPINE_MUSCLE_ATTRS, CHEST_MUSCLE_ATTRS):
        if chest in dof:
            folded[spine] = dof.get(spine, 0.0) + dof[chest]
    return folded


def muscles_for_rig(dof, bone_node):
    if bone_node[CHEST_SLOT] < 0:
        return chest_bend_on_spine(dof)
    return dof
