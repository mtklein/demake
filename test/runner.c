/* Headless mGBA driver: run a ROM with scripted input, screenshots, memory peeks.
 *
 *   runner ROM SCRIPT
 *
 * Script commands (one per line, # comments):
 *   wait N            run N frames
 *   hold KEYS N       hold comma-separated keys for N frames, then release
 *   shot PATH.ppm     write current frame as binary PPM
 *   peek ADDR N       hex-dump N bytes of bus memory to stdout
 *   msg TEXT          echo marker text to stdout
 */
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void log_cb(struct mLogger* l, int cat, enum mLogLevel level,
                   const char* fmt, va_list args) {
    (void)l; (void)cat;
    if (level & (mLOG_FATAL | mLOG_ERROR | mLOG_WARN | mLOG_INFO)) {
        printf("[mgba] ");
        vprintf(fmt, args);
        printf("\n");
    }
}
static struct mLogger logger = { .log = log_cb };

static uint32_t keybit(const char* name) {
    static const char* names[] = {"A","B","SELECT","START","RIGHT","LEFT","UP","DOWN","R","L"};
    for (int i = 0; i < 10; i++)
        if (!strcmp(name, names[i])) return 1u << i;
    fprintf(stderr, "unknown key '%s'\n", name);
    exit(1);
}

static uint32_t parse_keys(char* s) {
    uint32_t m = 0;
    for (char* tok = strtok(s, ","); tok; tok = strtok(NULL, ","))
        m |= keybit(tok);
    return m;
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: runner ROM SCRIPT\n"); return 1; }
    mLogSetDefaultLogger(&logger);

    struct mCore* core = mCoreFind(argv[1]);
    if (!core) { fprintf(stderr, "no core for %s\n", argv[1]); return 1; }
    core->init(core);
    mCoreConfigInit(&core->config, "runner");

    unsigned w = 240, h = 160;
    color_t* buf = malloc(w * h * sizeof(color_t));
    core->setVideoBuffer(core, buf, w);

    if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "load failed\n"); return 1; }
    core->reset(core);

    FILE* script = fopen(argv[2], "r");
    if (!script) { fprintf(stderr, "no script %s\n", argv[2]); return 1; }

    uint64_t frame = 0;
    char line[512];
    while (fgets(line, sizeof line, script)) {
        char* nl = strchr(line, '\n'); if (nl) *nl = 0;
        if (!line[0] || line[0] == '#') continue;
        char cmd[32], a1[256]; int n = 0;
        if (sscanf(line, "%31s", cmd) != 1) continue;

        if (!strcmp(cmd, "wait")) {
            sscanf(line, "%*s %d", &n);
            while (n-- > 0) { core->runFrame(core); frame++; }
        } else if (!strcmp(cmd, "hold")) {
            if (sscanf(line, "%*s %255s %d", a1, &n) < 2) n = 2;
            core->setKeys(core, parse_keys(a1));
            while (n-- > 0) { core->runFrame(core); frame++; }
            core->setKeys(core, 0);
        } else if (!strcmp(cmd, "shot")) {
            sscanf(line, "%*s %255s", a1);
            FILE* f = fopen(a1, "wb");
            if (!f) { fprintf(stderr, "can't write %s\n", a1); return 1; }
            fprintf(f, "P6\n%u %u\n255\n", w, h);
            for (unsigned i = 0; i < w * h; i++) {
                /* color_t is 32-bit; mgba native order: R in low byte */
                uint32_t c = buf[i];
                uint8_t px[3] = { c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF };
                fwrite(px, 1, 3, f);
            }
            fclose(f);
            printf("[shot] %s @frame %llu\n", a1, (unsigned long long)frame);
        } else if (!strcmp(cmd, "peek")) {
            unsigned addr = 0;
            sscanf(line, "%*s %x %d", &addr, &n);
            printf("[peek] %08x:", addr);
            for (int i = 0; i < n; i++)
                printf(" %02x", core->busRead8(core, addr + (unsigned)i));
            printf("\n");
        } else if (!strcmp(cmd, "msg")) {
            printf("[msg] %s\n", line + 4);
        } else {
            fprintf(stderr, "unknown cmd: %s\n", cmd);
            return 1;
        }
    }
    printf("[done] %llu frames\n", (unsigned long long)frame);
    fclose(script);
    core->deinit(core);
    free(buf);
    return 0;
}
