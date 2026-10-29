// Compare a replay trace with its baseline and report the first divergence.
// Usage: replaycmp <baseline> <trace> [--ignore-time]
#include <stdio.h>
#include <string.h>

typedef struct TraceLine
{
    unsigned present, time, frame;
    char state[17], screen[17], palette[17];
} TraceLine;

// Read one trace line; returns 0 at the end of the file.
static int ReadLine(FILE *fp, TraceLine *t)
{
    return fscanf(fp, "%u %u %u %16s %16s %16s", &t->present, &t->time, &t->frame, t->state, t->screen,
                  t->palette) == 6;
}

// Walk both traces in step and stop at the first differing column.
int main(int argc, char *argv[])
{
    FILE *base, *run;
    TraceLine b, r;
    int ignoreTime = argc > 3 && strcmp(argv[3], "--ignore-time") == 0;
    unsigned lines = 0;

    if (argc < 3 || (base = fopen(argv[1], "r")) == NULL || (run = fopen(argv[2], "r")) == NULL)
    {
        fprintf(stderr, "usage: replaycmp <baseline> <trace> [--ignore-time]\n");
        return 2;
    }
    for (;;)
    {
        int hasBase = ReadLine(base, &b), hasRun = ReadLine(run, &r);
        if (!hasBase || !hasRun)
        {
            if (hasBase != hasRun)
            {
                printf("traces differ in length: baseline %s after %u presents\n", hasBase ? "continues" : "ends",
                       lines);
                return 1;
            }
            break;
        }
        lines++;
        if (strcmp(b.state, r.state) != 0 || strcmp(b.screen, r.screen) != 0 ||
            strcmp(b.palette, r.palette) != 0 || b.frame != r.frame || (!ignoreTime && b.time != r.time))
        {
            printf("first divergence at present %u (baseline frame %u time %u, run frame %u time %u):%s%s%s%s%s\n",
                   b.present, b.frame, b.time, r.frame, r.time, strcmp(b.state, r.state) != 0 ? " state" : "",
                   strcmp(b.screen, r.screen) != 0 ? " screen" : "",
                   strcmp(b.palette, r.palette) != 0 ? " palette" : "", b.frame != r.frame ? " frame" : "",
                   (!ignoreTime && b.time != r.time) ? " time" : "");
            return 1;
        }
    }
    printf("identical: %u presents\n", lines);
    return 0;
}
