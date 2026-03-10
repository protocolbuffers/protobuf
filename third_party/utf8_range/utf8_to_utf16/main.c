#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

int utf8_to16_iconv(const unsigned char *buf8, size_t len8,
        unsigned short *buf16, size_t *len16);
int utf8_to16_naive(const unsigned char *buf8, size_t len8,
        unsigned short *buf16, size_t *len16);

static struct ftab {
    const char *name;
    int (*func)(const unsigned char *buf8, size_t len8,
            unsigned short *buf16, size_t *len16);
} ftab[] = {
    {
        .name = "iconv",
        .func = utf8_to16_iconv,
    }, {
        .name = "naive",
        .func = utf8_to16_naive,
    },
};

static unsigned char *load_test_buf(int len)
{
    const char utf8[] = "\xF0\x90\xBF\x80";
    const int utf8_len = sizeof(utf8)/sizeof(utf8[0]) - 1;

    unsigned char *data = malloc(len);
    unsigned char *p = data;

    while (len >= utf8_len) {
        memcpy(p, utf8, utf8_len);
        p += utf8_len;
        len -= utf8_len;
    }

    while (len--)
        *p++ = 0x7F;

    return data;
}

static unsigned char *load_test_file(int *len)
{
    unsigned char *data;
    int fd;
    struct stat stat;

    fd = open("../UTF-8-demo.txt", O_RDONLY);
    if (fd == -1) {
        printf("Failed to open ../UTF-8-demo.txt!\n");
        exit(1);
    }
    if (fstat(fd, &stat) == -1) {
        printf("Failed to get file size!\n");
        exit(1);
    }

    *len = stat.st_size;
    data = malloc(*len);
    if (read(fd, data, *len) != *len) {
        printf("Failed to read file!\n");
        exit(1);
    }

    close(fd);

    return data;
}

static void print_test(const unsigned char *data, int len)
{
    printf(" [len=%d] \"", len);
    while (len--)
        printf("\\x%02X", *data++);

    printf("\"\n");
}

struct test {
    const unsigned char *data;
    int len;
};

static void prepare_test_buf(unsigned char *buf, const struct test *pos,
                             int pos_len, int pos_idx)
{
    /* Round concatenate correct tokens to 1024 bytes */
    int buf_idx = 0;
    while (buf_idx < 1024) {
        int buf_len = 1024 - buf_idx;

        if (buf_len >= pos[pos_idx].len) {
            memcpy(buf+buf_idx, pos[pos_idx].data, pos[pos_idx].len);
            buf_idx += pos[pos_idx].len;
        } else {
            memset(buf+buf_idx, 0, buf_len);
            buf_idx += buf_len;
        }

        if (++pos_idx == pos_len)
            pos_idx = 0;
    }
}

/* Return 0 on success, -1 on error */
static int test_manual(const struct ftab *ftab, unsigned short *buf16,
        unsigned short *_buf16)
{
#define LEN16   4096

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-sign"
    /* positive tests */
    static const struct test pos[] = {
        {"", 0},
        {"\x00", 1},
        {"\x66", 1},
        {"\x7F", 1},
        {"\x00\x7F", 2},
        {"\x7F\x00", 2},
        {"\xC2\x80", 2},
        {"\xDF\xBF", 2},
        {"\xE0\xA0\x80", 3},
        {"\xE0\xA0\xBF", 3},
        {"\xED\x9F\x80", 3},
        {"\xEF\x80\xBF", 3},
        {"\xF0\x90\xBF\x80", 4},
        {"\xF2\x81\xBE\x99", 4},
        {"\xF4\x8F\x88\xAA", 4},
    };

    /* negative tests */
    static const struct test neg[] = {
        {"\x80", 1},
        {"\xBF", 1},
        {"\xC0\x80", 2},
        {"\xC1\x00", 2},
        {"\xC2\x7F", 2},
        {"\xDF\xC0", 2},
        {"\xE0\x9F\x80", 3},
        {"\xE0\xC2\x80", 3},
        {"\xED\xA0\x80", 3},
        {"\xED\x7F\x80", 3},
        {"\xEF\x80\x00", 3},
        {"\xF0\x8F\x80\x80", 4},
        {"\xF0\xEE\x80\x80", 4},
        {"\xF2\x90\x91\x7F", 4},
        {"\xF4\x90\x88\xAA", 4},
        {"\xF4\x00\xBF\xBF", 4},
        {"\x00\x00\x00\x00\x00\xC2\x80\x00\x00\x00\xE1\x80\x80\x00\x00\xC2" \
         "\xC2\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
         32},
        {"\x00\x00\x00\x00\x00\xC2\xC2\x80\x00\x00\xE1\x80\x80\x00\x00\x00",
         16},
        {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF1\x80",
         32},
        {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF1",
         32},
        {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF1\x80" \
         "\x80", 33},
        {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF1\x80" \
         "\xC2\x80", 34},
        {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF0" \
         "\x80\x80\x80", 35},
    };
#pragma GCC diagnostic push

    size_t len16 = LEN16, _len16 = LEN16;
    int ret, _ret;

    /* Test single token */
    for (int i = 0; i < sizeof(pos)/sizeof(pos[0]); ++i) {
        ret = ftab->func(pos[i].data, pos[i].len, buf16, &len16);
        _ret = utf8_to16_iconv(pos[i].data, pos[i].len, _buf16, &_len16);
        if (ret != _ret || len16 != _len16 || memcmp(buf16, _buf16, len16)) {
            printf("FAILED positive test(%d:%d, %lu:%lu): ",
                    ret, _ret, len16, _len16);
            print_test(pos[i].data, pos[i].len);
            return -1;
        }
        len16 = _len16 = LEN16;
    }
    for (int i = 0; i < sizeof(neg)/sizeof(neg[0]); ++i) {
        ret = ftab->func(neg[i].data, neg[i].len, buf16, &len16);
        _ret = utf8_to16_iconv(neg[i].data, neg[i].len, _buf16, &_len16);
        if (ret != _ret || len16 != _len16 || memcmp(buf16, _buf16, len16)) {
            printf("FAILED negative test(%d:%d, %lu:%lu): ",
                    ret, _ret, len16, _len16);
            print_test(neg[i].data, neg[i].len);
            return -1;
        }
        len16 = _len16 = LEN16;
    }

    /* Test shifted buffer to cover 1k length */
    /* buffer size must be greater than 1024 + 16 + max(test string length) */
    const int max_size = 1024*2;
    uint64_t buf64[max_size/8 + 2];
    /* Offset 8 bytes by 1 byte */
    unsigned char *buf = ((unsigned char *)buf64) + 1;
    int buf_len;

    for (int i = 0; i < sizeof(pos)/sizeof(pos[0]); ++i) {
        /* Positive test: shift 16 bytes, validate each shift */
        prepare_test_buf(buf, pos, sizeof(pos)/sizeof(pos[0]), i);
        buf_len = 1024;
        for (int j = 0; j < 16; ++j) {
            ret = ftab->func(buf, buf_len, buf16, &len16);
            _ret = utf8_to16_iconv(buf, buf_len, _buf16, &_len16);
            if (ret != _ret || len16 != _len16 || \
                    memcmp(buf16, _buf16, len16)) {
                printf("FAILED positive test(%d:%d, %lu:%lu): ",
                        ret, _ret, len16, _len16);
                print_test(buf, buf_len);
                return -1;
            }
            len16 = _len16 = LEN16;
            for (int k = buf_len; k >= 1; --k)
                buf[k] = buf[k-1];
            buf[0] = '\x55';
            ++buf_len;
        }

        /* Negative test: trunk last non ascii */
        while (buf_len >= 1 && buf[buf_len-1] <= 0x7F)
            --buf_len;
        if (buf_len) {
            ret = ftab->func(buf, buf_len-1, buf16, &len16);
            _ret = utf8_to16_iconv(buf, buf_len-1, _buf16, &_len16);
            if (ret != _ret || len16 != _len16 || \
                    memcmp(buf16, _buf16, len16)) {
                printf("FAILED negative test(%d:%d, %lu:%lu): ",
                        ret, _ret, len16, _len16);
                print_test(buf, buf_len-1);
                return -1;
            }
            len16 = _len16 = LEN16;
        }
    }

    /* Negative test */
    for (int i = 0; i < sizeof(neg)/sizeof(neg[0]); ++i) {
        /* Append one error token, shift 16 bytes, validate each shift */
        int pos_idx = i % (sizeof(pos)/sizeof(pos[0]));
        prepare_test_buf(buf, pos, sizeof(pos)/sizeof(pos[0]), pos_idx);
        memcpy(buf+1024, neg[i].data, neg[i].len);
        buf_len = 1024 + neg[i].len;
        for (int j = 0; j < 16; ++j) {
            ret = ftab->func(buf, buf_len, buf16, &len16);
            _ret = utf8_to16_iconv(buf, buf_len, _buf16, &_len16);
            if (ret != _ret || len16 != _len16 || \
                    memcmp(buf16, _buf16, len16)) {
                printf("FAILED negative test(%d:%d, %lu:%lu): ",
                        ret, _ret, len16, _len16);
                print_test(buf, buf_len);
                return -1;
            }
            len16 = _len16 = LEN16;
            for (int k = buf_len; k >= 1; --k)
                buf[k] = buf[k-1];
            buf[0] = '\x66';
            ++buf_len;
        }
    }

    return 0;
}

static void test(const unsigned char *buf8, size_t len8,
        unsigned short *buf16, size_t len16, const struct ftab *ftab)
{
    /* Use iconv as the reference answer */
    if (strcmp(ftab->name, "iconv") == 0)
        return;

    printf("%s\n", ftab->name);

    /* Test file or buffer */
    size_t _len16 = len16;
    unsigned short *_buf16 = (unsigned short *)malloc(_len16);
    if (utf8_to16_iconv(buf8, len8, _buf16, &_len16)) {
        printf("Invalid test file or buffer!\n");
        exit(1);
    }
    printf("standard test: ");
    if (ftab->func(buf8, len8, buf16, &len16) || len16 != _len16 || \
            memcmp(buf16, _buf16, len16) != 0)
        printf("FAIL\n");
    else
        printf("pass\n");
    free(_buf16);

    /* Manual cases */
    unsigned short *mbuf8 = (unsigned short *)malloc(LEN16);
    unsigned short *mbuf16 = (unsigned short *)malloc(LEN16);
    printf("manual test: %s\n",
            test_manual(ftab, mbuf8, mbuf16) ? "FAIL" : "pass");
    free(mbuf8);
    free(mbuf16);
    printf("\n");
}

static void bench(const unsigned char *buf8, size_t len8,
        unsigned short *buf16, size_t len16, const struct ftab *ftab)
{
    const int loops = 1024*1024*1024/len8;
    int ret = 0;
    double time, size;
    struct timeval tv1, tv2;

    fprintf(stderr, "bench %s... ", ftab->name);
    gettimeofday(&tv1, 0);
    for (int i = 0; i < loops; ++i)
        ret |= ftab->func(buf8, len8, buf16, &len16);
    gettimeofday(&tv2, 0);
    printf("%s\n", ret?"FAIL":"pass");

    time = tv2.tv_usec - tv1.tv_usec;
    time = time / 1000000 + tv2.tv_sec - tv1.tv_sec;
    size = ((double)len8 * loops) / (1024*1024);
    printf("time: %.4f s\n", time);
    printf("data: %.0f MB\n", size);
    printf("BW: %.2f MB/s\n", size / time);
    printf("\n");
}

static void usage(const char *bin)
{
    printf("Usage:\n");
    printf("%s test  [alg]     ==> test all or one algorithm\n", bin);
    printf("%s bench [alg]     ==> benchmark all or one algorithm\n", bin);
    printf("%s bench size NUM  ==> benchmark with specific buffer size\n", bin);
    printf("alg = ");
    for (int i = 0; i < sizeof(ftab)/sizeof(ftab[0]); ++i)
        printf("%s ", ftab[i].name);
    printf("\nNUM = buffer size in bytes, 1 ~ 67108864(64M)\n");
}

int main(int argc, char *argv[])
{
    int len8 = 0, len16;
    unsigned char *buf8;
    unsigned short *buf16;
    const char *alg = NULL;
    void (*tb)(const unsigned char *buf8, size_t len8,
           unsigned short *buf16, size_t len16, const struct ftab *ftab);

    tb = NULL;
    if (argc >= 2) {
        if (strcmp(argv[1], "test") == 0)
            tb = test;
        else if (strcmp(argv[1], "bench") == 0)
            tb = bench;
        if (argc >= 3) {
            alg = argv[2];
            if (strcmp(alg, "size") == 0) {
                if (argc < 4) {
                    tb = NULL;
                } else {
                    alg = NULL;
                    len8 = atoi(argv[3]);
                    if (len8 <= 0 || len8 > 67108864) {
                        printf("Buffer size error!\n\n");
                        tb = NULL;
                    }
                }
            }
        }
    }

    if (tb == NULL) {
        usage(argv[0]);
        return 1;
    }

    /* Load UTF8 test buffer */
    if (len8)
        buf8 = load_test_buf(len8);
    else
        buf8 = load_test_file(&len8);

    /* Prepare UTF16 buffer large enough */
    len16 = len8 * 2;
    buf16 = (unsigned short *)malloc(len16);

    if (tb == bench)
        printf("============== Bench UTF8 (%d bytes) ==============\n", len8);
    for (int i = 0; i < sizeof(ftab)/sizeof(ftab[0]); ++i) {
        if (alg && strcmp(alg, ftab[i].name) != 0)
            continue;
        tb((const unsigned char *)buf8, len8, buf16, len16, &ftab[i]);
    }

#if 0
    if (tb == bench) {
        printf("==================== Bench ASCII ====================\n");
        /* Change test buffer to ascii */
        for (int i = 0; i < len; i++)
            data[i] &= 0x7F;

        for (int i = 0; i < sizeof(ftab)/sizeof(ftab[0]); ++i) {
            if (alg && strcmp(alg, ftab[i].name) != 0)
                continue;
            tb((const unsigned char *)data, len, &ftab[i]);
            printf("\n");
        }
    }
#endif

    return 0;
}
