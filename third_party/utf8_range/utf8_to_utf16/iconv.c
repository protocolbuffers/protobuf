#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iconv.h>

static iconv_t s_cd;

/* Call iconv_open only once so the benchmark will be faster? */
static void __attribute__ ((constructor)) init_iconv(void)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    s_cd = iconv_open("UTF-16LE", "UTF-8");
#else
    s_cd = iconv_open("UTF-16BE", "UTF-8");
#endif
    if (s_cd == (iconv_t)-1) {
        perror("iconv_open");
        exit(1);
    }
}

/*
 * Parameters:
 * - buf8, len8: input utf-8 string
 * - buf16: buffer to store decoded utf-16 string
 * - *len16: on entry - utf-16 buffer length in bytes
 *           on exit  - length in bytes of valid decoded utf-16 string
 * Returns:
 *  -  0: success
 *  - >0: error position of input utf-8 string
 *  - -1: utf-16 buffer overflow
 * LE/BE depends on host
 */
int utf8_to16_iconv(const unsigned char *buf8, size_t len8,
        unsigned short *buf16, size_t *len16)
{
    size_t ret, len16_save = *len16;
    const unsigned char *buf8_0 = buf8;

    ret = iconv(s_cd, (char **)&buf8, &len8, (char **)&buf16, len16);

    *len16 = len16_save - *len16;

    if (ret != (size_t)-1)
        return 0;

    if (errno == E2BIG)
        return -1;              /* Output buffer full */

    return buf8 - buf8_0 + 1;   /* EILSEQ, EINVAL, error position */
}
