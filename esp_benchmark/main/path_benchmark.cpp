// ============================================================
// jps_go_style_fixed_v2.cpp
// 彻底修复了 JPS 强制邻居穿模 bug，确保路径合法且最优
// ============================================================

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <map>
#include <new>
#include <queue>
#include <random>
#include <set>
#include <utility>
#include <vector>
using namespace std;

const int INF = 1e9;
const int W_STRAIGHT = 10;
const int W_DIAGONAL = 14;

struct Grid
{
    int W, H;
    vector<int> cells;

    Grid(int w, int h) : W(w), H(h), cells(w * h, 0) {}

    int idx(int x, int y) const { return y * W + x; }
    bool inB(int x, int y) const { return x >= 0 && x < W && y >= 0 && y < H; }
    bool isBlock(int x, int y) const { return !inB(x, y) || cells[idx(x, y)] == 1; }
    bool isFree(int x, int y) const { return !isBlock(x, y); }

    void genRandom(int seed, float obstacleP)
    {
        mt19937 rng(seed);
        uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int i = 0; i < W * H; i++)
        {
            cells[i] = (dist(rng) < obstacleP) ? 1 : 0;
        }
        cells[0] = 0;
        cells[W * H - 1] = 0;
    }
};

struct Coord
{
    int x, y;
    Coord() : x(0), y(0) {}
    Coord(int xx, int yy) : x(xx), y(yy) {}

    Coord operator+(const Coord& o) const { return Coord(x + o.x, y + o.y); }
    Coord operator-(const Coord& o) const { return Coord(x - o.x, y - o.y); }
    bool operator==(const Coord& o) const { return x == o.x && y == o.y; }
    bool operator<(const Coord& o) const
    {
        if (x != o.x) return x < o.x;
        return y < o.y;
    }
};

int octileDist(const Coord& a, const Coord& b)
{
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    if (dx > dy) return 14 * dy + 10 * (dx - dy);
    return 14 * dx + 10 * (dy - dx);
}

enum JpsDir
{
    UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3,
    UP_LEFT = 4, UP_RIGHT = 5, DOWN_LEFT = 6, DOWN_RIGHT = 7,
    DIR_MAX = 8
};

Coord DIR_VEC[8] =
{
    Coord(0, -1),   // UP
    Coord(0, 1),    // DOWN
    Coord(-1, 0),   // LEFT
    Coord(1, 0),    // RIGHT
    Coord(-1, -1),  // UP_LEFT
    Coord(1, -1),   // UP_RIGHT
    Coord(-1, 1),   // DOWN_LEFT
    Coord(1, 1),    // DOWN_RIGHT
};

struct JpsNode
{
    Coord pos;
    int gCost;
    int hCost;
    JpsNode* parent = nullptr;
    vector<Coord> forceNeighbors;

    int fCost() const { return gCost + hCost; }
};

// 【修复核心】：所有生成强制邻居的判断，必须保证不发生对角线穿模
vector<Coord> hasForceNeighbor(const Grid& grid, const Coord& searchPos, JpsDir dir, const Coord& dst)
{
    vector<Coord> result;
    if (searchPos == dst)
    {
        result.push_back(searchPos);
        return result;
    }

    switch (dir)
    {
        case UP:
            if (grid.isBlock(searchPos.x - 1, searchPos.y) && grid.isFree(searchPos.x - 1, searchPos.y - 1) && grid.isFree(searchPos.x, searchPos.y - 1))
                result.push_back(Coord(searchPos.x - 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x + 1, searchPos.y) && grid.isFree(searchPos.x + 1, searchPos.y - 1) && grid.isFree(searchPos.x, searchPos.y - 1))
                result.push_back(Coord(searchPos.x + 1, searchPos.y - 1));
            break;

        case DOWN:
            if (grid.isBlock(searchPos.x - 1, searchPos.y) && grid.isFree(searchPos.x - 1, searchPos.y + 1) && grid.isFree(searchPos.x, searchPos.y + 1))
                result.push_back(Coord(searchPos.x - 1, searchPos.y + 1));
            if (grid.isBlock(searchPos.x + 1, searchPos.y) && grid.isFree(searchPos.x + 1, searchPos.y + 1) && grid.isFree(searchPos.x, searchPos.y + 1))
                result.push_back(Coord(searchPos.x + 1, searchPos.y + 1));
            break;

        case LEFT:
            if (grid.isBlock(searchPos.x, searchPos.y - 1) && grid.isFree(searchPos.x - 1, searchPos.y - 1) && grid.isFree(searchPos.x - 1, searchPos.y))
                result.push_back(Coord(searchPos.x - 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x, searchPos.y + 1) && grid.isFree(searchPos.x - 1, searchPos.y + 1) && grid.isFree(searchPos.x - 1, searchPos.y))
                result.push_back(Coord(searchPos.x - 1, searchPos.y + 1));
            break;

        case RIGHT:
            if (grid.isBlock(searchPos.x, searchPos.y - 1) && grid.isFree(searchPos.x + 1, searchPos.y - 1) && grid.isFree(searchPos.x + 1, searchPos.y))
                result.push_back(Coord(searchPos.x + 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x, searchPos.y + 1) && grid.isFree(searchPos.x + 1, searchPos.y + 1) && grid.isFree(searchPos.x + 1, searchPos.y))
                result.push_back(Coord(searchPos.x + 1, searchPos.y + 1));
            break;

        case UP_LEFT:
            if (grid.isBlock(searchPos.x + 1, searchPos.y) && grid.isFree(searchPos.x + 1, searchPos.y - 1) && grid.isFree(searchPos.x, searchPos.y - 1))
                result.push_back(Coord(searchPos.x + 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x, searchPos.y + 1) && grid.isFree(searchPos.x - 1, searchPos.y + 1) && grid.isFree(searchPos.x - 1, searchPos.y))
                result.push_back(Coord(searchPos.x - 1, searchPos.y + 1));
            break;

        case UP_RIGHT:
            if (grid.isBlock(searchPos.x - 1, searchPos.y) && grid.isFree(searchPos.x - 1, searchPos.y - 1) && grid.isFree(searchPos.x, searchPos.y - 1))
                result.push_back(Coord(searchPos.x - 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x, searchPos.y + 1) && grid.isFree(searchPos.x + 1, searchPos.y + 1) && grid.isFree(searchPos.x + 1, searchPos.y))
                result.push_back(Coord(searchPos.x + 1, searchPos.y + 1));
            break;

        case DOWN_LEFT:
            if (grid.isBlock(searchPos.x, searchPos.y - 1) && grid.isFree(searchPos.x - 1, searchPos.y - 1) && grid.isFree(searchPos.x - 1, searchPos.y))
                result.push_back(Coord(searchPos.x - 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x + 1, searchPos.y) && grid.isFree(searchPos.x + 1, searchPos.y + 1) && grid.isFree(searchPos.x, searchPos.y + 1))
                result.push_back(Coord(searchPos.x + 1, searchPos.y + 1));
            break;

        case DOWN_RIGHT:
            if (grid.isBlock(searchPos.x, searchPos.y - 1) && grid.isFree(searchPos.x + 1, searchPos.y - 1) && grid.isFree(searchPos.x + 1, searchPos.y))
                result.push_back(Coord(searchPos.x + 1, searchPos.y - 1));
            if (grid.isBlock(searchPos.x - 1, searchPos.y) && grid.isFree(searchPos.x - 1, searchPos.y + 1) && grid.isFree(searchPos.x, searchPos.y + 1))
                result.push_back(Coord(searchPos.x - 1, searchPos.y + 1));
            break;

        default:
            break;
    }
    return result;
}

struct JpsResult
{
    vector<Coord> path;
    int pathCost = -1;
    int expanded = 0;
    bool found = false;
};

class JpsSolver
{
public:
    JpsSolver(const Grid& g, Coord s, Coord d) : grid(g), src(s), dst(d)
    {
        open.reserve(1024);
    }

    JpsResult solve()
    {
        JpsResult res;

        JpsNode* startNode = new JpsNode();
        startNode->pos = src;
        startNode->gCost = 0;
        startNode->hCost = octileDist(src, dst);
        startNode->parent = nullptr;

        addNode(startNode);

        while (!open.empty())
        {
            JpsNode* cur = popBest();
            if (cur == nullptr) break;

            if (closed.count(cur->pos)) continue;
            closed.insert(cur->pos);
            res.expanded++;

            Coord curPos = cur->pos;

            if (curPos == dst || (abs(curPos.x - dst.x) + abs(curPos.y - dst.y) <= 1 && grid.isFree(dst.x, dst.y)))
            {
                if (!(curPos == dst)) {
                    JpsNode* finalNode = new JpsNode();
                    finalNode->pos = dst;
                    finalNode->gCost = cur->gCost + octileDist(curPos, dst);
                    finalNode->parent = cur;
                    cur = finalNode;
                    allNodes.push_back(finalNode);
                }

                res.found = true;
                vector<Coord> jumps;
                JpsNode* n = cur;
                while (n != nullptr)
                {
                    jumps.push_back(n->pos);
                    n = n->parent;
                }
                reverse(jumps.begin(), jumps.end());

                res.path = jumps;
                res.pathCost = cur->gCost;

                deleteAllNodes();
                return res;
            }

            vector<pair<JpsDir, Coord>> straightDirs;
            vector<pair<JpsDir, Coord>> slashDirs;

            if (cur->parent != nullptr)
            {
                Coord rel = curPos - cur->parent->pos;
                if (rel.x > 0) straightDirs.push_back({RIGHT, curPos});
                else if (rel.x < 0) straightDirs.push_back({LEFT, curPos});
                if (rel.y > 0) straightDirs.push_back({DOWN, curPos});
                else if (rel.y < 0) straightDirs.push_back({UP, curPos});

                if (rel.x > 0 && rel.y > 0) slashDirs.push_back({DOWN_RIGHT, curPos});
                if (rel.x > 0 && rel.y < 0) slashDirs.push_back({UP_RIGHT, curPos});
                if (rel.x < 0 && rel.y > 0) slashDirs.push_back({DOWN_LEFT, curPos});
                if (rel.x < 0 && rel.y < 0) slashDirs.push_back({UP_LEFT, curPos});
            }
            else
            {
                for (int i = UP; i <= RIGHT; i++) straightDirs.push_back({(JpsDir)i, curPos});
                for (int i = UP_LEFT; i <= DOWN_RIGHT; i++) slashDirs.push_back({(JpsDir)i, curPos});
            }

            for (auto& [dir, pos] : straightDirs) searchStraight(cur, pos, dir);
            for (auto& [dir, pos] : slashDirs) searchDiagonal(cur, pos, dir);
        }

        res.found = false;
        deleteAllNodes();
        return res;
    }

private:
    const Grid& grid;
    Coord src, dst;
    vector<JpsNode*> open;
    map<Coord, int> gCostMap;
    set<Coord> closed;
    vector<JpsNode*> allNodes;

    void addNode(JpsNode* node)
    {
        allNodes.push_back(node);
        if (gCostMap.count(node->pos) && gCostMap[node->pos] <= node->gCost)
        {
            return;
        }
        gCostMap[node->pos] = node->gCost;
        open.push_back(node);

        push_heap(open.begin(), open.end(),
                  [](JpsNode* a, JpsNode* b) { return a->fCost() > b->fCost(); });
    }

    JpsNode* popBest()
    {
        if (open.empty()) return nullptr;
        pop_heap(open.begin(), open.end(),
                 [](JpsNode* a, JpsNode* b) { return a->fCost() > b->fCost(); });
        JpsNode* res = open.back();
        open.pop_back();
        return res;
    }

    void searchStraight(JpsNode* curNode, Coord curPos, JpsDir dir)
    {
        Coord dirVec = DIR_VEC[dir];
        Coord searchPos = curPos;
        int weightAdd = 0;

        while (true)
        {
            searchPos = searchPos + dirVec;
            weightAdd += W_STRAIGHT;

            if (grid.isBlock(searchPos.x, searchPos.y)) break;

            vector<Coord> forceNb = hasForceNeighbor(grid, searchPos, dir, dst);

            if (!forceNb.empty() || searchPos == dst)
            {
                JpsNode* jp = new JpsNode();
                jp->pos = searchPos;
                jp->gCost = curNode->gCost + weightAdd;
                jp->hCost = octileDist(searchPos, dst);
                jp->parent = curNode;
                jp->forceNeighbors = forceNb;
                addNode(jp);

                for (auto& nbPos : forceNb)
                {
                    if (nbPos == searchPos) continue;
                    JpsNode* neighbor = new JpsNode();
                    neighbor->pos = nbPos;
                    neighbor->gCost = jp->gCost + W_DIAGONAL;
                    neighbor->hCost = octileDist(nbPos, dst);
                    neighbor->parent = jp;
                    addNode(neighbor);
                }
                break;
            }
        }
    }

    void searchDiagonal(JpsNode* curNode, Coord curPos, JpsDir dir)
    {
        vector<Coord> forceNb = hasForceNeighbor(grid, curPos, dir, dst);
        if (!forceNb.empty())
        {
            for (auto& val : forceNb)
            {
                if (val == curPos) continue;
                JpsNode* nbNode = new JpsNode();
                nbNode->pos = val;
                nbNode->gCost = curNode->gCost + W_DIAGONAL;
                nbNode->hCost = octileDist(val, dst);
                nbNode->parent = curNode;
                addNode(nbNode);
            }
        }

        Coord dirVec = DIR_VEC[dir];
        Coord newPos = curPos;
        int weightAdd = 0;

        while (true)
        {
            newPos = newPos + dirVec;
            weightAdd += W_DIAGONAL;

            if (grid.isBlock(newPos.x, newPos.y)) break;

            if (grid.isBlock(newPos.x - dirVec.x, newPos.y) && grid.isBlock(newPos.x, newPos.y - dirVec.y)) break;

            vector<Coord> fNb = hasForceNeighbor(grid, newPos, dir, dst);

            if (!fNb.empty() || newPos == dst)
            {
                JpsNode* jp = new JpsNode();
                jp->pos = newPos;
                jp->gCost = curNode->gCost + weightAdd;
                jp->hCost = octileDist(newPos, dst);
                jp->parent = curNode;
                jp->forceNeighbors = fNb;
                addNode(jp);

                for (auto& val : fNb)
                {
                    if (val == newPos) continue;
                    JpsNode* nbNode = new JpsNode();
                    nbNode->pos = val;
                    nbNode->gCost = jp->gCost + W_DIAGONAL;
                    nbNode->hCost = octileDist(val, dst);
                    nbNode->parent = jp;
                    addNode(nbNode);
                }
                break;
            }

            bool foundJump = false;
            switch (dir)
            {
                case UP_LEFT:
                    foundJump = probeStraight(newPos, UP) || probeStraight(newPos, LEFT); break;
                case UP_RIGHT:
                    foundJump = probeStraight(newPos, UP) || probeStraight(newPos, RIGHT); break;
                case DOWN_LEFT:
                    foundJump = probeStraight(newPos, DOWN) || probeStraight(newPos, LEFT); break;
                case DOWN_RIGHT:
                    foundJump = probeStraight(newPos, DOWN) || probeStraight(newPos, RIGHT); break;
                default: break;
            }

            if (foundJump)
            {
                JpsNode* jp = new JpsNode();
                jp->pos = newPos;
                jp->gCost = curNode->gCost + weightAdd;
                jp->hCost = octileDist(newPos, dst);
                jp->parent = curNode;
                addNode(jp);
                break;
            }
        }
    }

    bool probeStraight(Coord startPos, JpsDir dir)
    {
        Coord dirVec = DIR_VEC[dir];
        Coord p = startPos;

        while (true)
        {
            p = p + dirVec;
            if (grid.isBlock(p.x, p.y)) return false;
            if (p == dst) return true;

            vector<Coord> fn = hasForceNeighbor(grid, p, dir, dst);
            if (!fn.empty()) return true;
        }
    }

    void deleteAllNodes()
    {
        for (auto node : allNodes) delete node;
        allNodes.clear();
        open.clear();
        gCostMap.clear();
        closed.clear();
    }
};

// ==================== A* 参考逻辑 ====================

struct AStarResult
{
    vector<Coord> path;
    int pathCost = -1;
    int expanded = 0;
    bool found = false;
};

AStarResult astar8(const Grid& grid, Coord src, Coord dst)
{
    AStarResult res;
    if (!grid.isFree(src.x, src.y) || !grid.isFree(dst.x, dst.y)) return res;

    map<Coord, int> gCost;
    map<Coord, Coord> parent;
    set<Coord> closed;

    priority_queue<pair<int, Coord>, vector<pair<int, Coord>>, greater<>> open;

    gCost[src] = 0;
    open.push({octileDist(src, dst), src});

    int expanded = 0;

    while (!open.empty())
    {
        auto [f, cur] = open.top();
        open.pop();

        if (closed.count(cur)) continue;
        closed.insert(cur);
        expanded++;

        if (cur == dst) break;

        for (int d = 0; d < 8; d++)
        {
            Coord nb = cur + DIR_VEC[d];

            if (grid.isBlock(nb.x, nb.y)) continue;

            int stepCost = (d < 4) ? 10 : 14;

            if (d >= 4)
            {
                int ox = (DIR_VEC[d].x != 0) ? DIR_VEC[d].x : 0;
                int oy = (DIR_VEC[d].y != 0) ? DIR_VEC[d].y : 0;
                Coord orthoA(cur.x + ox, cur.y);
                Coord orthoB(cur.x, cur.y + oy);
                if (grid.isBlock(orthoA.x, orthoA.y) && grid.isBlock(orthoB.x, orthoB.y))
                {
                    continue;
                }
            }

            int nd = gCost[cur] + stepCost;

            if (!gCost.count(nb) || nd < gCost[nb])
            {
                gCost[nb] = nd;
                parent[nb] = cur;
                open.push({nd + octileDist(nb, dst), nb});
            }
        }
    }

    res.expanded = expanded;

    if (!gCost.count(dst)) return res;

    vector<Coord> path;
    Coord cur = dst;
    while (!(cur == src))
    {
        path.push_back(cur);
        auto it = parent.find(cur);
        if (it == parent.end()) return res;
        cur = it->second;
    }
    path.push_back(src);
    reverse(path.begin(), path.end());
    res.path = path;
    res.pathCost = gCost[dst];
    res.found = true;
    return res;
}

// Shared eight-connected movement model used by D* Lite and benchmark checks.
vector<pair<Coord, int>> neighbors8(const Grid& grid, const Coord& cur)
{
    vector<pair<Coord, int>> result;
    for (int d = 0; d < DIR_MAX; ++d)
    {
        Coord nb = cur + DIR_VEC[d];
        if (grid.isBlock(nb.x, nb.y)) continue;
        if (d >= UP_LEFT)
        {
            Coord orthoA(cur.x + DIR_VEC[d].x, cur.y);
            Coord orthoB(cur.x, cur.y + DIR_VEC[d].y);
            // Preserve the existing A*/JPS policy: only a fully blocked
            // corner forbids diagonal motion.
            if (grid.isBlock(orthoA.x, orthoA.y) &&
                grid.isBlock(orthoB.x, orthoB.y)) continue;
        }
        result.push_back({nb, d < UP_LEFT ? W_STRAIGHT : W_DIAGONAL});
    }
    return result;
}

struct DStarResult
{
    vector<Coord> path;
    int pathCost = -1;
    int expanded = 0;
    bool found = false;
};

struct DStarKey
{
    int first = INF;
    int second = INF;
    bool operator<(const DStarKey& other) const
    {
        if (first != other.first) return first < other.first;
        return second < other.second;
    }
    bool operator==(const DStarKey& other) const
    {
        return first == other.first && second == other.second;
    }
};

struct DStarQueueItem
{
    DStarKey key;
    Coord pos;
};

struct DStarQueueCompare
{
    bool operator()(const DStarQueueItem& a, const DStarQueueItem& b) const
    {
        if (a.key.first != b.key.first) return a.key.first > b.key.first;
        if (a.key.second != b.key.second) return a.key.second > b.key.second;
        return b.pos < a.pos;
    }
};

class DStarLiteSolver
{
public:
    DStarLiteSolver(Grid& gridRef, Coord startRef, Coord goalRef)
        : grid(gridRef), start(startRef), goal(goalRef) {}

    DStarResult solve()
    {
        DStarResult result;
        if (!grid.isFree(start.x, start.y) || !grid.isFree(goal.x, goal.y)) return result;

        rhs[goal] = 0;
        initialized = true;
        DStarKey goalKey = calculateKey(goal);
        openKeys[goal] = goalKey;
        open.push({goalKey, goal});
        computeShortestPath();
        if (getG(start) >= INF)
        {
            result.expanded = expanded;
            return result;
        }

        result.path.push_back(start);
        Coord current = start;
        set<Coord> visited;
        while (!(current == goal))
        {
            visited.insert(current);
            int best = INF;
            Coord next;
            bool hasNext = false;
            for (const auto& edge : neighbors8(grid, current))
            {
                int candidate = edge.second + getG(edge.first);
                if (candidate < best || (candidate == best && edge.first < next))
                {
                    best = candidate;
                    next = edge.first;
                    hasNext = true;
                }
            }
            if (!hasNext || best >= INF || visited.count(next))
            {
                result.path.clear();
                result.expanded = expanded;
                return result;
            }
            current = next;
            result.path.push_back(current);
            if (result.path.size() > grid.cells.size() + 1)
            {
                result.path.clear();
                result.expanded = expanded;
                return result;
            }
        }

        result.found = true;
        result.pathCost = getG(start);
        result.expanded = expanded;
        return result;
    }

    int updateCell(Coord cell, bool blocked)
    {
        if (!initialized || !grid.inB(cell.x, cell.y)) return 0;
        grid.cells[grid.idx(cell.x, cell.y)] = blocked ? 1 : 0;
        if (blocked)
        {
            g[cell] = INF;
            rhs[cell] = INF;
        }
        updateVertex(cell);
        // Update the geometric predecessor set, including vertices which no
        // longer connect to the newly blocked cell after the map update.
        for (int d = 0; d < DIR_MAX; ++d)
        {
            Coord predecessor = cell + DIR_VEC[d];
            if (grid.inB(predecessor.x, predecessor.y)) updateVertex(predecessor);
        }
        int before = expanded;
        computeShortestPath();
        return expanded - before;
    }

    DStarResult replan()
    {
        DStarResult result;
        if (!initialized || !grid.isFree(start.x, start.y) || !grid.isFree(goal.x, goal.y))
        {
            result.expanded = expanded;
            return result;
        }
        if (getG(start) >= INF)
        {
            result.expanded = expanded;
            return result;
        }

        result.path.push_back(start);
        Coord current = start;
        set<Coord> visited;
        while (!(current == goal))
        {
            visited.insert(current);
            int best = INF;
            Coord next;
            bool hasNext = false;
            for (const auto& edge : neighbors8(grid, current))
            {
                int candidate = edge.second + getG(edge.first);
                if (candidate < best || (candidate == best && edge.first < next))
                {
                    best = candidate;
                    next = edge.first;
                    hasNext = true;
                }
            }
            if (!hasNext || best >= INF || visited.count(next))
            {
                result.path.clear();
                result.expanded = expanded;
                return result;
            }
            current = next;
            result.path.push_back(current);
            if (result.path.size() > grid.cells.size() + 1)
            {
                result.path.clear();
                result.expanded = expanded;
                return result;
            }
        }
        result.found = true;
        result.pathCost = getG(start);
        result.expanded = expanded;
        return result;
    }

    int expansionCount() const { return expanded; }

private:
    Grid& grid;
    Coord start, goal;
    map<Coord, int> g, rhs;
    priority_queue<DStarQueueItem, vector<DStarQueueItem>, DStarQueueCompare> open;
    map<Coord, DStarKey> openKeys;
    bool initialized = false;

    int getG(const Coord& p) const
    {
        auto it = g.find(p);
        return it == g.end() ? INF : it->second;
    }

    int getRhs(const Coord& p) const
    {
        auto it = rhs.find(p);
        return it == rhs.end() ? INF : it->second;
    }

    int heuristic(const Coord& a, const Coord& b) const { return octileDist(a, b); }

    DStarKey calculateKey(const Coord& p) const
    {
        int value = min(getG(p), getRhs(p));
        return {value + heuristic(start, p), value};
    }

    void updateVertex(const Coord& u)
    {
        if (grid.isBlock(u.x, u.y))
        {
            rhs[u] = INF;
            openKeys.erase(u);
            return;
        }
        if (!(u == goal))
        {
            int best = INF;
            for (const auto& edge : neighbors8(grid, u))
                best = min(best, edge.second + getG(edge.first));
            rhs[u] = best;
        }
        if (getG(u) != getRhs(u))
        {
            DStarKey key = calculateKey(u);
            auto it = openKeys.find(u);
            if (it == openKeys.end() || !(it->second == key))
            {
                openKeys[u] = key;
                open.push({key, u});
            }
        }
        else
        {
            openKeys.erase(u);
        }
    }

    void computeShortestPath()
    {
        while (!open.empty())
        {
            DStarQueueItem item = open.top();
            DStarKey startKey = calculateKey(start);
            if (!(item.key < startKey) && getRhs(start) == getG(start)) break;
            open.pop();
            if (!(item.key.first == calculateKey(item.pos).first &&
                  item.key.second == calculateKey(item.pos).second)) continue;
            auto queued = openKeys.find(item.pos);
            if (queued == openKeys.end() || !(queued->second == item.key)) continue;
            openKeys.erase(queued);
            ++expanded;
            if (getG(item.pos) > getRhs(item.pos))
            {
                g[item.pos] = getRhs(item.pos);
                for (const auto& edge : neighbors8(grid, item.pos)) updateVertex(edge.first);
            }
            else
            {
                g[item.pos] = INF;
                updateVertex(item.pos);
                for (const auto& edge : neighbors8(grid, item.pos)) updateVertex(edge.first);
            }
        }
    }

    int expanded = 0;
};


extern "C" {
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}
#include "benchmark_profile.h"

namespace {
constexpr const char* TAG = "path_benchmark";
constexpr int kDefaultInstances = 10;
constexpr int kDefaultWarmups = 1;
constexpr int kDefaultMeasurements = 3;
constexpr long long kMissing = -1;
#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
constexpr int kPsramEnabled = 1;
#else
constexpr int kPsramEnabled = 0;
#endif

volatile size_t failedAllocationSize = 0;
volatile uint32_t failedAllocationCaps = 0;
volatile bool allocationFailed = false;
const char* allocationStage = "unknown";

struct Scenario { const char* name; int width; int height; int densityPercent; bool dynamic; };
enum class Planner { AStar, JPS, DStarLite };

void allocationFailureHook(size_t size, uint32_t caps, const char*) {
    failedAllocationSize = size; failedAllocationCaps = caps; allocationFailed = true;
}
void clearAllocationFailure() { allocationFailed = false; failedAllocationSize = 0; failedAllocationCaps = 0; allocationStage = "unknown"; }
const char* plannerName(Planner p) { return p == Planner::AStar ? "AStar" : p == Planner::JPS ? "JPS" : "DStarLite"; }
bool parsePlanner(const char* name, Planner& p) {
    if (strcmp(name, "AStar") == 0) { p = Planner::AStar; return true; }
    if (strcmp(name, "JPS") == 0) { p = Planner::JPS; return true; }
    if (strcmp(name, "DStarLite") == 0) { p = Planner::DStarLite; return true; }
    return false;
}
bool selectScenario(const char* name, Scenario& s) {
    static constexpr Scenario kScenarios[] = {
        {"initial_50x50_density10", 50, 50, 10, false},
        {"initial_100x100_density20", 100, 100, 20, false},
        {"initial_500x500_density20", 500, 500, 20, false},
        {"dynamic_50x50_density10", 50, 50, 10, true},
        {"dynamic_100x100_density20", 100, 100, 20, true},
        {"dynamic_500x500_density30", 500, 500, 30, true},
    };
    for (const auto& candidate : kScenarios) if (strcmp(name, candidate.name) == 0) { s = candidate; return true; }
    return false;
}
int mapSeed(const Scenario& s, int instance) { return 1000 + s.width * 17 + s.height * 31 + s.densityPercent * 101 + instance; }

struct Sample { bool found; int cost; int expanded; long long elapsedUs; size_t freeBefore; size_t freeAfter; size_t minBefore; size_t minAfter; };

void emitRow(const Scenario& s, Planner planner, int instance, int repetition, bool warmup, const char* phase,
             const char* status, const Sample& sample, int updateX, int updateY, size_t failedBytes,
             const char* failureStage, const char* failureReason) {
    // Warm-ups deliberately have no CSV rows and are excluded from all reports.
    if (warmup) return;
    ESP_LOGI(TAG,
        "csv,%s,%s,%s,%d,%d,%d,%s,esp32_%s_i%d,%d,%d,%d,%s,%d,%d,%d,%lld,%u,%u,%u,%u,%u,%s,%s,%d,%d,%d,%d,%s,%s",
        BENCHMARK_EXPERIMENT_ID, plannerName(planner), phase, s.width, s.height, s.densityPercent, s.dynamic ? "path_blocking" : "none",
        s.name, instance, mapSeed(s, instance), repetition, warmup ? 1 : 0, status, sample.found ? 1 : 0,
        sample.cost, sample.expanded, sample.elapsedUs, static_cast<unsigned>(sample.freeBefore),
        static_cast<unsigned>(sample.freeAfter), static_cast<unsigned>(sample.minBefore), static_cast<unsigned>(sample.minAfter),
        static_cast<unsigned>(failedBytes), failureStage, failureReason, updateX, updateY,
        kPsramEnabled, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, IDF_VER, BENCHMARK_FIRMWARE_GIT_COMMIT);
}

template <typename Solve> Sample measure(Solve&& solve) {
    const size_t before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t minBefore = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    const int64_t started = esp_timer_get_time();
    auto result = solve();
    const long long elapsed = esp_timer_get_time() - started;
    const size_t after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t minAfter = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    return {result.found, result.pathCost, result.expanded, elapsed, before, after, minBefore, minAfter};
}

bool chooseUpdate(Grid& grid, const vector<Coord>& path, Coord& changed) {
    if (path.size() < 3) return false;
    const size_t middle = path.size() / 2;
    for (size_t offset = 0; offset < path.size(); ++offset) {
        const size_t left = middle >= offset ? middle - offset : path.size();
        const size_t right = middle + offset;
        for (size_t index : {left, right}) {
            if (index == 0 || index + 1 >= path.size()) continue;
            const Coord candidate = path[index];
            grid.cells[grid.idx(candidate.x, candidate.y)] = 1;
            if (astar8(grid, Coord(0, 0), Coord(grid.W - 1, grid.H - 1)).found) { changed = candidate; return true; }
            grid.cells[grid.idx(candidate.x, candidate.y)] = 0;
        }
    }
    return false;
}

void releasePath(vector<Coord>& path) { path.clear(); vector<Coord>().swap(path); }

void runStatic(Planner planner, Grid& grid, const Scenario& s, int instance, int repetition, bool warmup) {
    allocationStage = "planner_initialization";
    if (planner == Planner::AStar) {
        allocationStage = "search";
        const Sample sample = measure([&] { return astar8(grid, Coord(0,0), Coord(grid.W-1,grid.H-1)); });
        emitRow(s, planner, instance, repetition, warmup, "initial", sample.found ? "OK" : "FAIL", sample, -1, -1, 0, "none", sample.found ? "none" : "path_not_found");
    } else if (planner == Planner::JPS) {
        JpsSolver solver(grid, Coord(0,0), Coord(grid.W-1,grid.H-1));
        allocationStage = "search";
        const Sample sample = measure([&] { return solver.solve(); });
        emitRow(s, planner, instance, repetition, warmup, "initial", sample.found ? "OK" : "FAIL", sample, -1, -1, 0, "none", sample.found ? "none" : "path_not_found");
    } else {
        DStarLiteSolver solver(grid, Coord(0,0), Coord(grid.W-1,grid.H-1));
        allocationStage = "search";
        const Sample sample = measure([&] { return solver.solve(); });
        emitRow(s, planner, instance, repetition, warmup, "initial", sample.found ? "OK" : "FAIL", sample, -1, -1, 0, "none", sample.found ? "none" : "path_not_found");
    }
}

void runDynamic(Planner planner, Grid& grid, const Scenario& s, int instance, int repetition, bool warmup) {
    const Coord start(0,0), goal(grid.W-1,grid.H-1);
    allocationStage = "event_selection";
    auto reference = astar8(grid, start, goal);
    Coord update;
    if (!reference.found || !chooseUpdate(grid, reference.path, update)) {
        Sample missing{false, -1, -1, kMissing, heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)};
        emitRow(s, planner, instance, repetition, warmup, "initial", "SKIP", missing, -1, -1, 0, "event_selection", "NO_DYNAMIC_EVENT");
        return;
    }
    releasePath(reference.path);
    // chooseUpdate leaves its accepted candidate blocked; every planner's
    // initial phase must instead use the original map.
    grid.cells[grid.idx(update.x, update.y)] = 0;
    if (planner == Planner::DStarLite) {
        allocationStage = "planner_initialization";
        DStarLiteSolver solver(grid, start, goal);
        allocationStage = "search";
        const Sample initial = measure([&] { return solver.solve(); });
        emitRow(s, planner, instance, repetition, warmup, "initial", initial.found ? "OK" : "FAIL", initial, update.x, update.y, 0, "none", initial.found ? "none" : "path_not_found");
        if (!initial.found) return;
        allocationStage = "map_update_replanning";
        const Sample repair = measure([&] { solver.updateCell(update, true); return solver.replan(); });
        emitRow(s, planner, instance, repetition, warmup, "repair", repair.found ? "OK" : "FAIL", repair, update.x, update.y, 0, "none", repair.found ? "none" : "path_not_found");
        return;
    }
    allocationStage = "planner_initialization";
    Sample initial;
    if (planner == Planner::AStar) initial = measure([&] { return astar8(grid, start, goal); });
    else { JpsSolver solver(grid, start, goal); allocationStage = "search"; initial = measure([&] { return solver.solve(); }); }
    emitRow(s, planner, instance, repetition, warmup, "initial", initial.found ? "OK" : "FAIL", initial, update.x, update.y, 0, "none", initial.found ? "none" : "path_not_found");
    if (!initial.found) return;
    allocationStage = "map_update";
    grid.cells[grid.idx(update.x, update.y)] = 1;
    allocationStage = "replanning";
    Sample replanned;
    if (planner == Planner::AStar) replanned = measure([&] { return astar8(grid, start, goal); });
    else { JpsSolver solver(grid, start, goal); replanned = measure([&] { return solver.solve(); }); }
    emitRow(s, planner, instance, repetition, warmup, "replan", replanned.found ? "OK" : "FAIL", replanned, update.x, update.y, 0, "none", replanned.found ? "none" : "path_not_found");
}

void runOne(Planner planner, const Scenario& s, int instance, int repetition, bool warmup) {
    clearAllocationFailure();
    const size_t before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t minBefore = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    try {
        allocationStage = "grid_allocation";
        { Grid grid(s.width, s.height); grid.genRandom(mapSeed(s, instance), s.densityPercent / 100.0f);
          if (s.dynamic) runDynamic(planner, grid, s, instance, repetition, warmup); else runStatic(planner, grid, s, instance, repetition, warmup); }
    } catch (const bad_alloc&) {
        Sample missing{false, -1, -1, kMissing, before, heap_caps_get_free_size(MALLOC_CAP_8BIT), minBefore, heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)};
        emitRow(s, planner, instance, repetition, warmup, s.dynamic ? "initial_or_replan" : "initial", "OOM", missing, -1, -1,
                failedAllocationSize, allocationStage, allocationFailed ? "bad_alloc_heap_callback" : "bad_alloc");
    }
}

} // namespace

extern "C" void app_main(void) {
    heap_caps_register_failed_alloc_callback(allocationFailureHook);
    Planner planner; Scenario scenario;
    if (!parsePlanner(BENCHMARK_PLANNER, planner) || !selectScenario(BENCHMARK_SCENARIO, scenario)) {
        ESP_LOGE(TAG, "profile_error,planner=%s,scenario=%s", BENCHMARK_PLANNER, BENCHMARK_SCENARIO);
        printf("planner_scenario_complete\n"); return;
    }
    ESP_LOGI(TAG, "csv_header,experiment_id,planner,phase,grid_width,grid_height,density,update_mode,map_id,seed,repetition,warmup,status,success,path_cost,expanded_nodes,elapsed_us,free_heap_before,free_heap_after,since_boot_minimum_free_heap_before,since_boot_minimum_free_heap_after,failed_alloc_bytes,failure_stage,failure_reason,update_x,update_y,psram_enabled,cpu_frequency_mhz,esp_idf_version,firmware_git_commit");
    ESP_LOGI(TAG, "profile,planner=%s,scenario=%s,instance_start=%d,instance_count=%d,warmups=%d,measurements=%d,experiment_id=%s,firmware_git_commit=%s,psram_enabled=%d,cpu_frequency_mhz=%d,esp_idf_version=%s",
             plannerName(planner), scenario.name, BENCHMARK_INSTANCE_START, BENCHMARK_INSTANCE_COUNT, BENCHMARK_WARMUP_COUNT, BENCHMARK_MEASURE_COUNT, BENCHMARK_EXPERIMENT_ID, BENCHMARK_FIRMWARE_GIT_COMMIT, kPsramEnabled, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, IDF_VER);
    for (int instance = BENCHMARK_INSTANCE_START; instance < BENCHMARK_INSTANCE_START + BENCHMARK_INSTANCE_COUNT; ++instance) {
        for (int warmup = 0; warmup < BENCHMARK_WARMUP_COUNT; ++warmup) {
            runOne(planner, scenario, instance, warmup, true);
            vTaskDelay(1); // Outside the timed region; permits the monitored idle task to run.
        }
        for (int repetition = 0; repetition < BENCHMARK_MEASURE_COUNT; ++repetition) {
            runOne(planner, scenario, instance, repetition, false);
            vTaskDelay(1); // Outside the timed region; permits the monitored idle task to run.
        }
    }
    printf("planner_scenario_complete\n");
}
