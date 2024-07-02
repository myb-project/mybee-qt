#include <QRandomGenerator>
#include <QFile>
#include <QtDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#else
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define MAX_COMMENT_SIZE 100

/* --- ed25519 crypto based on TweetNaCl tweetnacl.c 20140427 */

#define FOR(i,n) for (i = 0;i < n;++i)

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef i64 gf[16];

static const gf
    D2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0, 0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406},
    X = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c, 0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169},
    Y = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666};

static u64 dl64(const u8 *x) {
    u64 i,u=0;
    FOR(i,8) u=(u<<8)|x[i];
    return u;
}

static void ts64(u8 *x,u64 u) {
    int i;
    for (i = 7;i >= 0;--i) { x[i] = u; u >>= 8; }
}

static void set25519(gf r, const gf a) {
    int i;
    FOR(i,16) r[i]=a[i];
}

static void car25519(gf o) {
    int i;
    i64 c;
    FOR(i,16) {
        o[i]+=1<<16;
        c=o[i]>>16;
        o[(i+1)*(i<15)]+=c-1+37*(c-1)*(i==15);
        o[i]-=c<<16;
    }
}

static void sel25519(gf p,gf q,int b) {
    i64 t,i,c=~(b-1);
    FOR(i,16) {
        t= c&(p[i]^q[i]);
        p[i]^=t;
        q[i]^=t;
    }
}

static void pack25519(u8 *o,const gf n) {
    int i,j,b;
    gf m,t;
    FOR(i,16) t[i]=n[i];
    car25519(t);
    car25519(t);
    car25519(t);
    FOR(j,2) {
        m[0]=t[0]-0xffed;
        for(i=1;i<15;i++) {
            m[i]=t[i]-0xffff-((m[i-1]>>16)&1);
            m[i-1]&=0xffff;
        }
        m[15]=t[15]-0x7fff-((m[14]>>16)&1);
        b=(m[15]>>16)&1;
        m[14]&=0xffff;
        sel25519(t,m,1-b);
    }
    FOR(i,16) {
        o[2*i]=t[i]&0xff;
        o[2*i+1]=t[i]>>8;
    }
}

static u8 par25519(const gf a) {
    u8 d[32];
    pack25519(d,a);
    return d[0]&1;
}

static void A(gf o,const gf a,const gf b) {
    int i;
    FOR(i,16) o[i]=a[i]+b[i];
}

static void Z(gf o,const gf a,const gf b) {
    int i;
    FOR(i,16) o[i]=a[i]-b[i];
}

static void M(gf o,const gf a,const gf b) {
    i64 i,j,t[31];
    FOR(i,31) t[i]=0;
    FOR(i,16) FOR(j,16) t[i+j]+=a[i]*b[j];
    FOR(i,15) t[i]+=38*t[i+16];
    FOR(i,16) o[i]=t[i];
    car25519(o);
    car25519(o);
}

static void S(gf o,const gf a) {
    M(o,a,a);
}

static void inv25519(gf o,const gf i) {
    gf c;
    int a;
    FOR(a,16) c[a]=i[a];
    for(a=253;a>=0;a--) {
        S(c,c);
        if(a!=2&&a!=4) M(c,c,i);
    }
    FOR(a,16) o[a]=c[a];
}

static u64 R(u64 x,int c) { return (x >> c) | (x << (64 - c)); }
static u64 Ch(u64 x,u64 y,u64 z) { return (x & y) ^ (~x & z); }
static u64 Maj(u64 x,u64 y,u64 z) { return (x & y) ^ (x & z) ^ (y & z); }
static u64 Sigma0(u64 x) { return R(x,28) ^ R(x,34) ^ R(x,39); }
static u64 Sigma1(u64 x) { return R(x,14) ^ R(x,18) ^ R(x,41); }
static u64 sigma0(u64 x) { return R(x, 1) ^ R(x, 8) ^ (x >> 7); }
static u64 sigma1(u64 x) { return R(x,19) ^ R(x,61) ^ (x >> 6); }

/* ...ULL constant without triggering gcc -Wlong-long */
#define ULLC(hi, lo) (((u64)(hi)) << 32 | (lo))

static const u64 K[80] =  {
    ULLC(0x428a2f98U,0xd728ae22U), ULLC(0x71374491U,0x23ef65cdU), ULLC(0xb5c0fbcfU,0xec4d3b2fU), ULLC(0xe9b5dba5U,0x8189dbbcU),
    ULLC(0x3956c25bU,0xf348b538U), ULLC(0x59f111f1U,0xb605d019U), ULLC(0x923f82a4U,0xaf194f9bU), ULLC(0xab1c5ed5U,0xda6d8118U),
    ULLC(0xd807aa98U,0xa3030242U), ULLC(0x12835b01U,0x45706fbeU), ULLC(0x243185beU,0x4ee4b28cU), ULLC(0x550c7dc3U,0xd5ffb4e2U),
    ULLC(0x72be5d74U,0xf27b896fU), ULLC(0x80deb1feU,0x3b1696b1U), ULLC(0x9bdc06a7U,0x25c71235U), ULLC(0xc19bf174U,0xcf692694U),
    ULLC(0xe49b69c1U,0x9ef14ad2U), ULLC(0xefbe4786U,0x384f25e3U), ULLC(0x0fc19dc6U,0x8b8cd5b5U), ULLC(0x240ca1ccU,0x77ac9c65U),
    ULLC(0x2de92c6fU,0x592b0275U), ULLC(0x4a7484aaU,0x6ea6e483U), ULLC(0x5cb0a9dcU,0xbd41fbd4U), ULLC(0x76f988daU,0x831153b5U),
    ULLC(0x983e5152U,0xee66dfabU), ULLC(0xa831c66dU,0x2db43210U), ULLC(0xb00327c8U,0x98fb213fU), ULLC(0xbf597fc7U,0xbeef0ee4U),
    ULLC(0xc6e00bf3U,0x3da88fc2U), ULLC(0xd5a79147U,0x930aa725U), ULLC(0x06ca6351U,0xe003826fU), ULLC(0x14292967U,0x0a0e6e70U),
    ULLC(0x27b70a85U,0x46d22ffcU), ULLC(0x2e1b2138U,0x5c26c926U), ULLC(0x4d2c6dfcU,0x5ac42aedU), ULLC(0x53380d13U,0x9d95b3dfU),
    ULLC(0x650a7354U,0x8baf63deU), ULLC(0x766a0abbU,0x3c77b2a8U), ULLC(0x81c2c92eU,0x47edaee6U), ULLC(0x92722c85U,0x1482353bU),
    ULLC(0xa2bfe8a1U,0x4cf10364U), ULLC(0xa81a664bU,0xbc423001U), ULLC(0xc24b8b70U,0xd0f89791U), ULLC(0xc76c51a3U,0x0654be30U),
    ULLC(0xd192e819U,0xd6ef5218U), ULLC(0xd6990624U,0x5565a910U), ULLC(0xf40e3585U,0x5771202aU), ULLC(0x106aa070U,0x32bbd1b8U),
    ULLC(0x19a4c116U,0xb8d2d0c8U), ULLC(0x1e376c08U,0x5141ab53U), ULLC(0x2748774cU,0xdf8eeb99U), ULLC(0x34b0bcb5U,0xe19b48a8U),
    ULLC(0x391c0cb3U,0xc5c95a63U), ULLC(0x4ed8aa4aU,0xe3418acbU), ULLC(0x5b9cca4fU,0x7763e373U), ULLC(0x682e6ff3U,0xd6b2b8a3U),
    ULLC(0x748f82eeU,0x5defb2fcU), ULLC(0x78a5636fU,0x43172f60U), ULLC(0x84c87814U,0xa1f0ab72U), ULLC(0x8cc70208U,0x1a6439ecU),
    ULLC(0x90befffaU,0x23631e28U), ULLC(0xa4506cebU,0xde82bde9U), ULLC(0xbef9a3f7U,0xb2c67915U), ULLC(0xc67178f2U,0xe372532bU),
    ULLC(0xca273eceU,0xea26619cU), ULLC(0xd186b8c7U,0x21c0c207U), ULLC(0xeada7dd6U,0xcde0eb1eU), ULLC(0xf57d4f7fU,0xee6ed178U),
    ULLC(0x06f067aaU,0x72176fbaU), ULLC(0x0a637dc5U,0xa2c898a6U), ULLC(0x113f9804U,0xbef90daeU), ULLC(0x1b710b35U,0x131c471bU),
    ULLC(0x28db77f5U,0x23047d84U), ULLC(0x32caab7bU,0x40c72493U), ULLC(0x3c9ebe0aU,0x15c9bebcU), ULLC(0x431d67c4U,0x9c100d4cU),
    ULLC(0x4cc5d4beU,0xcb3e42b6U), ULLC(0x597f299cU,0xfc657e2aU), ULLC(0x5fcb6fabU,0x3ad6faecU), ULLC(0x6c44198cU,0x4a475817U),
};

static int crypto_hashblocks(u8 *x,const u8 *m,u64 n) {
    u64 z[8],b[8],a[8],w[16],t;
    int i,j;

    FOR(i,8) z[i] = a[i] = dl64(x + 8 * i);

    while (n >= 128) {
        FOR(i,16) w[i] = dl64(m + 8 * i);

        FOR(i,80) {
            FOR(j,8) b[j] = a[j];
            t = a[7] + Sigma1(a[4]) + Ch(a[4],a[5],a[6]) + K[i] + w[i%16];
            b[7] = t + Sigma0(a[0]) + Maj(a[0],a[1],a[2]);
            b[3] += t;
            FOR(j,8) a[(j+1)%8] = b[j];
            if (i%16 == 15)
                FOR(j,16)
            w[j] += w[(j+9)%16] + sigma0(w[(j+1)%16]) + sigma1(w[(j+14)%16]);
        }

        FOR(i,8) { a[i] += z[i]; z[i] = a[i]; }

        m += 128;
        n -= 128;
    }

    FOR(i,8) ts64(x+8*i,z[i]);

    return n;
}

__attribute__((aligned(1))) static const u8 iv[64] = {
    0x6a,0x09,0xe6,0x67,0xf3,0xbc,0xc9,0x08,
    0xbb,0x67,0xae,0x85,0x84,0xca,0xa7,0x3b,
    0x3c,0x6e,0xf3,0x72,0xfe,0x94,0xf8,0x2b,
    0xa5,0x4f,0xf5,0x3a,0x5f,0x1d,0x36,0xf1,
    0x51,0x0e,0x52,0x7f,0xad,0xe6,0x82,0xd1,
    0x9b,0x05,0x68,0x8c,0x2b,0x3e,0x6c,0x1f,
    0x1f,0x83,0xd9,0xab,0xfb,0x41,0xbd,0x6b,
    0x5b,0xe0,0xcd,0x19,0x13,0x7e,0x21,0x79,
};

static int crypto_hash(u8 *out,const u8 *m,u64 n) {
    u8 h[64],x[256];
    u64 i,b = n;

    FOR(i,64) h[i] = iv[i];

    crypto_hashblocks(h,m,n);
    m += n;
    n &= 127;
    m -= n;

    FOR(i,256) x[i] = 0;
    FOR(i,n) x[i] = m[i];
    x[n] = 128;

    n = 256-128*(n<112);
    x[n-9] = b >> 61;
    ts64(x+n-8,b<<3);
    crypto_hashblocks(h,x,n);

    FOR(i,64) out[i] = h[i];

    return 0;
}

static void add(gf p[4],gf q[4]) {
    gf a,b,c,d,t,e,f,g,h;

    Z(a, p[1], p[0]);
    Z(t, q[1], q[0]);
    M(a, a, t);
    A(b, p[0], p[1]);
    A(t, q[0], q[1]);
    M(b, b, t);
    M(c, p[3], q[3]);
    M(c, c, D2);
    M(d, p[2], q[2]);
    A(d, d, d);
    Z(e, b, a);
    Z(f, d, c);
    A(g, d, c);
    A(h, b, a);

    M(p[0], e, f);
    M(p[1], h, g);
    M(p[2], g, f);
    M(p[3], e, h);
}

static void cswap(gf p[4],gf q[4],u8 b) {
    int i;
    FOR(i,4)
    sel25519(p[i],q[i],b);
}

static void pack(u8 *r,gf p[4]) {
    gf tx, ty, zi;
    inv25519(zi, p[2]);
    M(tx, p[0], zi);
    M(ty, p[1], zi);
    pack25519(r, ty);
    r[31] ^= par25519(tx) << 7;
}

static void scalarmult(gf p[4],gf q[4],const u8 *s) {
    int i;
    memset(p, 0, 128 * 4);
#if defined(__i386__) || defined(__amd64__)
    *(char*)(&p[1][0]) |= 1;  /* gcc-4.8.4 is not smart enough to optimize this. */
    *(char*)(&p[2][0]) |= 1;
#else
    p[1][0] |= 1;
    p[2][0] |= 1;
#endif
    for (i = 255;i >= 0;--i) {
        u8 b = (s[i/8]>>(i&7))&1;
        cswap(p,q,b);
        add(q,p);
        add(p,p);
        cswap(p,q,b);
    }
}

static void scalarbase(gf p[4],const u8 *s) {
    gf q[4];
    memset(q, 0, sizeof(q));
    set25519(q[0],X);
    set25519(q[1],Y);
#if defined(__i386__) || defined(__amd64__)
    *(char*)(&q[2][0]) |= 1;  /* gcc-4.8.4 is not smart enough to optimize this. */
#else
    q[2][0] |= 1;
#endif
    M(q[3],X,Y);
    scalarmult(p,q,s);
}

static void keypair(unsigned char *pk, const unsigned char *sk) {
    u8 h[64];
    gf p[4];
    /* SHA-512 with 64 bytes of output in h, only the first 32 bytes are used. */
    crypto_hash(h, sk, 32);
    h[0] &= 248;
    h[31] &= 63;
    h[31] |= 64;
    scalarbase(p, h);
    pack(pk, p);
}

static void generate_seed(u8 *a, u32 size) {
    bool ok = false;
#ifdef Q_OS_WIN
    HCRYPTPROV prov;
    if (CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        ok = CryptGenRandom(prov, size, (BYTE*)a));
        CryptReleaseContext(prov, 0);
    }
#else
    int fd = ::open("/dev/urandom", O_RDONLY, 0);
    if (fd != -1) {
        ok = (::read(fd, a, size) == size);
        ::close(fd);
    }
#endif
    if (!ok) {
        for (uint i = 0; i < size; i += sizeof(quint32)) {
            *(reinterpret_cast<quint32*>(a[i])) = QRandomGenerator::global()->generate();
        }
    }
}

static char *append(char *p, char *pend, const char *input, u32 input_size) {
    if (input_size > pend - p + 0U) {
        qWarning() << Q_FUNC_INFO << "append too long";
        return p;
    }
    memcpy(p, input, input_size);
    return p + input_size;
}

static const char b64chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

u32 base64encode_size(u32 size) {
    return ((size + 2) / 3 * 4) + 1;  /* TODO(pts): Handle overflow. */
}

static char *base64encode(char *p, char *pend, const u8 *input, u32 input_size) {
    u32 i;
    if (base64encode_size(input_size) > pend - p + 0U) {
        qWarning() << Q_FUNC_INFO << "base64 too long";
        return p;
    }
    for (i = 0; i + 2 < input_size; i += 3) {  /* TODO(pts): Handle overflow. */
        *p++ = b64chars[(input[i] >> 2) & 0x3F];
        *p++ = b64chars[((input[i] & 0x3) << 4) |
                        ((int) (input[i + 1] & 0xF0) >> 4)];
        *p++ = b64chars[((input[i + 1] & 0xF) << 2) |
                        ((int) (input[i + 2] & 0xC0) >> 6)];
        *p++ = b64chars[input[i + 2] & 0x3F];
    }
    if (i < input_size) {
        *p++ = b64chars[(input[i] >> 2) & 0x3F];
        if (i == (input_size - 1)) {
            *p++ = b64chars[((input[i] & 0x3) << 4)];
            *p++ = '=';
        } else {
            *p++ = b64chars[((input[i] & 0x3) << 4) |
                            ((int) (input[i + 1] & 0xF0) >> 4)];
            *p++ = b64chars[((input[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }
    return p;
}

static const char c_kprefix[62] =
#if 0
    "openssh-key-v1\0\0\0\0\4none\0\0\0\4none\0\0\0\0\0\0\0\1\0\0\0\x33"
    "\0\0\0\x0bssh-ed25519\0\0\0 ";  /* 19 bytes in this line. */
#else  /* Avoid g++ warning: initializer-string for array of chars is too long. */
    {'o','p','e','n','s','s','h','-','k','e','y','-','v','1','\0','\0','\0','\0','\4','n','o','n','e','\0','\0','\0','\4','n','o','n','e','\0','\0','\0','\0','\0','\0','\0','\1','\0','\0','\0','\x33','\0','\0','\0','\x0b','s','s','h','-','e','d','2','5','5','1','9','\0','\0','\0',' '};
#endif

static char *build_openssh_public_key_ed25519(char *p, char *pend, const u8 *public_key,
                                              const char *comment, u32 comment_size) {
    u8 ubuf[19 + 32];
    static const char eprefix[12] =
#if 0
      "ssh-ed25519 ";
#else
        {'s','s','h','-','e','d','2','5','5','1','9',' '};
#endif
    static const char newline[1] = {'\n'};
    memcpy(ubuf, c_kprefix + 62 - 19, 19);
    memcpy(ubuf + 19, public_key, 32);
    p = append(p, pend, eprefix, 12);
    p = base64encode(p, pend, ubuf, 19 + 32);
    p = append(p, pend, eprefix + 11, 1);  /* Space. */
    p = append(p, pend, comment, comment_size);
    p = append(p, pend, newline, 1);
    return p;
}

static char *add_u32be(char *p, char *pend, u32 v) {
    if (4 > pend - p + 0U) {
        qWarning() << Q_FUNC_INFO << "add_u32be too long";
        return p;
    }
    *p++ = v >> 24;
    *p++ = v >> 16;
    *p++ = v >> 8;
    *p++ = v;
    return p;
}

/* Input: [p, p + size).
 * Output [p, p + size + (size + line_size - 1) / line_size), always with a
 *        trailing newline.
 */
static char *split_lines(char *p, char *pend, u32 size, u32 line_size) {
    const u32 d = (size + line_size - 1) / line_size;
    char *q, *r, *psize;
    u32 i;
    if (size + d > pend - p + 0U) {
        qWarning() << Q_FUNC_INFO << "split_lines too long";
        return p;
    }
    for (i = size; i-- > 0;) {
        p[i + d] = p[i];
    }
    q = p + d;
    r = p + line_size;
    psize = q + size - 1;
    while (p != psize) {
        if (p == r) {
            *p++ = '\n';
            r = p + line_size;
        }
        *p++ = *q++;
    }
    *p++ = '\n';
    return p;
}

static char *build_openssh_private_key_ed25519(char *p, char *pend, const u8 *public_key,
                                               const char *comment, u32 comment_size,
                                               const u8 *private_key, const u8 *checkstr) {
    __attribute__((aligned(1))) static const char c_begin[36] =
#if 0
      "-----BEGIN OPENSSH PRIVATE KEY-----\n";
#else
        {'-','-','-','-','-','B','E','G','I','N',' ','O','P','E','N','S','S','H',' ','P','R','I','V','A','T','E',' ','K','E','Y','-','-','-','-','-','\n'};
#endif
    /* There is \0 byte inserted between c_begin and c_end unless {'.',...} */
    __attribute__((aligned(1))) static const char c_end[34] =
#if 0
      "-----END OPENSSH PRIVATE KEY-----\n";
#else
        {'-','-','-','-','-','E','N','D',' ','O','P','E','N','S','S','H',' ','P','R','I','V','A','T','E',' ','K','E','Y','-','-','-','-','-','\n'};
#endif
    __attribute__((aligned(1))) static const char c_pad7[7] = {1,2,3,4,5,6,7};
    /* Buffer size needed in data: 236 + comment_size bytes. */
    char data[236 + MAX_COMMENT_SIZE], *origp, *dpend = data + sizeof data;
    u32 data_size;
    const u32 pad_size = -(comment_size + 3) & 7;
    origp = p;
    p = data;
    p = append(p, dpend, c_kprefix, 62);
    p = append(p, dpend, (const char*)public_key, 32);
    p = add_u32be(p, dpend, 131 + comment_size + pad_size);
    p = append(p, dpend, (const char*)checkstr, 4);
    p = append(p, dpend, (const char*)checkstr, 4);
    p = append(p, dpend, c_kprefix + 62 - 19, 19);
    p = append(p, dpend, (const char*)public_key, 32);
    p = add_u32be(p, dpend, 64);
    p = append(p, dpend, (const char*)private_key, 32);
    p = append(p, dpend, (const char*)public_key, 32);
    p = add_u32be(p, dpend, comment_size);
    p = append(p, dpend, comment, comment_size);
    p = append(p, dpend, c_pad7, pad_size);
    data_size = p - data;

    p = origp;
    p = append(p, pend, c_begin, 36);
    origp = p;
    p = base64encode(p, pend, (const u8*)data, data_size);
    p = split_lines(origp, pend, p - origp, 70);
    p = append(p, pend, c_end, 34);
    return p;
}

bool sshKeygenEd25519(const QString &filename, const QString &comm)
{
    QString comment = comm.simplified();
    if (comment.length() > MAX_COMMENT_SIZE) comment.truncate(MAX_COMMENT_SIZE);

    quint8 rnd36[36];
    generate_seed(rnd36, sizeof(rnd36));

    quint8 public_key[32];
    keypair(public_key, rnd36);

    // Buffer size needed: <= 400 + comment_size * 142 / 105 bytes
    char buf[401 + MAX_COMMENT_SIZE + MAX_COMMENT_SIZE / 3 + MAX_COMMENT_SIZE * 2 / 105];

    int len = build_openssh_public_key_ed25519(buf, buf + sizeof(buf), public_key, qPrintable(comment), comment.size()) - buf;
    QFile pub_file(filename + ".pub");
    if (!pub_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << Q_FUNC_INFO << "Can't write" << pub_file.fileName();
        return false;
    }
    if (pub_file.write(buf, len) != len) {
        qWarning() << Q_FUNC_INFO << "Can't write" << pub_file.fileName();
        return false;
    }
    pub_file.close();
    QFile::setPermissions(pub_file.fileName(), QFile::Permission(0x644));

    len = build_openssh_private_key_ed25519(buf, buf + sizeof(buf), public_key, qPrintable(comment), comment.size(),
                                            rnd36 /* private_key */, rnd36 + 32 /* checkstr */) - buf;
    QFile priv_file(filename);
    if (!priv_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << Q_FUNC_INFO << "Can't write" << priv_file.fileName();
        return false;
    }
    if (priv_file.write(buf, len) != len) {
        qWarning() << Q_FUNC_INFO << "Can't write" << priv_file.fileName();
        return false;
    }
    priv_file.close();
    QFile::setPermissions(priv_file.fileName(), QFile::Permission(0x600));
    return true;
}
