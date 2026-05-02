/*
 * snake.c — Snake entity: initialisation, movement, body management.
 */
#include "snake.h"

static int opposite(Direction a, Direction b) {
    return (a==DIR_UP&&b==DIR_DOWN)||(a==DIR_DOWN&&b==DIR_UP)||
           (a==DIR_LEFT&&b==DIR_RIGHT)||(a==DIR_RIGHT&&b==DIR_LEFT);
}

void snake_init(Snake *s, int hx, int hy, Direction dir, int len, int is_ai)
{
    memset(s, 0, sizeof(*s));
    s->alive   = 1;
    s->is_ai   = is_ai;
    s->dir     = dir;
    s->next_dir= dir;
    s->length  = len;
    s->anim_t  = 1.0f;

    s->body[0].x = hx; s->body[0].y = hy;
    for (int i = 1; i < len; i++) {
        s->body[i] = s->body[i-1];
        switch(dir){
            case DIR_RIGHT: s->body[i].x--; break;
            case DIR_LEFT:  s->body[i].x++; break;
            case DIR_DOWN:  s->body[i].y--; break;
            case DIR_UP:    s->body[i].y++; break;
            default: break;
        }
    }
    s->prev_head = s->body[0];
}

void snake_set_dir(Snake *s, Direction d) {
    if (d == DIR_NONE) return;
    if (opposite(s->dir, d)) return;
    s->next_dir = d;
}

Point snake_next_head(const Snake *s) {
    Point h = s->body[0];
    switch(s->next_dir) {
        case DIR_UP:    h.y--; break;
        case DIR_DOWN:  h.y++; break;
        case DIR_LEFT:  h.x--; break;
        case DIR_RIGHT: h.x++; break;
        default: break;
    }
    return h;
}

/* Advance one grid step. grow=1: append tail. Returns 1 always (bounds checked by caller). */
int snake_step(Snake *s, int grow)
{
    if (!s->alive) return 0;
    s->prev_head = s->body[0];
    s->dir = s->next_dir;

    if (grow) {
        /* shift right, insert new head */
        int max = GRID_COLS * GRID_ROWS - 1;
        if (s->length < max) {
            for (int i = s->length; i > 0; i--) s->body[i] = s->body[i-1];
            s->length++;
        } else {
            for (int i = s->length-1; i > 0; i--) s->body[i] = s->body[i-1];
        }
    } else {
        for (int i = s->length-1; i > 0; i--) s->body[i] = s->body[i-1];
    }

    /* compute new head */
    Point nh = s->body[1]; /* old head, now at [1] */
    switch(s->dir){
        case DIR_UP:    nh.y--; break;
        case DIR_DOWN:  nh.y++; break;
        case DIR_LEFT:  nh.x--; break;
        case DIR_RIGHT: nh.x++; break;
        default: break;
    }
    s->body[0] = nh;
    s->anim_t  = 0.0f;   /* reset lerp */
    return 1;
}

int snake_occupies(const Snake *s, Point p) {
    for (int i = 0; i < s->length; i++)
        if (s->body[i].x==p.x && s->body[i].y==p.y) return 1;
    return 0;
}

int snake_body_hit(const Snake *s, Point p) {
    for (int i = 1; i < s->length; i++)
        if (s->body[i].x==p.x && s->body[i].y==p.y) return 1;
    return 0;
}

void snake_shrink(Snake *s, int amount) {
    s->length -= amount;
    if (s->length < 2) s->length = 2;
}
