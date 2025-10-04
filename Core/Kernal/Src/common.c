#include "common.h"

void* ker_memset(void * buf, char c, size_t n)
{
    char * p = (char *)buf;
    while(n--)
    {
        *p++ = c;
    }
    return buf;
}

void* ker_memcpy(void* dst, const void* src, size_t n)
{
    char * d = (char *)dst;
    const char * s = (const char *)src;
    while(n--)
    {
        *d++ = *s++;
    }
    return dst;
}

char* ker_strcpy(char * dst, const char * src)
{
    char * d = dst;
    while((*d++ = *src++));
    return dst;
}

int ker_strcmp(const char * s1, const char * s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void ker_putchar(char ch);

__attribute__((weak)) void ker_putchar(char ch) {
    (void)ch;
}

void ker_printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++; // 跳过 '%'
            switch (*fmt) { // 读取下一个字符
                case '\0': // 当 '%' 作为格式字符串的末尾
                    ker_putchar('%');
                    goto end;
                case '%': // 打印 '%'
                    ker_putchar('%');
                    break;
                case 's': { // 打印以 NULL 结尾的字符串
                    const char *s = va_arg(vargs, const char *);
                    while (*s) {
                        ker_putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd': { // 以十进制打印整型
                    int value = va_arg(vargs, int);
                    unsigned magnitude = value; // https://github.com/nuta/operating-system-in-1000-lines/issues/64
                    if (value < 0) {
                        ker_putchar('-');
                        magnitude = -magnitude;
                    }

                    unsigned divisor = 1;
                    while (magnitude / divisor > 9)
                        divisor *= 10;

                    while (divisor > 0) {
                        ker_putchar('0' + magnitude / divisor);
                        magnitude %= divisor;
                        divisor /= 10;
                    }

                    break;
                }
                case 'x': { // 以十六进制打印整型
                    unsigned value = va_arg(vargs, unsigned);
                    for (int i = 7; i >= 0; i--) {
                        unsigned nibble = (value >> (i * 4)) & 0xf;
                        ker_putchar("0123456789abcdef"[nibble]);
                    }
                }
            }
        } else {
            ker_putchar(*fmt);
        }

        fmt++;
    }

end:
    va_end(vargs);
}


