// ============================================================
// jps_go_style_fixed_v2.cpp
// 彻底修复了 JPS 强制邻居穿模 bug，确保路径合法且最优
// ============================================================

#include <bits/stdc++.h>
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

#if 0 // Legacy v1-v3 benchmark pipeline retained for reference; replaced by BenchmarkRunner.inc.
struct BenchmarkRecord
{
    long long runId;
    int mapId;
    string algorithm;
    int width, height;
    double requestedDensity, actualDensity;
    int seed, repetition;
    Coord src, dst;
    bool found, pathValid, optimal;
    int pathCost, expanded, pathNodes;
    long long elapsedNs;
};

bool validPath(const Grid& grid, const vector<Coord>& path, Coord src, Coord dst)
{
    if (path.empty() || !(path.front() == src) || !(path.back() == dst)) return false;
    int total = 0;
    for (size_t i = 1; i < path.size(); ++i)
    {
        int dx = path[i].x - path[i - 1].x;
        int dy = path[i].y - path[i - 1].y;
        if (dx != 0 && dy != 0 && abs(dx) != abs(dy)) return false;
        int sx = (dx > 0) - (dx < 0), sy = (dy > 0) - (dy < 0);
        int steps = max(abs(dx), abs(dy));
        for (int step = 1; step <= steps; ++step)
        {
            Coord p(path[i - 1].x + sx * step, path[i - 1].y + sy * step);
            if (grid.isBlock(p.x, p.y)) return false;
            if (sx != 0 && sy != 0)
            {
                Coord previous(path[i - 1].x + sx * (step - 1),
                               path[i - 1].y + sy * (step - 1));
                if (grid.isBlock(previous.x + sx, previous.y) &&
                    grid.isBlock(previous.x, previous.y + sy)) return false;
            }
            total += (sx != 0 && sy != 0) ? W_DIAGONAL : W_STRAIGHT;
        }
    }
    return total >= 0;
}

template <typename Result>
BenchmarkRecord makeRecord(long long runId, int mapId, const string& name,
                           const Grid& grid, double requestedDensity, double actualDensity,
                           int seed, int repetition, Coord src, Coord dst,
                           const Result& result, long long elapsedNs, int referenceCost)
{
    BenchmarkRecord record{runId, mapId, name, grid.W, grid.H, requestedDensity,
                           actualDensity, seed, repetition, src, dst, result.found,
                           result.found && validPath(grid, result.path, src, dst),
                           result.pathCost == referenceCost,
                           result.pathCost, result.expanded, static_cast<int>(result.path.size()), elapsedNs};
    return record;
}

void writeCsv(const vector<BenchmarkRecord>& records, const string& file)
{
    ofstream out(file);
    out << "run_id,map_id,algorithm,width,height,requested_density,actual_density,seed,repetition,"
           "source_x,source_y,target_x,target_y,found,path_valid,optimal,path_cost,expanded_nodes,path_nodes,"
           "elapsed_ns,elapsed_us,elapsed_ms\n";
    out << fixed << setprecision(6);
    for (const auto& r : records)
    {
        out << r.runId << ',' << r.mapId << ',' << r.algorithm << ',' << r.width << ',' << r.height << ','
            << r.requestedDensity << ',' << r.actualDensity << ',' << r.seed << ',' << r.repetition << ','
            << r.src.x << ',' << r.src.y << ',' << r.dst.x << ',' << r.dst.y << ',' << r.found << ','
            << r.pathValid << ',' << r.optimal << ',' << r.pathCost << ',' << r.expanded << ',' << r.pathNodes << ','
            << r.elapsedNs << ',' << r.elapsedNs / 1000.0 << ',' << r.elapsedNs / 1000000.0 << '\n';
    }
}

void writeConfig(const string& file)
{
    ofstream out(file);
    out << "key,value\n"
        << "compiler,g++\n"
        << "compile_flags,\"-std=c++17 -O2\"\n"
        << "grid_sizes,\"50x50;100x100;500x500\"\n"
        << "densities,\"0.10;0.20;0.30\"\n"
        << "seeds,\"42;43;44;45;46\"\n"
        << "timing_repetitions,5\n"
        << "movement_model,8-connected\n"
        << "diagonal_rule,allow_single_corner_contact\n";
}

struct SummaryKey
{
    string algorithm;
    int width, height;
    double density;
    bool operator<(const SummaryKey& other) const
    {
        return tie(algorithm, width, height, density) <
               tie(other.algorithm, other.width, other.height, other.density);
    }
};

double percentile(vector<long long> values, double p)
{
    if (values.empty()) return 0.0;
    sort(values.begin(), values.end());
    double index = p * static_cast<double>(values.size() - 1);
    size_t lower = static_cast<size_t>(index);
    size_t upper = min(lower + 1, values.size() - 1);
    double fraction = index - static_cast<double>(lower);
    return values[lower] * (1.0 - fraction) + values[upper] * fraction;
}

void writeSummary(const vector<BenchmarkRecord>& records, const string& file)
{
    map<SummaryKey, vector<const BenchmarkRecord*>> groups;
    for (const auto& record : records)
        groups[{record.algorithm, record.width, record.height, record.requestedDensity}].push_back(&record);

    ofstream out(file);
    out << "algorithm,width,height,density,total_runs,successful_runs,failed_runs,success_rate,"
           "mean_path_cost,median_path_cost,mean_expanded_nodes,median_expanded_nodes,"
           "mean_elapsed_us,median_elapsed_us,p95_elapsed_us,min_elapsed_us,max_elapsed_us\n";
    out << fixed << setprecision(6);
    for (const auto& [key, group] : groups)
    {
        vector<long long> costs, expanded, times;
        long long costSum = 0, expandedSum = 0, timeSum = 0;
        int successful = 0;
        for (const auto* record : group)
        {
            if (!record->found) continue;
            ++successful;
            costs.push_back(record->pathCost);
            expanded.push_back(record->expanded);
            times.push_back(record->elapsedNs / 1000);
            costSum += record->pathCost;
            expandedSum += record->expanded;
            timeSum += record->elapsedNs / 1000;
        }
        auto mean = [](long long sum, size_t count) {
            return count == 0 ? 0.0 : static_cast<double>(sum) / count;
        };
        out << key.algorithm << ',' << key.width << ',' << key.height << ',' << key.density << ','
            << group.size() << ',' << successful << ',' << group.size() - successful << ','
            << mean(successful, group.size()) << ','
            << mean(costSum, costs.size()) << ',' << percentile(costs, 0.5) << ','
            << mean(expandedSum, expanded.size()) << ',' << percentile(expanded, 0.5) << ','
            << mean(timeSum, times.size()) << ',' << percentile(times, 0.5) << ','
            << percentile(times, 0.95) << ','
            << (times.empty() ? 0.0 : *min_element(times.begin(), times.end())) << ','
            << (times.empty() ? 0.0 : *max_element(times.begin(), times.end())) << '\n';
    }
}

struct DynamicRecord
{
    long long runId;
    int mapId;
    string algorithm;
    int width, height;
    double density;
    int seed, repetition;
    Coord src, dst, changedCell;
    int initialCost, replannedCost;
    bool found, pathValid, optimal;
    int initialExpanded, replannedExpanded;
    long long initialNs, replannedNs;
};

void writeDynamicCsv(const vector<DynamicRecord>& records, const string& file)
{
    ofstream out(file);
    out << "run_id,map_id,algorithm,width,height,density,seed,repetition,"
           "source_x,source_y,target_x,target_y,changed_x,changed_y,"
           "initial_cost,replanned_cost,found,path_valid,optimal,"
           "initial_expanded_nodes,replanned_expanded_nodes,initial_elapsed_ns,"
           "replanned_elapsed_ns,initial_elapsed_us,replanned_elapsed_us,"
           "initial_to_replan_ratio\n";
    out << fixed << setprecision(6);
    for (const auto& r : records)
    {
        double initialToReplanRatio = r.replannedNs > 0
            ? static_cast<double>(r.initialNs + r.replannedNs) / r.replannedNs
            : 0.0;
        out << r.runId << ',' << r.mapId << ',' << r.algorithm << ','
            << r.width << ',' << r.height << ',' << r.density << ','
            << r.seed << ',' << r.repetition << ','
            << r.src.x << ',' << r.src.y << ',' << r.dst.x << ',' << r.dst.y << ','
            << r.changedCell.x << ',' << r.changedCell.y << ','
            << r.initialCost << ',' << r.replannedCost << ',' << r.found << ','
            << r.pathValid << ',' << r.optimal << ','
            << r.initialExpanded << ',' << r.replannedExpanded << ','
            << r.initialNs << ',' << r.replannedNs << ','
            << r.initialNs / 1000.0 << ',' << r.replannedNs / 1000.0 << ','
            << initialToReplanRatio << '\n';
    }
}

void writeDynamicSummary(const vector<DynamicRecord>& records, const string& file)
{
    struct Group
    {
        vector<const DynamicRecord*> astar, jps, dstar;
    };
    map<tuple<int, int, int, double>, Group> groups;
    for (const auto& record : records)
    {
        auto& group = groups[{record.width, record.height, record.mapId, record.density}];
        if (record.algorithm == "AStar") group.astar.push_back(&record);
        else if (record.algorithm == "JPS") group.jps.push_back(&record);
        else group.dstar.push_back(&record);
    }

    ofstream out(file);
    out << "width,height,map_id,density,samples,"
           "astar_replan_us_mean,jps_replan_us_mean,dstar_replan_us_mean,"
           "astar_replan_expanded_mean,jps_replan_expanded_mean,dstar_replan_expanded_mean,"
           "dstar_vs_astar_time_speedup,dstar_vs_jps_time_speedup,"
           "dstar_vs_astar_expansion_ratio,dstar_vs_jps_expansion_ratio\n";
    out << fixed << setprecision(6);
    for (const auto& [key, group] : groups)
    {
        size_t count = min({group.astar.size(), group.jps.size(), group.dstar.size()});
        if (count == 0) continue;
        double astarTime = 0.0, jpsTime = 0.0, dstarTime = 0.0;
        double astarExpanded = 0.0, jpsExpanded = 0.0, dstarExpanded = 0.0;
        for (size_t i = 0; i < count; ++i)
        {
            astarTime += group.astar[i]->replannedNs / 1000.0;
            jpsTime += group.jps[i]->replannedNs / 1000.0;
            dstarTime += group.dstar[i]->replannedNs / 1000.0;
            astarExpanded += group.astar[i]->replannedExpanded;
            jpsExpanded += group.jps[i]->replannedExpanded;
            dstarExpanded += group.dstar[i]->replannedExpanded;
        }
        astarTime /= count; jpsTime /= count; dstarTime /= count;
        astarExpanded /= count; jpsExpanded /= count; dstarExpanded /= count;
        out << get<0>(key) << ',' << get<1>(key) << ',' << get<2>(key) << ',' << get<3>(key) << ','
            << count << ',' << astarTime << ',' << jpsTime << ',' << dstarTime << ','
            << astarExpanded << ',' << jpsExpanded << ',' << dstarExpanded << ','
            << (dstarTime > 0.0 ? astarTime / dstarTime : 0.0) << ','
            << (dstarTime > 0.0 ? jpsTime / dstarTime : 0.0) << ','
            << (astarExpanded > 0.0 ? dstarExpanded / astarExpanded : 0.0) << ','
            << (jpsExpanded > 0.0 ? dstarExpanded / jpsExpanded : 0.0) << '\n';
    }
}

template <typename Result>
DynamicRecord makeDynamicRecord(long long runId, int mapId, const string& algorithm,
                                const Grid& grid, double density, int seed, int repetition,
                                Coord src, Coord dst, Coord changedCell,
                                const Result& initial, const Result& replanned,
                                long long initialNs, long long replannedNs,
                                int referenceCost)
{
    return {runId, mapId, algorithm, grid.W, grid.H, density, seed, repetition,
            src, dst, changedCell, initial.pathCost, replanned.pathCost,
            replanned.found,
            replanned.found && validPath(grid, replanned.path, src, dst),
            replanned.pathCost == referenceCost,
            initial.expanded, replanned.expanded, initialNs, replannedNs};
}

bool chooseDynamicCell(Grid& grid, Coord src, Coord dst, const vector<Coord>& initialPath,
                       Coord& changedCell, int& changedIndex, int& changedCost)
{
    if (initialPath.size() < 3) return false;
    vector<int> candidates;
    int middle = static_cast<int>(initialPath.size() / 2);
    for (int offset = 0; offset < static_cast<int>(initialPath.size()); ++offset)
    {
        int left = middle - offset;
        int right = middle + offset;
        if (left > 0 && left + 1 < static_cast<int>(initialPath.size())) candidates.push_back(left);
        if (right != left && right > 0 && right + 1 < static_cast<int>(initialPath.size())) candidates.push_back(right);
    }
    for (int index : candidates)
    {
        Coord candidate = initialPath[index];
        if (grid.isBlock(candidate.x, candidate.y)) continue;
        grid.cells[grid.idx(candidate.x, candidate.y)] = 1;
        auto alternative = astar8(grid, src, dst);
        grid.cells[grid.idx(candidate.x, candidate.y)] = 0;
        if (alternative.found && alternative.pathCost >= 0)
        {
            changedCell = candidate;
            changedIndex = index;
            changedCost = alternative.pathCost;
            return true;
        }
    }
    return false;
}

int main()
{
    const vector<pair<int, int>> sizes = {{50, 50}, {100, 100}, {500, 500}};
    const vector<double> densities = {0.10, 0.20, 0.30};
    const vector<int> seeds = {42, 43, 44, 45, 46};
    const int repetitions = 5;
    vector<BenchmarkRecord> records;
    vector<DynamicRecord> dynamicRecords;
    long long runId = 1;
    long long dynamicRunId = 1;
    int mapId = 1;

    for (const auto& [width, height] : sizes)
    {
        for (double density : densities)
        {
            for (int seed : seeds)
            {
                Grid grid(width, height);
                grid.genRandom(seed, static_cast<float>(density));
                Coord src(0, 0), dst(width - 1, height - 1);
                int blocked = accumulate(grid.cells.begin(), grid.cells.end(), 0);
                double actualDensity = static_cast<double>(blocked) / grid.cells.size();

                // A* is the static reference and is intentionally measured too.
                for (int repetition = 0; repetition < repetitions; ++repetition)
                {
                    auto timeRun = [&](const string& name, auto&& solve, int referenceCost)
                    {
                        auto begin = chrono::steady_clock::now();
                        auto result = solve();
                        auto end = chrono::steady_clock::now();
                        long long ns = chrono::duration_cast<chrono::nanoseconds>(end - begin).count();
                        records.push_back(makeRecord(runId++, mapId, name, grid, density, actualDensity,
                                                     seed, repetition, src, dst, result, ns, referenceCost));
                    };

                    auto reference = astar8(grid, src, dst);
                    timeRun("AStar", [&] { return astar8(grid, src, dst); }, reference.pathCost);
                    timeRun("JPS", [&] { return JpsSolver(grid, src, dst).solve(); }, reference.pathCost);
                    timeRun("DStarLite", [&] { return DStarLiteSolver(grid, src, dst).solve(); }, reference.pathCost);
                }

                // Dynamic experiment: block a traversable middle cell of the
                // initial A* path, then compare restart planning with D* Lite
                // incremental replanning on the same modified map.
                // Choose the changed cell from D* Lite's own initial path so
                // the update actually invalidates its current plan rather
                // than accidentally leaving an equivalent route untouched.
                Grid scenarioGrid = grid;
                DStarLiteSolver scenarioDStar(scenarioGrid, src, dst);
                auto initialForScenario = scenarioDStar.solve();
                Coord changedCell;
                int changedIndex = -1;
                int dynamicReferenceCost = -1;
                if (initialForScenario.found &&
                    chooseDynamicCell(scenarioGrid, src, dst, initialForScenario.path,
                                      changedCell, changedIndex, dynamicReferenceCost))
                {
                    (void)changedIndex;
                    for (int repetition = 0; repetition < repetitions; ++repetition)
                    {
                        auto runDynamic = [&](const string& algorithm, auto&& initialSolve,
                                              auto&& replanSolve)
                        {
                            Grid initialGrid = grid;
                            auto initialBegin = chrono::steady_clock::now();
                            auto initial = initialSolve(initialGrid);
                            auto initialEnd = chrono::steady_clock::now();
                            long long initialNs = chrono::duration_cast<chrono::nanoseconds>(
                                initialEnd - initialBegin).count();

                            Grid changedGrid = grid;
                            changedGrid.cells[changedGrid.idx(changedCell.x, changedCell.y)] = 1;
                            auto replanBegin = chrono::steady_clock::now();
                            auto replanned = replanSolve(initial, changedGrid);
                            auto replanEnd = chrono::steady_clock::now();
                            long long replanNs = chrono::duration_cast<chrono::nanoseconds>(
                                replanEnd - replanBegin).count();

                            dynamicRecords.push_back(makeDynamicRecord(
                                dynamicRunId++, mapId, algorithm, changedGrid, density, seed,
                                repetition, src, dst, changedCell, initial, replanned,
                                initialNs, replanNs, dynamicReferenceCost));
                        };

                        runDynamic("AStar",
                            [&](Grid& g) { return astar8(g, src, dst); },
                            [&](const AStarResult&, Grid& g) { return astar8(g, src, dst); });
                        runDynamic("JPS",
                            [&](Grid& g) { return JpsSolver(g, src, dst).solve(); },
                            [&](const JpsResult&, Grid& g) { return JpsSolver(g, src, dst).solve(); });

                        Grid dstarGrid = grid;
                        DStarLiteSolver dstar(dstarGrid, src, dst);
                        auto initialBegin = chrono::steady_clock::now();
                        auto initial = dstar.solve();
                        auto initialEnd = chrono::steady_clock::now();
                        long long initialNs = chrono::duration_cast<chrono::nanoseconds>(
                            initialEnd - initialBegin).count();
                        int beforeReplanExpanded = initial.expanded;
                        auto replanBegin = chrono::steady_clock::now();
                        dstar.updateCell(changedCell, true);
                        auto replanned = dstar.replan();
                        auto replanEnd = chrono::steady_clock::now();
                        long long replanNs = chrono::duration_cast<chrono::nanoseconds>(
                            replanEnd - replanBegin).count();
                        replanned.expanded -= beforeReplanExpanded;
                        dynamicRecords.push_back(makeDynamicRecord(
                            dynamicRunId++, mapId, "DStarLite", dstarGrid, density, seed,
                            repetition, src, dst, changedCell, initial, replanned,
                            initialNs, replanNs, dynamicReferenceCost));
                    }
                }
                ++mapId;
            }
        }
    }

    filesystem::create_directories("results");
    writeCsv(records, "results/benchmark_raw.csv");
    writeSummary(records, "results/benchmark_summary.csv");
    writeConfig("results/benchmark_config.csv");
    writeDynamicCsv(dynamicRecords, "results/benchmark_dynamic_raw.csv");
    writeDynamicSummary(dynamicRecords, "results/benchmark_dynamic_summary.csv");
    cout << "Benchmark complete: " << records.size() << " records\n";
    cout << "Raw data: results/benchmark_raw.csv\n";
    cout << "Summary: results/benchmark_summary.csv\n";
    cout << "Dynamic data: results/benchmark_dynamic_raw.csv ("
         << dynamicRecords.size() << " records)\n";
    cout << "Dynamic summary: results/benchmark_dynamic_summary.csv\n";

    return 0;
}
#endif

#include "BenchmarkRunner.inc"