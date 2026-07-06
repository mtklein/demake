/* Host-side torture test of src/aeabi.c division vs native / and %. */
#include <stdio.h>
#include <stdint.h>
unsigned __aeabi_uidiv(unsigned n, unsigned d);
int __aeabi_idiv(int n, int d);
unsigned long long __aeabi_uidivmod(unsigned n, unsigned d);
unsigned long long __aeabi_idivmod(int n, int d);
int main(void) {
    unsigned long long checks = 0, fails = 0;
    /* dense small values: everything the game plausibly does */
    for (int n = -3000; n <= 3000; n++)
        for (int d = -64; d <= 64; d++) {
            if (!d) continue;
            if (__aeabi_idiv(n, d) != n / d) fails++;
            unsigned long long r = __aeabi_idivmod(n, d);
            if ((int)(r & 0xFFFFFFFFu) != n / d || (int)(r >> 32) != n % d) fails++;
            checks += 2;
        }
    /* unsigned sweeps incl. big values and edges */
    unsigned edges[] = {0,1,2,3,7,10,12,16,100,255,1000,65535,0x7FFFFFFF,0x80000000u,0xFFFFFFFFu};
    for (unsigned i = 0; i < sizeof edges/sizeof *edges; i++)
        for (unsigned j = 0; j < sizeof edges/sizeof *edges; j++) {
            unsigned n = edges[i], d = edges[j];
            if (!d) continue;
            if (__aeabi_uidiv(n, d) != n / d) fails++;
            unsigned long long r = __aeabi_uidivmod(n, d);
            if ((unsigned)(r & 0xFFFFFFFFu) != n / d || (unsigned)(r >> 32) != n % d) fails++;
            checks += 2;
        }
    /* INT_MIN edges */
    int imin = (int)0x80000000;
    if (__aeabi_idiv(imin, 1) != imin) fails++;
    if (__aeabi_idiv(imin, 2) != imin / 2) fails++;
    if (__aeabi_idiv(imin, -2) != imin / -2) fails++;
    checks += 3;
    printf("%llu checks, %llu failures\n", checks, fails);
    return fails != 0;
}
