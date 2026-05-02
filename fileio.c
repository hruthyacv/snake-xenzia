/*
 * fileio.c — Leaderboard persistence (binary file, sorted descending).
 */
#include "fileio.h"

#define MAGIC   0x534E4B32u   /* "SNK2" */
#define VERSION 2

void fileio_load(Leaderboard *lb)
{
    memset(lb, 0, sizeof(*lb));
    FILE *f = fopen(SCORES_FILE, "rb");
    if (!f) return;

    unsigned magic = 0; int ver = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != MAGIC) { fclose(f); return; }
    if (fread(&ver,   4, 1, f) != 1 || ver   != VERSION){ fclose(f); return; }
    if (fread(&lb->count, 4, 1, f) != 1) { fclose(f); return; }
    if (lb->count < 0 || lb->count > MAX_SCORES) { lb->count = 0; fclose(f); return; }
    fread(lb->entries, sizeof(ScoreEntry), (size_t)lb->count, f);
    fclose(f);
}

void fileio_save(const Leaderboard *lb)
{
    FILE *f = fopen(SCORES_FILE, "wb");
    if (!f) return;
    unsigned magic = MAGIC; int ver = VERSION;
    fwrite(&magic,     4, 1, f);
    fwrite(&ver,       4, 1, f);
    fwrite(&lb->count, 4, 1, f);
    if (lb->count > 0)
        fwrite(lb->entries, sizeof(ScoreEntry), (size_t)lb->count, f);
    fclose(f);
}

/* Returns 1-based rank on insertion, 0 if not in top MAX_SCORES. */
int fileio_insert(Leaderboard *lb, const ScoreEntry *e)
{
    int rank;
    for (rank = 0; rank < lb->count; rank++)
        if (e->score > lb->entries[rank].score) break;
    if (rank >= MAX_SCORES) return 0;

    int last = (lb->count < MAX_SCORES) ? lb->count : MAX_SCORES - 1;
    for (int i = last; i > rank; i--)
        lb->entries[i] = lb->entries[i-1];
    lb->entries[rank] = *e;
    if (lb->count < MAX_SCORES) lb->count++;
    return rank + 1;
}
