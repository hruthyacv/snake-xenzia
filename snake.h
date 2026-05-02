#ifndef SNAKE_H
#define SNAKE_H
#include "defs.h"
void snake_init(Snake *s, int head_x, int head_y, Direction dir, int len, int is_ai);
void snake_set_dir(Snake *s, Direction d);
int  snake_step(Snake *s, int grow);       /* returns 1 ok, 0 out-of-bounds */
int  snake_occupies(const Snake *s, Point p);
int  snake_body_hit(const Snake *s, Point p); /* body only, skip head */
void snake_shrink(Snake *s, int amount);
Point snake_next_head(const Snake *s);
#endif
