#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

static long ns(const struct timespec *a, const struct timespec *b)
{ return (b->tv_sec-a->tv_sec)*1000000000L + (b->tv_nsec-a->tv_nsec); }

int main(void)
{
    static char text[1024 * 1024 + 1];
    const char *pat = "needle42";
    const size_t plen = 8, wanted = 5461;
    uint8_t skip[256];
    memset(text, 'x', sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';
    for (size_t n = 0; n < wanted; n++)
        memcpy(text + n * ((sizeof(text) - 1) / wanted), pat, plen);
    for (size_t i = 0; i < 256; i++) skip[i] = (uint8_t)plen;
    for (size_t i = 0; i + 1 < plen; i++) skip[(unsigned char)pat[i]] = (uint8_t)(plen - 1 - i);
    volatile size_t sink = 0;
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    for (int n = 0; n < 100; n++) {
        const char *p = text;
        while ((p = strstr(p, pat)) != NULL) { sink++; p++; }
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    printf("strstr: %.3f ms/100 searches (matches=%zu)\n", ns(&a,&b)/1e6, sink);
    sink = 0;
    clock_gettime(CLOCK_MONOTONIC, &a);
    for (int n = 0; n < 100; n++) {
        size_t i = 0, len = sizeof(text) - 1;
        while (i + plen <= len) {
            size_t j = plen;
            while (j && text[i+j-1] == pat[j-1]) j--;
            if (!j) { sink++; i++; }
            else i += skip[(unsigned char)text[i+plen-1]];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    printf("BMH precompiled: %.3f ms/100 searches (matches=%zu)\n", ns(&a,&b)/1e6, sink);
    return 0;
}
