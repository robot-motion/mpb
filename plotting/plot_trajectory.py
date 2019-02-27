#!/usr/bin/env python3

import click
import numpy as np

plot_trajectory_options = [
    click.option('--draw_arrows', default=False, type=bool),
    click.option('--draw_dots', default=False, type=bool),
    click.option('--plot_every_nth_polygon', default=10, type=int),
    click.option('--draw_lines', default=True, type=bool),
    click.option('--line_alpha', default=0.6, type=float),
    click.option('--line_width', default=2., type=float)
]


def plot_trajectory(traj, planner: str, settings, color, add_label=True, alpha: float = 1., draw_arrows=False,
                    draw_dots=False, plot_every_nth_polygon=10, draw_lines=True, line_alpha=0.6, line_width=2., **_):
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon
    if len(traj) == 0:
        click.echo("Planner %s found no solution!" % planner)
        return
    traj = np.array(traj)
    if settings["collision_model"] == 0:
        # point collision model
        if add_label:
            plt.plot(traj[:, 0], traj[:, 1], '-', color=color, label=planner)
        else:
            plt.plot(traj[:, 0], traj[:, 1], '-', color=color)
        if draw_dots:
            plt.plot(traj[:, 0], traj[:, 1], '.', color=color)
    else:
        # polygon collision model
        points = np.array(settings["robot_shape"])
        if points.shape[0] == 0:
            raise Exception("Robot shape is empty!")
        if draw_lines:
            plt.plot(traj[:, 0], traj[:, 1], '-', color=color, alpha=line_alpha, linewidth=line_width)
        if add_label:
            plt.plot([], '-', color=color, label=planner)
        for i in range(traj.shape[0]):
            if i % plot_every_nth_polygon != 0:
                continue
            state = traj[i, :]
            c, s = np.cos(-state[2]), np.sin(-state[2])
            rotation = np.array([[c, -s], [s, c]])
            poly = Polygon(state[:2] + np.matmul(points, rotation), True, fill=False, linestyle='--', edgecolor=color,
                           alpha=alpha)
            plt.gca().add_patch(poly)
    if draw_arrows:
        import math
        for i in range(traj.shape[0]):
            state = traj[i, :]
            dx, dy = math.cos(state[2]), math.sin(state[2])
            plt.arrow(state[0], state[1], dx * 2., dy * 2., color=color, width=0.01, head_width=0.2, alpha=alpha)


def plot_nodes(traj, planner: str, settings, color, add_label=False, alpha: float = 1., draw_arrows=False, **_):
    import matplotlib.pyplot as plt
    if len(traj) == 0:
        click.echo("Planner %s found no solution!" % planner)
        return
    traj = np.array(traj)
    if settings["collision_model"] == 0:
        # point collision model
        if add_label:
            plt.plot(traj[:, 0], traj[:, 1], '.', color=color, alpha=alpha, label=planner)
        else:
            plt.plot(traj[:, 0], traj[:, 1], '.', color=color, alpha=alpha)
    # if draw_arrows:
    #     import math
    #     for i in range(traj.shape[0]):
    #         state = traj[i, :]
    #         dx, dy = math.cos(state[2]), math.sin(state[2])
    #         plt.arrow(state[0], state[1], dx * 2., dy * 2., color=color, width=0.01, head_width=0.2, alpha=alpha)