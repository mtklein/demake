/* AEABI integer division for the ROM build.
 *
 * clang has no libgcc; these replace the only helpers the game ever pulled
 * from it (per the old rom.map: __aeabi_{u,}idiv{,mod} -- no 64-bit math, no
 * floats, no memcpy). ARM7TDMI has no divide instruction, so this is classic
 * shift-subtract long division; nothing here can recurse into a helper.
 *
 * ABI notes: the *divmod entry points return quotient in r0 and remainder in
 * r1. Under AAPCS a 64-bit return value places its low word in r0 and high
 * word in r1, which expresses exactly that in plain C. Division by zero
 * returns 0 (the game never divides by zero; no trap needed). */

unsigned __aeabi_uidiv(unsigned n, unsigned d) {
    if (d == 0) return 0;
    unsigned q = 0, bit = 1;
    while (!(d & 0x80000000u) && d < n) { d <<= 1; bit <<= 1; }
    while (bit) {
        if (n >= d) { n -= d; q |= bit; }
        d >>= 1; bit >>= 1;
    }
    return q;
}

int __aeabi_idiv(int n, int d) {
    unsigned un = n < 0 ? 0u - (unsigned)n : (unsigned)n;   /* INT_MIN-safe */
    unsigned ud = d < 0 ? 0u - (unsigned)d : (unsigned)d;
    unsigned q = __aeabi_uidiv(un, ud);
    return (n < 0) != (d < 0) ? -(int)q : (int)q;
}

unsigned long long __aeabi_uidivmod(unsigned n, unsigned d) {
    unsigned q = __aeabi_uidiv(n, d);
    return ((unsigned long long)(n - q * d) << 32) | q;
}

unsigned long long __aeabi_idivmod(int n, int d) {
    int q = __aeabi_idiv(n, d);
    return ((unsigned long long)(unsigned)(n - q * d) << 32) | (unsigned)q;
}

/* AEABI memory intrinsics: clang lowers large struct copies/zeroing (the
 * dice records inside R5Attack, EC spawns) to these instead of inlining.
 * Beware the ABI's argument order: __aeabi_memset is (dest, n, value) --
 * NOT the C memset order. The whole family is provided so future struct
 * growth can't surface a link error; unused members cost a few bytes.
 * -ffreestanding (implies -fno-builtin) keeps clang from collapsing these
 * loops back into calls to themselves. */

void __aeabi_memcpy(void* dest, const void* src, unsigned n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
}

void __aeabi_memcpy4(void* dest, const void* src, unsigned n) {
    unsigned* d = dest;
    const unsigned* s = src;
    while (n >= 4) { *d++ = *s++; n -= 4; }
    unsigned char* db = (unsigned char*)d;
    const unsigned char* sb = (const unsigned char*)s;
    while (n--) *db++ = *sb++;
}

void __aeabi_memcpy8(void* dest, const void* src, unsigned n) {
    __aeabi_memcpy4(dest, src, n);
}

void __aeabi_memset(void* dest, unsigned n, int c) {
    unsigned char* d = dest;
    while (n--) *d++ = (unsigned char)c;
}

void __aeabi_memset4(void* dest, unsigned n, int c) {
    unsigned v = (unsigned char)c;
    v |= v << 8; v |= v << 16;
    unsigned* d = dest;
    while (n >= 4) { *d++ = v; n -= 4; }
    unsigned char* db = (unsigned char*)d;
    while (n--) *db++ = (unsigned char)c;
}

void __aeabi_memset8(void* dest, unsigned n, int c) {
    __aeabi_memset4(dest, n, c);
}

void __aeabi_memclr(void* dest, unsigned n)  { __aeabi_memset(dest, n, 0); }
void __aeabi_memclr4(void* dest, unsigned n) { __aeabi_memset4(dest, n, 0); }
void __aeabi_memclr8(void* dest, unsigned n) { __aeabi_memset4(dest, n, 0); }
