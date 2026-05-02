/*
 * ai.c — BFS pathfinding for the AI snake.
 * Falls back to greedy Manhattan distance if food is unreachable.
 */
#include "ai.h"
#include <string.h>

#define Q_SIZE (GRID_COLS * GRID_ROWS + 4)

typedef struct { Point pos; Direction first; } BFSNode;

static int visited[GRID_ROWS][GRID_COLS];
static BFSNode queue[Q_SIZE];

static int is_safe(Point p, const Snake *ai, const Snake *player,
                   const Point *obs, int obs_count)
{
    if (p.x < 0 || p.x >= GRID_COLS || p.y < 0 || p.y >= GRID_ROWS) return 0;
    if (snake_body_hit(ai, p)) return 0;
    if (snake_occupies(player, p)) return 0;
    for (int i = 0; i < obs_count; i++)
        if (obs[i].x==p.x && obs[i].y==p.y) return 0;
    return 1;
}

static Point dir_delta(Direction d) {
    switch(d){
        case DIR_UP:    return (Point){0,-1};
        case DIR_DOWN:  return (Point){0, 1};
        case DIR_LEFT:  return (Point){-1,0};
        case DIR_RIGHT: return (Point){ 1,0};
        default:        return (Point){0, 0};
    }
}

static Direction bfs(const Snake *ai, const Snake *player,
                     Point food, const Point *obs, int obs_count)
{
    memset(visited, 0, sizeof(visited));
    int head=0, tail=0;
    Direction dirs[4]={DIR_UP,DIR_DOWN,DIR_LEFT,DIR_RIGHT};

    for (int i=0;i<4;i++){
        Point d = dir_delta(dirs[i]);
        Point nb = {ai->body[0].x+d.x, ai->body[0].y+d.y};
        if (!is_safe(nb,ai,player,obs,obs_count)) continue;
        if (visited[nb.y][nb.x]) continue;
        visited[nb.y][nb.x]=1;
        queue[tail++]=(BFSNode){nb, dirs[i]};
        if (tail>=Q_SIZE) tail=0;
    }
    while (head!=tail){
        BFSNode cur=queue[head++];
        if (head>=Q_SIZE) head=0;
        if (cur.pos.x==food.x && cur.pos.y==food.y) return cur.first;
        for (int i=0;i<4;i++){
            Point d=dir_delta(dirs[i]);
            Point nb={cur.pos.x+d.x,cur.pos.y+d.y};
            if (!is_safe(nb,ai,player,obs,obs_count)) continue;
            if (visited[nb.y][nb.x]) continue;
            visited[nb.y][nb.x]=1;
            queue[tail++]=(BFSNode){nb, cur.first};
            if (tail>=Q_SIZE) tail=0;
        }
    }
    return DIR_NONE;
}

static Direction greedy(const Snake *ai, const Snake *player,
                        Point food, const Point *obs, int obs_count)
{
    Direction dirs[4]={DIR_RIGHT,DIR_DOWN,DIR_LEFT,DIR_UP};
    Direction best=DIR_NONE;
    int best_dist=999999;
    for (int i=0;i<4;i++){
        Direction d=dirs[i];
        /* don't reverse */
        if ((ai->dir==DIR_UP&&d==DIR_DOWN)||(ai->dir==DIR_DOWN&&d==DIR_UP)||
            (ai->dir==DIR_LEFT&&d==DIR_RIGHT)||(ai->dir==DIR_RIGHT&&d==DIR_LEFT)) continue;
        Point dv=dir_delta(d);
        Point nb={ai->body[0].x+dv.x, ai->body[0].y+dv.y};
        if (!is_safe(nb,ai,player,obs,obs_count)) continue;
        int dist=abs(nb.x-food.x)+abs(nb.y-food.y);
        if (dist<best_dist){best_dist=dist;best=d;}
    }
    if (best!=DIR_NONE) return best;
    /* any safe move */
    for (int i=0;i<4;i++){
        Point dv=dir_delta(dirs[i]);
        Point nb={ai->body[0].x+dv.x, ai->body[0].y+dv.y};
        if (is_safe(nb,ai,player,obs,obs_count)) return dirs[i];
    }
    return DIR_NONE;
}

void ai_think(Snake *ai, const Snake *player, Point food,
              const Point *obs, int obs_count)
{
    if (!ai->alive) return;
    Direction d = bfs(ai, player, food, obs, obs_count);
    if (d==DIR_NONE) d = greedy(ai, player, food, obs, obs_count);
    snake_set_dir(ai, d);
}
