#ifndef FILEIO_H
#define FILEIO_H
#include "defs.h"
void     fileio_load(Leaderboard *lb);
void     fileio_save(const Leaderboard *lb);
int      fileio_insert(Leaderboard *lb, const ScoreEntry *e);
#endif
