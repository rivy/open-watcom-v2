/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2020-2020 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
* Description:  conversion tool to convert Japanese text files
*                  Shift-JIS Windows CP932 <-> UTF-8
*
****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool.h"


#define MAX_MB  1024
#define MARGIN  6

typedef struct cvt_chr {
    unsigned short  s;
    unsigned short  u;
} cvt_chr;

typedef int (*comp_fn)(const void *,const void *);

static cvt_chr cvt_table[] = {
    #define pick(s,u) {s, u },
    #include "cp932uni.h"
    #undef pick
};

/*
 * lead byte ranges
 * 0x81-0x9F
 * 0xE0-0xFC
 */

static int compare_sjis( const cvt_chr *p1, const cvt_chr *p2 )
{
    return( p1->s - p2->s );
}

static int compare_utf8( const cvt_chr *p1, const cvt_chr *p2 )
{
    return( p1->u - p2->u );
}

static size_t sjis_to_utf8( const char *src, char *dst )
{
    size_t      i;
    size_t      o;
    size_t      src_len;
    cvt_chr     x;
    cvt_chr     *p;

    src_len = strlen( src );
    o = 0;
    for( i = 0; i < src_len && o < MAX_MB - MARGIN; i++ ) {
        x.s = (unsigned char)src[i];
        if( x.s < 0x80 ) {
            dst[o++] = (char)x.s;
        } else {
            if( x.s > 0x80 && x.s < 0xA0 || x.s > 0xDF && x.s < 0xFD ) {
                x.s = x.s << 8 | (unsigned char)src[++i];
            }
            p = bsearch( &x, cvt_table, sizeof( cvt_table ) / sizeof( cvt_table[0] ), sizeof( cvt_table[0] ), (comp_fn)compare_sjis );
            if( p == NULL ) {
                printf( "unknown double-byte character: 0x%4X\n", x.s );
                dst[o++] = '?';
            } else {
                if( p->u > 0x7FF ) {
                    dst[o++] = (char)((p->u >> 12) | 0xE0);
                    dst[o++] = (char)(((p->u >> 6) & 0x3F) | 0x80);
                } else {
                    dst[o++] = (char)((p->u >> 6) | 0xC0);
                }
                dst[o++] = (char)((p->u & 0x3F) | 0x80);
            }
        }
    }
    dst[o] = '\0';
    return( o );
}

static size_t utf8_to_sjis( const char *src, char *dst )
{
    size_t      i;
    size_t      o;
    size_t      src_len;
    cvt_chr     x;
    cvt_chr     *p;

    src_len = strlen( src );
    o = 0;
    for( i = 0; i < src_len && o < MAX_MB - MARGIN; i++ ) {
        x.u = (unsigned char)src[i];
        if( x.u < 0x80 ) {
            dst[o++] = (char)x.u;
        } else {
            if( (x.u & 0xF0) == 0xE0 ) {
                 x.u &= 0x0F;
                 x.u = (x.u << 6) | ((unsigned char)src[++i] & 0x3F);
            } else {
                 x.u &= 0x1F;
            }
            x.u = (x.u << 6) | ((unsigned char)src[++i] & 0x3F);
            p = bsearch( &x, cvt_table, sizeof( cvt_table ) / sizeof( cvt_table[0] ), sizeof( cvt_table[0] ), (comp_fn)compare_utf8 );
            if( p == NULL ) {
                printf( "unknown unicode character: 0x%4X\n", x.u );
                dst[o++] = '?';
            } else {
                if( p->s > 0xFF ) {
                    dst[o++] = (char)(p->s >> 8);
                }
                dst[o++] = (char)p->s;
            }
        }
    }
    dst[o] = '\0';
    return( o );
}

static void usage( void )
{
    printf( "Usage: sjisutf8 -[sjis|utf8] <input file> <output file>\n" );
    printf( "    -sjis  convert utf-8 to sjis encoding\n" );
    printf( "    -utf8  convert sjis to utf-8 encoding\n" );
}

int main( int argc, char *argv[] )
{
    FILE    *fi;
    FILE    *fo;
    char    in_buff[MAX_MB];
    char    out_buff[MAX_MB];
    char    cvt_dir;
    size_t  in_len;

    /* unused parameters */ (void)argc;

    cvt_dir = '\0';
    ++argv;
    while( *argv != NULL && **argv == '-' ) {
        if( strcmp( argv[0] + 1, "sjis" ) == 0 ) {
            cvt_dir = 's';
        } else if( strcmp( argv[0] + 1, "utf8" ) == 0 ) {
            cvt_dir = 'u';
        } else {
            usage();
            printf( "\nOption '%s' not recognized.\n", *argv );
            return( 1 );
        }
        ++argv;
    }
    if( cvt_dir == '\0' ) {
        usage();
        printf( "\nOption -sjis or -utf8 must be specified.\n" );
        return( 1 );
    }
    if( *argv == NULL ) {
        usage();
        printf( "\nMissing input file name.\n" );
        return( 2 );
    }
    fi = fopen( *argv, "rb" );
    if( fi == NULL ) {
        printf( "Cannot open input file '%s'.\n", *argv );
        return( 3 );
    }
    ++argv;
    if( *argv == NULL ) {
        fclose( fi );
        usage();
        printf( "\nMissing output file name.\n" );
        return( 4 );
    }
    fo = fopen( *argv, "wb" );
    if( fo == NULL ) {
        printf( "Cannot open output file '%s'.\n", *argv );
        fclose( fi );
        return( 5 );
    }
    if( cvt_dir == 's' ) {
        qsort( cvt_table, sizeof( cvt_table ) / sizeof( cvt_table[0] ), sizeof( cvt_table[0] ), (comp_fn)compare_utf8 );
    }
    while( fgets( in_buff, sizeof( in_buff ), fi ) != NULL ) {
        in_len = strlen( in_buff );
        if( in_len ) {
            if( in_buff[in_len - 1] == '\r' || in_buff[in_len - 1] == 0x1A ) {
                in_buff[--in_len] = '\0';
            }
        }
        if( cvt_dir == 's' ) {
            utf8_to_sjis( in_buff, out_buff );
        } else {
            sjis_to_utf8( in_buff, out_buff );
        }
        fputs( out_buff, fo );
    }
    fclose( fi );
    fclose( fo );
    return( 0 );
}
