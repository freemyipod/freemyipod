#include <stdio.h> 
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

static void mark_bad(uint32_t* bitmap, int sector)
{
    bitmap[sector >> 5] &= ~(1 << (sector & 31));
}

static bool is_good(uint32_t* bitmap, int sector)
{
    return bitmap[sector >> 5] & (1 << (sector & 31));
}

int main(int argc, char** argv)
{
    int sectors;
    sscanf(argv[3], "%i", &sectors);
    size_t bitmapsize = ((sectors + 65535) & ~0xffff) >> 3;
    uint32_t* bitmap = malloc(bitmapsize);
    memset(bitmap, 0xff, bitmapsize);
    FILE* in = fopen(argv[1], "r");
    if (!in)
    {
        perror("Failed to open input file");
        return 2;
    }
    char line[256];
    while (fgets(line, sizeof(line), in))
    {
        int sector;
        if (sscanf(line, "%i", &sector) == 1 && sector) mark_bad(bitmap, sector);
    }
    fclose(in);
    int l0count = (sectors + 0x7ffff) >> 19;
    int bbtpages = 64 + ((l0count + 0x3f) & ~0x3f);
    int bbtsize;
    uint16_t (*bbt)[0x20];
    bool success = false;
    while (bbtpages <= 32832)
    {
        bbtsize = bbtpages << 6;
        l0count = (sectors - bbtsize + 0xfffff) >> 19;
        bbt = malloc(bbtsize);
        memset(bbt, 0, bbtsize);
        int logical = 0;
        int physical = 1;
        int remapbase;
        int bbtgood;
        for (bbtgood = 0; bbtgood < (bbtpages >> 6) - 1; physical++)
            if (is_good(bitmap, physical))
            {
                ((uint32_t*)bbt)[0x200 + bbtgood] = physical;
                bbtgood++;
            }
        int bbtused = 64 + l0count;
        int level;
        uint16_t *l1, *l2, *l3;
        int l1count = 0, l2count = 0, l3count = 0;
        while (!success)
        {
            if (!(logical & 0x7fff)) level = 0;
            else if (!(logical & 0x3ff)) level = 1;
            else if (!(logical & 0x1f)) level = 2;
            else level = 3;
            if (!level) remapbase = ((physical - logical) >> 12) << 12;
            if (physical - logical - remapbase > 0x7fff || remapbase > 0xffff)
            {
                printf("Need to remap across too high distance!\n");
                return 5;
            }
            int clean;
            for (clean = 0; clean < (1 << ((3 - level) * 5)) && physical + clean < sectors; clean++)
                if (!is_good(bitmap, physical + clean))
                    break;
            if (clean >= 32768)
            {
                bbt[64][(logical >> 15) << 1] = physical - logical - remapbase;
                bbt[64][((logical >> 15) << 1) | 1] = remapbase >> 12;
                physical += 32768;
                logical += 32768;
            }
            else if (clean >= 1024)
            {
                if (level == 0)
                {
                    if (bbtused == bbtpages) break;
                    level = 1;
                    l1 = bbt[bbtused];
                    bbt[64][(logical >> 15) << 1] = 0x8000 | (bbtused - 64);
                    bbt[64][((logical >> 15) << 1) | 1] = remapbase >> 12;
                    bbtused++;
                    l1count++;
                }
                l1[(logical >> 10) & 0x1f] = physical - logical - remapbase;
                physical += 1024;
                logical += 1024;
            }
            else if (clean >= 32)
            {
                if (level == 0)
                {
                    if (bbtused == bbtpages) break;
                    level = 1;
                    l1 = bbt[bbtused];
                    bbt[64][(logical >> 15) << 1] = 0x8000 | (bbtused - 64);
                    bbt[64][((logical >> 15) << 1) | 1] = remapbase >> 12;
                    bbtused++;
                    l1count++;
                }
                if (level == 1)
                {
                    if (bbtused == bbtpages) break;
                    level = 2;
                    l2 = bbt[bbtused];
                    l1[(logical >> 10) & 0x1f] = 0x8000 | (bbtused - 64);
                    bbtused++;
                    l2count++;
                }
                l2[(logical >> 5) & 0x1f] = physical - logical - remapbase;
                physical += 32;
                logical += 32;
            }
            else if (clean >= 1)
            {
                if (level == 0)
                {
                    if (bbtused == bbtpages) break;
                    level = 1;
                    l1 = bbt[bbtused];
                    bbt[64][(logical >> 15) << 1] = 0x8000 | (bbtused - 64);
                    bbt[64][((logical >> 15) << 1) | 1] = remapbase >> 12;
                    bbtused++;
                    l1count++;
                }
                if (level == 1)
                {
                    if (bbtused == bbtpages) break;
                    level = 2;
                    l2 = bbt[bbtused];
                    l1[(logical >> 10) & 0x1f] = 0x8000 | (bbtused - 64);
                    bbtused++;
                    l2count++;
                }
                if (level == 2)
                {
                    if (bbtused == bbtpages) break;
                    level = 3;
                    l3 = bbt[bbtused];
                    l2[(logical >> 5) & 0x1f] = 0x8000 | (bbtused - 64);
                    bbtused++;
                    l3count++;
                }
                l3[logical & 0x1f] = physical - logical - remapbase;
                physical++;
                logical++;
            }
            else physical++;
            if (physical >= sectors) success = true;
        }
        if (success)
        {        
            printf("BBT using %d sectors (%d KiB)\n", bbtgood + 1, bbtsize >> 10);
            printf("Level 0: %d pages, Level 1: %d pages, Level 2: %d pages, Level 3: %d pages\n",
                   l0count, l1count, l2count, l3count);
            printf("User data sectors: %d (%d KiB)\n", logical, logical << 2);
            memcpy(bbt, "emBIbbth", 8);
            ((uint32_t*)bbt)[0x1fc] = logical;
            ((uint32_t*)bbt)[0x1ff] = (bbtpages >> 6) - 1;
            break;
        }
        free(bbt);
        bbtpages += 64;
    }
    if (success)
    {
        int out = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0644);
        if (out < 0)
        {
            perror("Failed to open output file");
            return 3;
        }
        int rc;
        while (bbtsize && (rc = write(out, bbt, bbtsize)) >= 0) bbtsize -= rc;
        if (rc < 0)
        {
            perror("Failed to open output file");
            return 4;
        }
        close(out);
        return 0;
    }
    printf("Can't fit BBT into 2052 KiB!\n");
    return 1;
}
