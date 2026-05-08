# Dash / Plotly visualizer for both Gift Wrapping and NAW
# -------------------------------------------------------
# Expected files from merged_hulls.cpp:
#
# points_uniform_1000_gw.txt
# hull_steps_uniform_1000_gw.txt
# hull_final_uniform_1000_gw.txt
#
# points_uniform_1000_naw.txt
# hull_steps_uniform_1000_naw.txt
# hull_final_uniform_1000_naw.txt
#
# Run:
#   python visualizer.py

import os
import plotly.graph_objects as go
from dash import Dash, dcc, html, Input, Output, State

##as i increase the number of points, the files get larger and take more time to load and render. 

POINT_OPTIONS = [10, 100, 500, 1000, 5000]
DEFAULT_POINTS = 1000

DISTRIBUTION_OPTIONS = ["uniform", "sphere", "gaussian", "clustered", "paraboloid", "ring", "shell", "ball"]
DEFAULT_DISTRIBUTION = "uniform"

ALGORITHM_OPTIONS = ["gw", "naw"]
DEFAULT_ALGORITHM = "naw"


# -------------------------------------------------------
# File loading
# -------------------------------------------------------

def load_points(filename):
    pts = []
    with open(filename) as f:
        for line in f:
            if line.strip():
                x, y, z = map(float, line.split())
                pts.append((x, y, z))
    return pts


def load_steps(filename):
    steps = []
    current = []

    with open(filename) as f:
        for line in f:
            line = line.strip()

            if not line:
                continue

            if line.startswith("#STEP"):
                if current:
                    steps.append(current)
                    current = []
            else:
                a, b, c = map(int, line.split())
                current.append((a, b, c))

        if current:
            steps.append(current)

    return steps


def load_final_faces(filename):
    faces = []
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            a, b, c = map(int, line.split())
            faces.append((a, b, c))
    return faces


# -------------------------------------------------------
# Plotly mesh helpers
# -------------------------------------------------------

def build_mesh(pts, faces):
    mesh_x, mesh_y, mesh_z = [], [], []
    i_idx, j_idx, k_idx = [], [], []
    intensity = []

    for idx, (a, b, c) in enumerate(faces):
        mesh_x.extend([pts[a][0], pts[b][0], pts[c][0]])
        mesh_y.extend([pts[a][1], pts[b][1], pts[c][1]])
        mesh_z.extend([pts[a][2], pts[b][2], pts[c][2]])

        base = 3 * idx
        i_idx.append(base)
        j_idx.append(base + 1)
        k_idx.append(base + 2)

        intensity.extend([idx, idx, idx])

    return mesh_x, mesh_y, mesh_z, i_idx, j_idx, k_idx, intensity


def build_wireframe_edges(pts, faces):
    x_edges, y_edges, z_edges = [], [], []

    for a, b, c in faces:
        for u, v in [(a, b), (b, c), (c, a)]:
            x_edges.extend([pts[u][0], pts[v][0], None])
            y_edges.extend([pts[u][1], pts[v][1], None])
            z_edges.extend([pts[u][2], pts[v][2], None])

    return x_edges, y_edges, z_edges


# -------------------------------------------------------
# Figure builder
# -------------------------------------------------------

def build_figure(point_count, distribution, algorithm, hull_style, point_size, point_opacity, mode):
    suffix = f"{distribution}_{point_count}_{algorithm}"

    points_file = f"points_{suffix}.txt"
    steps_file = f"hull_steps_{suffix}.txt"
    final_file = f"hull_final_{suffix}.txt"

    if mode == "final":
        hull_file = final_file
    else:
        hull_file = steps_file

    if not os.path.exists(points_file) or not os.path.exists(hull_file):
        fig = go.Figure()
        fig.update_layout(
            title=(
                f"Missing files:<br>{points_file}<br>{hull_file}<br>"
                f"Generate them first using merged_algorithms.cpp."
            ),
            paper_bgcolor="rgb(235, 238, 242)",
            plot_bgcolor="rgb(235, 238, 242)",
            scene=dict(bgcolor="rgb(235, 238, 242)")
        )
        return fig

    pts = load_points(points_file)

    if mode == "final":
        steps = [load_final_faces(final_file)]
    else:
        steps = load_steps(steps_file)

    if not steps or not steps[-1]:
        fig = go.Figure()
        fig.update_layout(title="No hull data found.")
        return fig

    boundary_vertices = set()
    for face in steps[-1]:
        boundary_vertices.update(face)

    point_colors = [
        "red" if i in boundary_vertices else "blue"
        for i in range(len(pts))
    ]

    frames = []

    for step_id, faces in enumerate(steps):
        frame_data = []

        mesh_x, mesh_y, mesh_z, i_idx, j_idx, k_idx, intensity = build_mesh(pts, faces)
        edge_x, edge_y, edge_z = build_wireframe_edges(pts, faces)

        if hull_style in ["solid", "both"]:
            frame_data.append(
                go.Mesh3d(
                    x=mesh_x,
                    y=mesh_y,
                    z=mesh_z,
                    i=i_idx,
                    j=j_idx,
                    k=k_idx,
                    intensity=intensity,
                    colorscale="Rainbow",
                    opacity=0.45,
                    flatshading=True,
                    showscale=False,
                    name="Solid Hull"
                )
            )

        if hull_style in ["wireframe", "both"]:
            frame_data.append(
                go.Scatter3d(
                    x=edge_x,
                    y=edge_y,
                    z=edge_z,
                    mode="lines",
                    line=dict(color="black", width=3),
                    name="Wireframe Hull"
                )
            )

        frame_data.append(
            go.Scatter3d(
                x=[p[0] for p in pts],
                y=[p[1] for p in pts],
                z=[p[2] for p in pts],
                mode="markers",
                marker=dict(
                    size=point_size,
                    color=point_colors,
                    opacity=point_opacity
                ),
                name="Points"
            )
        )

        frames.append(go.Frame(data=frame_data, name=str(step_id)))

    fig = go.Figure(data=frames[0].data, frames=frames)

    if mode == "animate":
        sliders = [
            dict(
                x=0.08,
                y=0.02,
                len=0.85,
                active=0,
                steps=[
                    dict(
                        method="animate",
                        args=[
                            [str(i)],
                            dict(
                                mode="immediate",
                                frame=dict(duration=0, redraw=True),
                                transition=dict(duration=0)
                            )
                        ],
                        label=str(i)
                    )
                    for i in range(len(frames))
                ]
            )
        ]

        updatemenus = [
            dict(
                type="buttons",
                direction="left",
                x=0.02,
                y=1.05,
                showactive=False,
                buttons=[
                    dict(
                        label="Run",
                        method="animate",
                        args=[
                            None,
                            dict(
                                frame=dict(duration=80, redraw=True),
                                transition=dict(duration=0),
                                fromcurrent=True
                            )
                        ]
                    ),
                    dict(
                        label="Pause",
                        method="animate",
                        args=[
                            [None],
                            dict(
                                mode="immediate",
                                frame=dict(duration=0),
                                transition=dict(duration=0)
                            )
                        ]
                    ),
                    dict(
                        label="Restart",
                        method="animate",
                        args=[
                            [str(i) for i in range(len(frames))],
                            dict(
                                mode="immediate",
                                frame=dict(duration=80, redraw=True),
                                transition=dict(duration=0),
                                fromcurrent=False
                            )
                        ]
                    )
                ]
            )
        ]
    else:
        sliders = []
        updatemenus = []

    title_mode = "Final Hull" if mode == "final" else "Animation"
    algo_name = "Gift Wrapping" if algorithm == "gw" else "Newton Apple Wrapper"

    fig.update_layout(
        title=(
            f"3D Convex Hull - {algo_name} | {title_mode} | "
            f"{distribution.capitalize()} | {point_count} Points"
        ),

        paper_bgcolor="rgb(235, 238, 242)",
        plot_bgcolor="rgb(235, 238, 242)",

        scene=dict(
            xaxis=dict(visible=False, showgrid=False, zeroline=False, showticklabels=False),
            yaxis=dict(visible=False, showgrid=False, zeroline=False, showticklabels=False),
            zaxis=dict(visible=False, showgrid=False, zeroline=False, showticklabels=False),
            bgcolor="rgb(235, 238, 242)",
            aspectmode="data"
        ),

        margin=dict(l=0, r=0, t=50, b=0),
        sliders=sliders,
        updatemenus=updatemenus
    )

    return fig


# -------------------------------------------------------
# Dash app
# -------------------------------------------------------

app = Dash(__name__)

app.layout = html.Div(
    style={
        "display": "flex",
        "height": "100vh",
        "backgroundColor": "rgb(235, 238, 242)",
        "fontFamily": "Arial"
    },
    children=[
        html.Div(
            style={
                "width": "300px",
                "backgroundColor": "rgb(215, 220, 228)",
                "padding": "25px",
                "boxShadow": "2px 0 8px rgba(0,0,0,0.12)"
            },
            children=[
                html.H3("Controls"),

                html.Label("Algorithm"),
                dcc.Dropdown(
                    id="algorithm",
                    options=[
                        {"label": "Gift Wrapping", "value": "gw"},
                        {"label": "Newton Apple Wrapper", "value": "naw"}
                    ],
                    value=DEFAULT_ALGORITHM,
                    clearable=False
                ),

                html.Br(),

                html.Label("Point Number"),
                dcc.Dropdown(
                    id="point-count",
                    options=[
                        {"label": f"{n} points", "value": n}
                        for n in POINT_OPTIONS
                    ],
                    value=DEFAULT_POINTS,
                    clearable=False
                ),

                html.Br(),

                html.Label("Distribution"),
                dcc.Dropdown(
                    id="distribution",
                    options=[
                        {"label": "Uniform cube", "value": "uniform"},
                        {"label": "Sphere surface", "value": "sphere"},
                        {"label": "Gaussian", "value": "gaussian"},
                        {"label": "Clustered", "value": "clustered"},
                        {"label": "Paraboloid", "value": "delaunay"},
                        {"label": "Ring", "value": "ring"},
                        {"label": "Shell", "value": "shell"},
                        {"label": "Ball", "value": "ball"}
                    ],
                    value=DEFAULT_DISTRIBUTION,
                    clearable=False
                ),

                html.Br(),

                html.Label("Display Mode"),
                dcc.Dropdown(
                    id="display-mode",
                    options=[
                        {"label": "Final Hull (faster)", "value": "final"},
                        {"label": "Animation (slower)", "value": "animate"}
                    ],
                    value="final",
                    clearable=False
                ),

                html.Br(),

                html.Button(
                    "Load Dataset",
                    id="load-button",
                    n_clicks=0,
                    style={
                        "width": "100%",
                        "padding": "10px",
                        "fontSize": "15px",
                        "cursor": "pointer"
                    }
                ),

                html.Br(),
                html.Br(),

                html.Label("Hull Style"),
                dcc.Dropdown(
                    id="hull-style",
                    options=[
                        {"label": "Solid", "value": "solid"},
                        {"label": "Wireframe", "value": "wireframe"},
                        {"label": "Solid + Wireframe", "value": "both"}
                    ],
                    value="solid",
                    clearable=False
                ),

                html.Br(),

                html.Label("Point Size"),
                dcc.Slider(
                    id="point-size",
                    min=1,
                    max=12,
                    step=1,
                    value=3,
                    marks={1: "1", 3: "3", 6: "6", 9: "9", 12: "12"}
                ),

                html.Br(),

                html.Label("Point Opacity"),
                dcc.Slider(
                    id="point-opacity",
                    min=0.1,
                    max=1.0,
                    step=0.1,
                    value=0.85,
                    marks={0.1: "0.1", 0.5: "0.5", 1.0: "1"}
                ),

                html.P(
                    "Algorithm, point number, distribution, and display mode change after Load Dataset. "
                    "Hull style and point settings update immediately.",
                    style={"fontSize": "13px", "color": "#333", "marginTop": "20px"}
                )
            ]
        ),

        html.Div(
            style={"flex": "1", "padding": "10px"},
            children=[
                dcc.Loading(
                    type="circle",
                    children=[
                        dcc.Graph(
                            id="hull-graph",
                            figure=build_figure(
                                DEFAULT_POINTS,
                                DEFAULT_DISTRIBUTION,
                                DEFAULT_ALGORITHM,
                                "solid",
                                3,
                                0.85,
                                "final"
                            ),
                            style={"height": "95vh"}
                        )
                    ]
                )
            ]
        )
    ]
)


@app.callback(
    Output("hull-graph", "figure"),

    Input("load-button", "n_clicks"),
    Input("hull-style", "value"),
    Input("point-size", "value"),
    Input("point-opacity", "value"),

    State("point-count", "value"),
    State("distribution", "value"),
    State("algorithm", "value"),
    State("display-mode", "value")
)
def update_graph(n_clicks, hull_style, point_size, point_opacity,
                 point_count, distribution, algorithm, display_mode):
    return build_figure(
        point_count,
        distribution,
        algorithm,
        hull_style,
        point_size,
        point_opacity,
        display_mode
    )


if __name__ == "__main__":
    app.run(debug=True)
