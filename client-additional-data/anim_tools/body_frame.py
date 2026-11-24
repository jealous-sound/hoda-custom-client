ROOT_T_FIRST = 7
ROOT_Q_FIRST = 10
FOOT_GOAL_T_FIRSTS = (14, 21)
IDENTITY = (0.0, 0.0, 0.0, 1.0)


def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (
        aw*bx + ax*bw + ay*bz - az*by,
        aw*by - ax*bz + ay*bw + az*bx,
        aw*bz + ax*by - ay*bx + az*bw,
        aw*bw - ax*bx - ay*by - az*bz,
    )


def quat_conj(q):
    return (-q[0], -q[1], -q[2], q[3])


def quat_normalize(q):
    length = sum(c*c for c in q) ** 0.5 or 1.0
    return tuple(c/length for c in q)


def quat_rotate(q, v):
    return quat_mul(quat_mul(q, (v[0], v[1], v[2], 0.0)), quat_conj(q))[:3]


def clip_vector(dof, first, unit):
    return tuple(dof.get(first + i, 0.0) * unit for i in range(3))


def clip_root_rotation(dof):
    return quat_normalize(tuple(dof.get(ROOT_Q_FIRST + i, IDENTITY[i]) for i in range(4)))


def planted_feet_centre(dof, unit):
    root_t = clip_vector(dof, ROOT_T_FIRST, unit)
    root_q = clip_root_rotation(dof)
    goals = [
        tuple(r + g for r, g in zip(root_t, quat_rotate(root_q, clip_vector(dof, first, unit))))
        for first in FOOT_GOAL_T_FIRSTS
        if all(first + i in dof for i in range(3))
    ]
    if not goals:
        return root_t
    return tuple(sum(goal[i] for goal in goals) / len(goals) for i in range(3))


def tpose_body_turn(dof, tpose_body_q):
    root_q = clip_root_rotation(dof)
    return quat_normalize(quat_mul(quat_mul(root_q, tpose_body_q), quat_conj(root_q)))


def body_in_tpose_frame(hips_t, hips_q, dof, tpose_body_q, unit):
    turn = tpose_body_turn(dof, tpose_body_q)
    pivot = planted_feet_centre(dof, unit)
    offset = quat_rotate(turn, tuple(h - p for h, p in zip(hips_t, pivot)))
    return tuple(p + o for p, o in zip(pivot, offset)), quat_normalize(quat_mul(turn, hips_q))
