#include <bits/stdc++.h>
#include <limits>
using namespace std;

// ============================================================
// Merged 3D Convex Hull Generator
// Algorithms:
//   1) Gift Wrapping / Jarvis March extension to 3D  -> gw
//   2) Newton Apple Wrapper style incremental hull    -> naw
//   3) Run both                                      -> both
//
// Timing-only version: no point files, no hull files, no snapshots.
// ============================================================

const double EPS = std::numeric_limits<double>::epsilon() * 128.0;
const double PI = acos(-1.0);
const bool SAVE_STEPS = false;
const bool SAVE_FILES = false;

struct Point {
    double x, y, z;
    int id;
};

struct SimpleFace {
    int a, b, c;
    SimpleFace(int a_ = 0, int b_ = 0, int c_ = 0) : a(a_), b(b_), c(c_) {}
};

struct FaceKeyHash {
    size_t operator()(const array<int, 3>& t) const {
        size_t h1 = hash<int>{}(t[0]);
        size_t h2 = hash<int>{}(t[1]);
        size_t h3 = hash<int>{}(t[2]);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct EdgeTask {
    int u, v, prev;
    EdgeTask(int u_ = 0, int v_ = 0, int prev_ = 0) : u(u_), v(v_), prev(prev_) {}
};

Point operator+(const Point& a, const Point& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, -1};
}

Point operator-(const Point& a, const Point& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z, -1};
}

Point operator/(const Point& a, double k) {
    return {a.x / k, a.y / k, a.z / k, -1};
}

double dotP(const Point& a, const Point& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Point crossP(const Point& a, const Point& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
        -1
    };
}

double normP(const Point& a) {
    return sqrt(dotP(a, a));
}

Point normalizeP(const Point& a) {
    double n = normP(a);
    if (n < EPS) return {0, 0, 0, -1};
    return {a.x / n, a.y / n, a.z / n, -1};
}

double orient3D(const Point& a, const Point& b, const Point& c, const Point& p) {
    return dotP(crossP(b - a, c - a), p - a);
}

double areaTriangle(const Point& a, const Point& b, const Point& c) {
    return 0.5 * normP(crossP(b - a, c - a));
}

double areaTriangle2(const Point& a, const Point& b, const Point& c) {
    return normP(crossP(b - a, c - a));
}

array<int, 3> triKey(int a, int b, int c) {
    array<int, 3> t = {a, b, c};
    sort(t.begin(), t.end());
    return t;
}

unsigned long long packDir(int u, int v) {
    return (static_cast<unsigned long long>(static_cast<unsigned int>(u)) << 32) |
           static_cast<unsigned int>(v);
}

unsigned long long packUndir(int u, int v) {
    if (u > v) swap(u, v);
    return packDir(u, v);
}

// ============================================================
// Point generation
// ============================================================

vector<Point> generatePoints(int N, unsigned seed, const string& distribution) {
    mt19937 rng(seed);

    uniform_real_distribution<double> uni(-1.0, 1.0);
    normal_distribution<double> gauss(0.0, 0.35);
    uniform_real_distribution<double> angle(0.0, 2.0 * PI);
    uniform_real_distribution<double> u01(0.0, 1.0);

    vector<Point> pts;
    pts.reserve(N);

    if (distribution == "uniform" || distribution == "cube") {
        for (int i = 0; i < N; i++) {
            pts.push_back({uni(rng), uni(rng), uni(rng), i});
        }
    }
    else if (distribution == "gaussian") {
        for (int i = 0; i < N; i++) {
            pts.push_back({gauss(rng), gauss(rng), gauss(rng), i});
        }
    }
    else if (distribution == "sphere") {
        for (int i = 0; i < N; i++) {
            double theta = angle(rng);
            double z = uni(rng);
            double r = sqrt(max(0.0, 1.0 - z * z));
            pts.push_back({r * cos(theta), r * sin(theta), z, i});
        }
    }
    else if (distribution == "clustered") {
        vector<Point> centers = {
            {-0.6, -0.6, -0.6, -1},
            { 0.6,  0.6,  0.6, -1},
            {-0.6,  0.6,  0.3, -1},
            { 0.6, -0.5,  0.2, -1}
        };

        uniform_int_distribution<int> centerDist(0, (int)centers.size() - 1);
        normal_distribution<double> noise(0.0, 0.18);

        for (int i = 0; i < N; i++) {
            Point c = centers[centerDist(rng)];
            pts.push_back({c.x + noise(rng), c.y + noise(rng), c.z + noise(rng), i});
        }
    }
    else if (distribution == "delaunay" || distribution == "paraboloid") {
        for (int i = 0; i < N; i++) {
            double x = uni(rng);
            double y = uni(rng);
            double z = x * x + y * y;
            pts.push_back({x, y, z, i});
        }
    }
    else if (distribution == "ring") {
        double R = 1.0;
        double thickness = 0.08;
        normal_distribution<double> noise(0.0, thickness);

        for (int i = 0; i < N; i++) {
            double theta = angle(rng);
            double phi = angle(rng);

            double r_small = thickness;

            double x = (R + r_small * cos(phi)) * cos(theta);
            double y = (R + r_small * cos(phi)) * sin(theta);
            double z = r_small * sin(phi);

            x += noise(rng);
            y += noise(rng);
            z += noise(rng);

            pts.push_back({x, y, z, i});
        }
    }
    else if (distribution == "shell") {
        double r_min = 0.8;
        double r_max = 1.0;

        for (int i = 0; i < N; i++) {
            double theta = angle(rng);
            double z = uni(rng);

            double r_xy = sqrt(max(0.0, 1.0 - z * z));
            Point dir = {r_xy * cos(theta), r_xy * sin(theta), z, -1};

            double radius = r_min + (r_max - r_min) * u01(rng);

            pts.push_back({
                dir.x * radius,
                dir.y * radius,
                dir.z * radius,
                i
            });
        }
    }
    else if (distribution == "ball" || distribution == "solid_sphere") {
        for (int i = 0; i < N; i++) {
            double theta = angle(rng);
            double z = uni(rng);
            double r_xy = sqrt(max(0.0, 1.0 - z * z));

            double radius = cbrt(u01(rng));

            pts.push_back({
                radius * r_xy * cos(theta),
                radius * r_xy * sin(theta),
                radius * z,
                i
            });
        }
    }
    else {
        cerr << "Unknown distribution: " << distribution << "\n";
        cerr << "Use: uniform, gaussian, sphere, clustered, delaunay, ring, shell, ball\n";
        exit(1);
    }

    return pts;
}

// ============================================================
// Output helpers
// ============================================================

void writePoints(const vector<Point>& pts, const string& suffix) {
    ofstream pf("points_" + suffix + ".txt");
    for (const auto& p : pts) {
        pf << p.x << " " << p.y << " " << p.z << "\n";
    }
}

void writeSteps(const vector<vector<array<int,3>>>& steps, const string& suffix) {
    ofstream hf("hull_steps_" + suffix + ".txt");
    for (int s = 0; s < (int)steps.size(); s++) {
        hf << "#STEP " << s << "\n";
        for (auto& f : steps[s]) {
            hf << f[0] << " " << f[1] << " " << f[2] << "\n";
        }
    }
}

void writeFinal(const vector<array<int,3>>& faces, const string& suffix) {
    ofstream ff("hull_final_" + suffix + ".txt");
    for (auto& f : faces) {
        ff << f[0] << " " << f[1] << " " << f[2] << "\n";
    }
}

void verifyHull(const vector<array<int,3>>& faces, const string& label) {
    set<int> V;
    set<pair<int,int>> E;

    for (auto& f : faces) {
        int a = f[0], b = f[1], c = f[2];
        V.insert(a);
        V.insert(b);
        V.insert(c);

        auto addEdge = [&](int u, int v) {
            if (u > v) swap(u, v);
            E.insert({u, v});
        };

        addEdge(a, b);
        addEdge(b, c);
        addEdge(c, a);
    }

    int F = (int)faces.size();
    int euler = (int)V.size() - (int)E.size() + F;

    cout << "[" << label << "] Hull vertices = " << V.size() << "\n";
    cout << "[" << label << "] Unique edges  = " << E.size() << "\n";
    cout << "[" << label << "] Faces         = " << F << "\n";
    cout << "[" << label << "] Euler V-E+F   = " << euler << "\n";
}

// ============================================================
// Gift Wrapping
// ============================================================

bool isHullFace(const vector<Point>& pts, int a, int b, int c) {
    bool hasPos = false;
    bool hasNeg = false;

    for (int p = 0; p < (int)pts.size(); p++) {
        if (p == a || p == b || p == c) continue;

        double o = orient3D(pts[a], pts[b], pts[c], pts[p]);

        if (o > EPS) hasPos = true;
        if (o < -EPS) hasNeg = true;

        if (hasPos && hasNeg) return false;
    }

    return true;
}

void orientFaceOutwardGW(const vector<Point>& pts, int& a, int& b, int& c) {
    bool hasPos = false;

    for (int p = 0; p < (int)pts.size(); p++) {
        if (p == a || p == b || p == c) continue;

        double o = orient3D(pts[a], pts[b], pts[c], pts[p]);

        if (o > EPS) {
            hasPos = true;
            break;
        }
    }

    if (hasPos) swap(b, c);
}

int findNextPointReal3D(const vector<Point>& pts, int prev, int u, int v) {
    int best = -1;

    for (int w = 0; w < (int)pts.size(); w++) {
        if (w == prev || w == u || w == v) continue;
        if (areaTriangle(pts[u], pts[v], pts[w]) < EPS) continue;

        if (best == -1) {
            best = w;
            continue;
        }

        double s = orient3D(pts[u], pts[v], pts[best], pts[w]);

        if (s > EPS) {
            best = w;
        }
        else if (fabs(s) <= EPS) {
            double aBest = areaTriangle(pts[u], pts[v], pts[best]);
            double aW = areaTriangle(pts[u], pts[v], pts[w]);

            if (aW > aBest + EPS) {
                best = w;
            }
        }
    }

    if (best == -1) return -1;

    double check = orient3D(pts[u], pts[v], pts[prev], pts[best]);
    if (fabs(check) <= EPS) return -1;

    return best;
}

struct HullOutput {
    vector<Point> points;
    vector<vector<array<int,3>>> steps;
    vector<array<int,3>> finalFaces;
    double ms = 0.0;
};

HullOutput runGiftWrapping(vector<Point> pts) {
    HullOutput out;
    out.points = pts;

    int N = (int)pts.size();

    int ia = 0;
    for (int i = 1; i < N; i++) {
        if (pts[i].x < pts[ia].x - EPS ||
            (fabs(pts[i].x - pts[ia].x) < EPS && pts[i].y < pts[ia].y - EPS) ||
            (fabs(pts[i].x - pts[ia].x) < EPS &&
             fabs(pts[i].y - pts[ia].y) < EPS &&
             pts[i].z < pts[ia].z - EPS)) {
            ia = i;
        }
    }

    int ib = -1;
    double minAngle2D = 1e100;

    for (int i = 0; i < N; i++) {
        if (i == ia) continue;

        double dx = pts[i].x - pts[ia].x;
        double dy = pts[i].y - pts[ia].y;
        double ang = atan2(dx, dy);

        if (ib == -1 || ang < minAngle2D) {
            minAngle2D = ang;
            ib = i;
        }
    }

    int ic = -1;

    for (int i = 0; i < N; i++) {
        if (i == ia || i == ib) continue;
        if (areaTriangle(pts[ia], pts[ib], pts[i]) < EPS) continue;

        if (ic == -1) {
            ic = i;
            continue;
        }

        if (orient3D(pts[ia], pts[ib], pts[ic], pts[i]) > EPS) {
            ic = i;
        }
    }

    if (ia == -1 || ib == -1 || ic == -1) {
        cerr << "[GW] No valid initial hull face found.\n";
        return out;
    }

    orientFaceOutwardGW(pts, ia, ib, ic);

    vector<SimpleFace> faces;
    unordered_set<array<int, 3>, FaceKeyHash> usedFaces;
    deque<EdgeTask> q;
    unordered_set<unsigned long long> processedDir;
    unordered_map<unsigned long long, int> edgeCount;

    auto snapshot = [&]() {
        return;
    };

    auto addFace = [&](int a, int b, int c) -> bool {
        orientFaceOutwardGW(pts, a, b, c);

        array<int, 3> key = triKey(a, b, c);
        if (usedFaces.count(key)) return false;

        usedFaces.insert(key);
        faces.emplace_back(a, b, c);

        edgeCount[packUndir(a, b)]++;
        edgeCount[packUndir(b, c)]++;
        edgeCount[packUndir(c, a)]++;

        q.push_back(EdgeTask(b, a, c));
        q.push_back(EdgeTask(c, b, a));
        q.push_back(EdgeTask(a, c, b));

        return true;
    };

    auto t0 = chrono::high_resolution_clock::now();

    addFace(ia, ib, ic);
    snapshot();

    while (!q.empty()) {
        EdgeTask cur = q.front();
        q.pop_front();

        int u = cur.u;
        int v = cur.v;
        int prev = cur.prev;

        unsigned long long dkey = packDir(u, v);

        if (processedDir.count(dkey)) continue;

        unsigned long long ekey = packUndir(u, v);

        if (edgeCount[ekey] >= 2) {
            processedDir.insert(dkey);
            continue;
        }

        int w = findNextPointReal3D(pts, prev, u, v);

        if (w == -1) {
            processedDir.insert(dkey);
            continue;
        }

        int a = u;
        int b = v;
        int c = w;

        orientFaceOutwardGW(pts, a, b, c);

        if (!isHullFace(pts, a, b, c)) {
            processedDir.insert(dkey);
            continue;
        }

        if (!usedFaces.count(triKey(a, b, c))) {
            if (addFace(a, b, c)) {
                snapshot();
            }
        }

        processedDir.insert(dkey);
    }

    auto t1 = chrono::high_resolution_clock::now();

    out.ms = chrono::duration<double, milli>(t1 - t0).count();

    for (auto& f : faces) {
        out.finalFaces.push_back({f.a, f.b, f.c});
    }

    return out;
}

// ============================================================
// Newton Apple Wrapper - Incremental topology version
// ============================================================

struct NAWFace {
    int v[3];
    int adj[3];
    bool alive;

    NAWFace() {
        v[0] = v[1] = v[2] = -1;
        adj[0] = adj[1] = adj[2] = -1;
        alive = true;
    }

    NAWFace(int a, int b, int c) {
        v[0] = a;
        v[1] = b;
        v[2] = c;

        adj[0] = adj[1] = adj[2] = -1;
        alive = true;
    }
};

class NAWHull3D {
private:
    vector<Point> pts;
    vector<NAWFace> faces;
    vector<int> aliveFaces;
    vector<vector<int>> vertexFaces;
    unordered_map<unsigned long long, int> edgeFace;
    vector<vector<array<int,3>>> steps;

    Point interiorPoint;
    int lastInsertedHullVertex = -1;

    bool faceVisible(int fi, int p) const {
        if (fi < 0 || fi >= (int)faces.size()) return false;

        const NAWFace& f = faces[fi];
        if (!f.alive) return false;

        return orient3D(pts[f.v[0]], pts[f.v[1]], pts[f.v[2]], pts[p]) > EPS;
    }

    void orientOutward(NAWFace& f) {
        double s = orient3D(
            pts[f.v[0]],
            pts[f.v[1]],
            pts[f.v[2]],
            interiorPoint
        );

        if (s > EPS) {
            swap(f.v[1], f.v[2]);
        }
    }

    bool findInitialTetrahedron(int& i0, int& i1, int& i2, int& i3) {
        int N = (int)pts.size();

        if (N < 4) return false;

        i0 = 0;
        i1 = i2 = i3 = -1;

        for (int i = 1; i < N; i++) {
            if (fabs(pts[i].x - pts[i0].x) > EPS ||
                fabs(pts[i].y - pts[i0].y) > EPS ||
                fabs(pts[i].z - pts[i0].z) > EPS) {
                i1 = i;
                break;
            }
        }

        if (i1 == -1) return false;

        for (int i = 0; i < N; i++) {
            if (i == i0 || i == i1) continue;

            if (areaTriangle2(pts[i0], pts[i1], pts[i]) > EPS) {
                i2 = i;
                break;
            }
        }

        if (i2 == -1) return false;

        for (int i = 0; i < N; i++) {
            if (i == i0 || i == i1 || i == i2) continue;

            if (fabs(orient3D(pts[i0], pts[i1], pts[i2], pts[i])) > EPS) {
                i3 = i;
                break;
            }
        }

        return i3 != -1;
    }

    void snapshot() {
        return;
    }

    int addFaceRaw(int a, int b, int c) {
        NAWFace f(a, b, c);
        orientOutward(f);

        int id = (int)faces.size();
        faces.push_back(f);
        aliveFaces.push_back(id);

        return id;
    }

    void rebuildTopologyInitialOnly() {
        edgeFace.clear();
        vertexFaces.assign(pts.size(), {});

        for (int fi : aliveFaces) {
            if (fi < 0 || fi >= (int)faces.size()) continue;

            NAWFace& f = faces[fi];
            if (!f.alive) continue;

            f.adj[0] = f.adj[1] = f.adj[2] = -1;

            for (int e = 0; e < 3; e++) {
                int u = f.v[e];
                int v = f.v[(e + 1) % 3];

                edgeFace[packDir(u, v)] = fi;
            }

            vertexFaces[f.v[0]].push_back(fi);
            vertexFaces[f.v[1]].push_back(fi);
            vertexFaces[f.v[2]].push_back(fi);
        }

        for (int fi : aliveFaces) {
            if (fi < 0 || fi >= (int)faces.size()) continue;

            NAWFace& f = faces[fi];
            if (!f.alive) continue;

            for (int e = 0; e < 3; e++) {
                int u = f.v[e];
                int v = f.v[(e + 1) % 3];

                auto it = edgeFace.find(packDir(v, u));

                if (it != edgeFace.end()) {
                    f.adj[e] = it->second;
                }
            }
        }
    }

    void rebuildAliveFaces() {
        vector<int> compact;
        compact.reserve(aliveFaces.size());

        for (int fi : aliveFaces) {
            if (fi >= 0 && fi < (int)faces.size() && faces[fi].alive) {
                compact.push_back(fi);
            }
        }

        aliveFaces.swap(compact);
    }

    void removeFaceIncremental(int fi) {
        if (fi < 0 || fi >= (int)faces.size()) return;
        if (!faces[fi].alive) return;

        NAWFace& f = faces[fi];

        for (int e = 0; e < 3; e++) {
            int u = f.v[e];
            int v = f.v[(e + 1) % 3];

            edgeFace.erase(packDir(u, v));

            int nb = f.adj[e];

            if (nb >= 0 && nb < (int)faces.size() && faces[nb].alive) {
                NAWFace& nf = faces[nb];

                for (int ne = 0; ne < 3; ne++) {
                    if (nf.adj[ne] == fi) {
                        nf.adj[ne] = -1;
                    }
                }
            }
        }

        f.alive = false;
        f.adj[0] = f.adj[1] = f.adj[2] = -1;
    }

    void connectFaceToNeighbors(int fi) {
        if (fi < 0 || fi >= (int)faces.size()) return;
        if (!faces[fi].alive) return;

        NAWFace& f = faces[fi];

        f.adj[0] = f.adj[1] = f.adj[2] = -1;

        for (int e = 0; e < 3; e++) {
            int u = f.v[e];
            int v = f.v[(e + 1) % 3];

            auto it = edgeFace.find(packDir(v, u));

            if (it != edgeFace.end()) {
                int nb = it->second;

                if (nb >= 0 && nb < (int)faces.size() && faces[nb].alive) {
                    f.adj[e] = nb;

                    NAWFace& nf = faces[nb];

                    for (int ne = 0; ne < 3; ne++) {
                        int nu = nf.v[ne];
                        int nv = nf.v[(ne + 1) % 3];

                        if (nu == v && nv == u) {
                            nf.adj[ne] = fi;
                            break;
                        }
                    }
                }
            }

            edgeFace[packDir(u, v)] = fi;
        }

        vertexFaces[f.v[0]].push_back(fi);
        vertexFaces[f.v[1]].push_back(fi);
        vertexFaces[f.v[2]].push_back(fi);
    }

    int addFaceIncremental(int a, int b, int c) {
        NAWFace f(a, b, c);
        orientOutward(f);

        int id = (int)faces.size();

        faces.push_back(f);
        aliveFaces.push_back(id);

        connectFaceToNeighbors(id);

        return id;
    }

    bool findSeedVisibleFace(int p, int& seedFace) {
        seedFace = -1;

        if (lastInsertedHullVertex >= 0 &&
            lastInsertedHullVertex < (int)vertexFaces.size()) {

            for (int fi : vertexFaces[lastInsertedHullVertex]) {
                if (faceVisible(fi, p)) {
                    seedFace = fi;
                    return true;
                }
            }
        }

        for (int fi : aliveFaces) {
            if (faceVisible(fi, p)) {
                seedFace = fi;
                return true;
            }
        }

        return false;
    }

    vector<int> collectVisibleRegion(int p, int seedFace) {
        vector<int> visible;
        queue<int> q;
        unordered_set<int> visited;

        q.push(seedFace);
        visited.insert(seedFace);

        while (!q.empty()) {
            int fi = q.front();
            q.pop();

            if (!faceVisible(fi, p)) continue;

            visible.push_back(fi);

            const NAWFace& f = faces[fi];

            for (int e = 0; e < 3; e++) {
                int nb = f.adj[e];

                if (nb == -1) continue;
                if (!faces[nb].alive) continue;

                if (visited.insert(nb).second) {
                    q.push(nb);
                }
            }
        }

        return visible;
    }

    vector<pair<int,int>> computeHorizon(const vector<int>& visible) {
        unordered_set<int> visSet(visible.begin(), visible.end());
        vector<pair<int,int>> horizon;

        for (int fi : visible) {
            const NAWFace& f = faces[fi];

            for (int e = 0; e < 3; e++) {
                int nb = f.adj[e];

                if (nb == -1 || !visSet.count(nb)) {
                    int u = f.v[e];
                    int v = f.v[(e + 1) % 3];

                    horizon.push_back({u, v});
                }
            }
        }

        return horizon;
    }

    void insertPoint(int p) {
        int seedFace;

        if (!findSeedVisibleFace(p, seedFace)) return;

        vector<int> visible = collectVisibleRegion(p, seedFace);

        if (visible.empty()) return;

        vector<pair<int,int>> horizon = computeHorizon(visible);

        for (int fi : visible) {
            removeFaceIncremental(fi);
        }

        rebuildAliveFaces();

        for (auto& e : horizon) {
            int u = e.first;
            int v = e.second;

            addFaceIncremental(v, u, p);
        }

        lastInsertedHullVertex = p;

        snapshot();
    }

public:
    explicit NAWHull3D(vector<Point> input) : pts(std::move(input)) {}

    bool build() {
        int N = (int)pts.size();

        if (N < 4) return false;

        sort(pts.begin(), pts.end(), [](const Point& a, const Point& b) {
            if (fabs(a.z - b.z) > EPS) return a.z < b.z;
            if (fabs(a.x - b.x) > EPS) return a.x < b.x;
            return a.y < b.y;
        });

        for (int i = 0; i < N; i++) {
            pts[i].id = i;
        }

        int i0, i1, i2, i3;

        if (!findInitialTetrahedron(i0, i1, i2, i3)) {
            return false;
        }

        interiorPoint = (pts[i0] + pts[i1] + pts[i2] + pts[i3]) / 4.0;
        interiorPoint.id = -1;

        faces.clear();
        aliveFaces.clear();
        edgeFace.clear();
        vertexFaces.assign(N, {});
        steps.clear();

        addFaceRaw(i0, i1, i2);
        addFaceRaw(i0, i3, i1);
        addFaceRaw(i0, i2, i3);
        addFaceRaw(i1, i3, i2);

        rebuildTopologyInitialOnly();
        snapshot();

        set<int> initial = {i0, i1, i2, i3};

        lastInsertedHullVertex = max(max(i0, i1), max(i2, i3));

        for (int p = 0; p < N; p++) {
            if (initial.count(p)) continue;

            insertPoint(p);
        }

        rebuildAliveFaces();

        return true;
    }

    const vector<Point>& getPoints() const {
        return pts;
    }

    const vector<vector<array<int,3>>>& getSteps() const {
        return steps;
    }

    vector<array<int,3>> finalFaces() const {
        vector<array<int,3>> out;

        for (int fi : aliveFaces) {
            if (faces[fi].alive) {
                out.push_back({
                    faces[fi].v[0],
                    faces[fi].v[1],
                    faces[fi].v[2]
                });
            }
        }

        return out;
    }
};

HullOutput runNAW(vector<Point> pts) {
    HullOutput out;

    NAWHull3D hull(pts);

    auto t0 = chrono::high_resolution_clock::now();
    bool ok = hull.build();
    auto t1 = chrono::high_resolution_clock::now();

    if (!ok) {
        cerr << "[NAW] Could not build hull. Degenerate input?\n";
        return out;
    }

    out.ms = chrono::duration<double, milli>(t1 - t0).count();
    out.points = hull.getPoints();
    out.steps = hull.getSteps();
    out.finalFaces = hull.finalFaces();

    return out;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char** argv) {
    int N = 1000;
    unsigned seed = 12345;
    string distribution = "uniform";
    string algorithm = "both";

    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) seed = static_cast<unsigned>(atoi(argv[2]));
    if (argc > 3) distribution = argv[3];
    if (argc > 4) algorithm = argv[4];

    cout << "[INFO] Timing-only mode: file output and snapshots disabled\n";
    cout << "[INFO] EPS = " << scientific << EPS << defaultfloat << "\n";

    vector<Point> basePts = generatePoints(N, seed, distribution);

    cout << "Input points   = " << N << "\n";
    cout << "Distribution   = " << distribution << "\n";
    cout << "Algorithm      = " << algorithm << "\n";

    if (algorithm == "gw" || algorithm == "both") {
        HullOutput gw = runGiftWrapping(basePts);
        string suffix = distribution + "_" + to_string(N) + "_gw";

        if (SAVE_FILES) {
            writePoints(gw.points, suffix);

            if (SAVE_STEPS) {
                writeSteps(gw.steps, suffix);
            }

            writeFinal(gw.finalFaces, suffix);
        }

        cout << "\n[GW] Finished Gift Wrapping.\n";
        cout << "[GW] Hull faces   = " << gw.finalFaces.size() << "\n";
        cout << "[GW] Build steps  = " << gw.steps.size() << "\n";
        cout << "[GW] Wall time ms = " << gw.ms << "\n";

        verifyHull(gw.finalFaces, "GW");
    }

    if (algorithm == "naw" || algorithm == "both") {
        HullOutput naw = runNAW(basePts);
        string suffix = distribution + "_" + to_string(N) + "_naw";

        if (SAVE_FILES) {
            writePoints(naw.points, suffix);

            if (SAVE_STEPS) {
                writeSteps(naw.steps, suffix);
            }

            writeFinal(naw.finalFaces, suffix);
        }

        cout << "\n[NAW] Finished NAW-style hull.\n";
        cout << "[NAW] Hull faces   = " << naw.finalFaces.size() << "\n";
        cout << "[NAW] Build steps  = " << naw.steps.size() << "\n";
        cout << "[NAW] Wall time ms = " << naw.ms << "\n";

        verifyHull(naw.finalFaces, "NAW");
    }

    if (algorithm != "gw" && algorithm != "naw" && algorithm != "both") {
        cerr << "Unknown algorithm: " << algorithm << "\n";
        cerr << "Use: gw, naw, or both\n";
        return 1;
    }

    return 0;
}
