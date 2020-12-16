#include "../include/MySHA.hpp"
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#else
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#endif // WIN32


/////////////////////SHA512///////////////////////////////////
#ifndef PEDANTIC
	#if defined(__GNUC__) && __GNUC__>=2 && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
		#if defined(__x86_64) || defined(__x86_64__)
			#define ROTR(a,n)    ({ unsigned long long ret;              \
                                asm ("rorq %1,%0"       \
                                : "=r"(ret)             \
                                : "J"(n),"0"(a)         \
                                : "cc"); ret;           })
			#if !defined(B_ENDIAN)
				#define PULL64(x) ({ unsigned long long ret=*((const unsigned long long *)(&(x)));  \
                                asm ("bswapq    %0"             \
                                : "=r"(ret)                     \
                                : "0"(ret)); ret;               })
			#endif
		#elif (defined(__i386) || defined(__i386__)) && !defined(B_ENDIAN)
			#if defined(I386_ONLY)
				#define PULL64(x) ({ const unsigned int *p=(const unsigned int *)(&(x));\
                         unsigned int hi=p[0],lo=p[1];          \
                                asm("xchgb %%ah,%%al;xchgb %%dh,%%dl;"\
                                    "roll $16,%%eax; roll $16,%%edx; "\
                                    "xchgb %%ah,%%al;xchgb %%dh,%%dl;" \
                                : "=a"(lo),"=d"(hi)             \
                                : "0"(lo),"1"(hi) : "cc");      \
                                ((unsigned long long)hi)<<32|lo;        })
			#else
				#define PULL64(x) ({ const unsigned int *p=(const unsigned int *)(&(x));\
                         unsigned int hi=p[0],lo=p[1];          \
                                asm ("bswapl %0; bswapl %1;"    \
                                : "=r"(lo),"=r"(hi)             \
                                : "0"(lo),"1"(hi));             \
                                ((unsigned long long)hi)<<32|lo;        })
			#endif
		#elif (defined(_ARCH_PPC) && defined(__64BIT__)) || defined(_ARCH_PPC64)
			#define ROTR(a,n)    ({ unsigned long long ret;              \
                                asm ("rotrdi %0,%1,%2"  \
                                : "=r"(ret)             \
                                : "r"(a),"K"(n)); ret;  })
		#elif defined(__aarch64__)
			#define ROTR(a,n)    ({ unsigned long long ret;              \
                                asm ("ror %0,%1,%2"     \
                                : "=r"(ret)             \
                                : "r"(a),"I"(n)); ret;  })
			#if  defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
				__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
				#define PULL64(x)   ({ unsigned long long ret;                      \
                                asm ("rev       %0,%1"          \
                                : "=r"(ret)                     \
                                : "r"(*((const unsigned long long *)(&(x))))); ret;             })
			#endif
		#endif
	#elif defined(_MSC_VER)
		#if defined(_WIN64)         /* applies to both IA-64 and AMD64 */
			#pragma intrinsic(_rotr64)
			#define ROTR(a,n)    _rotr64((a),n)
		#endif
		#if defined(_M_IX86) && !defined(OPENSSL_NO_ASM) && !defined(OPENSSL_NO_INLINE_ASM)
			#if defined(I386_ONLY)
				static unsigned long long __fastcall __pull64be(const void *x)
				{
					_asm mov edx, [ecx + 0]
						_asm mov eax, [ecx + 4]
						_asm xchg dh, dl
					_asm xchg ah, al
					_asm rol edx, 16 _asm rol eax, 16 _asm xchg dh, dl _asm xchg ah, al
				}
			#else
				static unsigned long long  __fastcall __pull64be(const void *x)
				{
					_asm mov edx, [ecx + 0]
						_asm mov eax, [ecx + 4]
						_asm bswap edx _asm bswap eax
				}
			#endif
			#define PULL64(x) __pull64be(&(x))
			#if _MSC_VER<=1200
				#pragma inline_depth(0)
			#endif
		#endif
	#endif
#endif


#ifndef PULL64
	#define B(x,j)    (((unsigned long long)(*(((const unsigned char *)(&x))+j)))<<((7-j)*8))
	#define PULL64(x) (B(x,0)|B(x,1)|B(x,2)|B(x,3)|B(x,4)|B(x,5)|B(x,6)|B(x,7))
#endif

#ifndef ROTR
	#define ROTR(x,s)       (((x)>>s) | (x)<<(64-s))
#endif

#define Sigma0(x)       (ROTR((x),28) ^ ROTR((x),34) ^ ROTR((x),39))
#define Sigma1(x)       (ROTR((x),14) ^ ROTR((x),18) ^ ROTR((x),41))
#define sigma0(x)       (ROTR((x),1)  ^ ROTR((x),8)  ^ ((x)>>7))
#define sigma1(x)       (ROTR((x),19) ^ ROTR((x),61) ^ ((x)>>6))
#define Ch(x,y,z)       (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)      (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define ROUND_00_15(i,a,b,c,d,e,f,g,h)          do {    \
        T1 += h + Sigma1(e) + Ch(e,f,g) + K512[i];      \
        h = Sigma0(a) + Maj(a,b,c);                     \
        d += T1;        h += T1;                } while (0)
#define ROUND_16_80(i,j,a,b,c,d,e,f,g,h,X)      do {    \
        s0 = X[(j+1)&0x0f];     s0 = sigma0(s0);        \
        s1 = X[(j+14)&0x0f];    s1 = sigma1(s1);        \
        T1 = X[(j)&0x0f] += s0 + s1 + X[(j+9)&0x0f];    \
        ROUND_00_15(i+j,a,b,c,d,e,f,g,h);               } while (0)



static const unsigned long long K512[80] = {
	0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
	0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
	0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
	0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
	0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
	0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
	0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
	0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
	0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
	0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
	0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
	0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
	0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
	0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
	0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
	0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
	0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
	0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
	0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
	0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
	0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
	0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
	0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
	0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
	0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
	0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
	0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
	0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
	0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
	0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
	0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
	0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
	0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
	0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
	0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
	0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
	0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
	0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
	0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

/////////////////////////////////////////////////////////////////


#define SHA256_ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define SHA256_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

#define SHA256_Ch(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define SHA256_Maj(x, y, z) (((x) & ((y) | (z))) | ((y) & (z)))
#define SHA256_SIGMA0(x) (SHA256_ROTR((x), 2) ^  SHA256_ROTR((x), 13) ^ SHA256_ROTR((x), 22))
#define SHA256_SIGMA1(x) (SHA256_ROTR((x), 6) ^  SHA256_ROTR((x), 11) ^ SHA256_ROTR((x), 25))
#define SHA256_sigma0(x) (SHA256_ROTR((x), 7) ^  SHA256_ROTR((x), 18) ^ ((x) >> 3))
#define SHA256_sigma1(x) (SHA256_ROTR((x), 17) ^ SHA256_ROTR((x), 19) ^ ((x) >> 10))


#define DO_ROUND() { \
	t1 = h + SHA256_SIGMA1(e) + SHA256_Ch(e, f, g) + *(Kp++) + *(W++); \
	t2 = SHA256_SIGMA0(a) + SHA256_Maj(a, b, c); \
	h = g; \
	g = f; \
	f = e; \
	e = d + t1; \
	d = c; \
	c = b; \
	b = a; \
	a = t1 + t2; \
}


static const unsigned int K[64] = {
	0x428a2f98L, 0x71374491L, 0xb5c0fbcfL, 0xe9b5dba5L,
	0x3956c25bL, 0x59f111f1L, 0x923f82a4L, 0xab1c5ed5L,
	0xd807aa98L, 0x12835b01L, 0x243185beL, 0x550c7dc3L,
	0x72be5d74L, 0x80deb1feL, 0x9bdc06a7L, 0xc19bf174L,
	0xe49b69c1L, 0xefbe4786L, 0x0fc19dc6L, 0x240ca1ccL,
	0x2de92c6fL, 0x4a7484aaL, 0x5cb0a9dcL, 0x76f988daL,
	0x983e5152L, 0xa831c66dL, 0xb00327c8L, 0xbf597fc7L,
	0xc6e00bf3L, 0xd5a79147L, 0x06ca6351L, 0x14292967L,
	0x27b70a85L, 0x2e1b2138L, 0x4d2c6dfcL, 0x53380d13L,
	0x650a7354L, 0x766a0abbL, 0x81c2c92eL, 0x92722c85L,
	0xa2bfe8a1L, 0xa81a664bL, 0xc24b8b70L, 0xc76c51a3L,
	0xd192e819L, 0xd6990624L, 0xf40e3585L, 0x106aa070L,
	0x19a4c116L, 0x1e376c08L, 0x2748774cL, 0x34b0bcb5L,
	0x391c0cb3L, 0x4ed8aa4aL, 0x5b9cca4fL, 0x682e6ff3L,
	0x748f82eeL, 0x78a5636fL, 0x84c87814L, 0x8cc70208L,
	0x90befffaL, 0xa4506cebL, 0xbef9a3f7L, 0xc67178f2L
};

static const unsigned char padding[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#ifndef RUNTIME_ENDIAN

	#ifdef WORDS_BIGENDIAN

		#define BYTESWAP(x) (x)
		#define BYTESWAP64(x) (x)

	#else /* WORDS_BIGENDIAN */

		#define BYTESWAP(x) ((SHA256_ROTR((x), 8) & 0xff00ff00L) | \
		     (SHA256_ROTL((x), 8) & 0x00ff00ffL))
		#define BYTESWAP64(x) _byteswap64(x)

		//static inline uint64_t _byteswap64(uint64_t x)
		unsigned long long _byteswap64(unsigned long long x)
		{
			unsigned int a = x >> 32;
			unsigned int b = (unsigned int) x;
			return ((unsigned long long) BYTESWAP(b) << 32) | (unsigned long long) BYTESWAP(a);
		}

	#endif /* WORDS_BIGENDIAN */

#else /* !RUNTIME_ENDIAN */

	#define BYTESWAP(x) _byteswap(sc->littleEndian, x)
	#define BYTESWAP64(x) _byteswap64(sc->littleEndian, x)

	#define _BYTESWAP(x) ((SHA256_ROTR((x), 8) & 0xff00ff00L) | \
		      (SHA256_ROTL((x), 8) & 0x00ff00ffL))
	#define _BYTESWAP64(x) __byteswap64(x)

	static inline unsigned long long __byteswap64(unsigned long long x)
	{
		unsigned int a = x >> 32;
		unsigned int b = (unsigned int) x;
		return ((unsigned long long) _BYTESWAP(b) << 32) | (unsigned long long) _BYTESWAP(a);
	}

	static inline unsigned int _byteswap(int littleEndian, unsigned int x)
	{
		if (!littleEndian)
			return x;
		else
			return _BYTESWAP(x);
	}

	static inline unsigned long long _byteswap64(int littleEndian, unsigned long long x)
	{
		if (!littleEndian)
			return x;
		else
			return _BYTESWAP64(x);
	}

	static inline void setEndian(int *littleEndianp)
	{
		union {
			unsigned int w;
			unsigned char b[4];
		} endian;

		endian.w = 1L;
		*littleEndianp = endian.b[0] != 0;
	}

#endif /* !RUNTIME_ENDIAN */



static void burnStack (long long size)
{
	char buf[128];

	memset (buf, 0, sizeof (buf));
	size -= sizeof (buf);
	if (size > 0)
		burnStack (size);
}

///////////////////////////////////////////////////////////////

CSHA::CSHA()
{
}

CSHA::~CSHA()
{
}

std::string CSHA::StringToSHA256(const std::string &szSrc, bool bLowercase)
{
	return StringToSHA256(szSrc.c_str(), szSrc.length(), bLowercase);
}

std::string CSHA::StringToSHA256(const char* pSrc, size_t iLen, bool bLowercase)
{
	if (nullptr == pSrc || 0 == iLen)
		return "";

	uint8_t buff[SHA256_DIGEST_LENGTH]{ '\0' };

	if (StringToSHA256(pSrc, iLen, buff, SHA256_DIGEST_LENGTH))
	{
		//转字符串
		return BytesToHexString(buff, SHA256_DIGEST_LENGTH, bLowercase);
	}
	else
		return "";
}

bool CSHA::StringToSHA256(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen)
{
	return StringToSHA256(szSrc.c_str(), szSrc.length(), pDst, iDstBufLen);
}

bool CSHA::StringToSHA256(const char* pSrc, size_t iLen, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (nullptr == pSrc || 0 == iLen || nullptr == pDst || 0 == iDstBufLen)
		return false;

	SSHA256Context SHA256Context;
	unsigned char hash[SHA256_DIGEST_LENGTH];

	SHA256Init(&SHA256Context);

	SHA256Update(&SHA256Context, pSrc, iLen);
	SHA256Final(&SHA256Context, hash);

	//复制结果
	memcpy(pDst, hash, (iDstBufLen <= SHA256_DIGEST_LENGTH ? iDstBufLen : SHA256_DIGEST_LENGTH));

	return true;
}

std::string CSHA::FileToSHA256(const std::string & szFileName, bool bLowercase)
{
	uint8_t buff[SHA256_DIGEST_LENGTH + 1]{ '\0' };

	if (FileToSHA256(szFileName, buff, SHA256_DIGEST_LENGTH))
	{
		return BytesToHexString(buff, SHA256_DIGEST_LENGTH, bLowercase);
	}
	else
		return "";
}

bool CSHA::FileToSHA256(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (szFileName.empty() || !CheckFileExist(szFileName) || nullptr == pDst || 0 == iDstBufLen)
		return false;

	std::ifstream FileIn(szFileName.c_str(), std::ios::in | std::ios::binary);
	if (FileIn.fail())
		return false;

	SSHA256Context SHA256Context;
	unsigned char hash[SHA256_DIGEST_LENGTH];

	SHA256Init(&SHA256Context);

	const std::streamsize iBufLen = 1024;
	char buf[iBufLen] = { '\0' };
	uint32_t readLen = 0;

	while (!FileIn.eof())
	{
		FileIn.read(buf, iBufLen);
		readLen = (uint32_t)FileIn.gcount();

		if (readLen > 0)
			SHA256Update(&SHA256Context, (const uint8_t*)buf, readLen);
	}

	SHA256Final(&SHA256Context, hash);
	FileIn.close();

	//复制结果
	memcpy(pDst, hash, (iDstBufLen <= SHA256_DIGEST_LENGTH ? iDstBufLen : SHA256_DIGEST_LENGTH));

	return true;
}

void CSHA::SHA256Init(SSHA256Context *sc)
{
#ifdef RUNTIME_ENDIAN
	setEndian (&sc->littleEndian);
#endif /* RUNTIME_ENDIAN */

	sc->totalLength = 0LL;
	sc->hash[0] = 0x6a09e667L;
	sc->hash[1] = 0xbb67ae85L;
	sc->hash[2] = 0x3c6ef372L;
	sc->hash[3] = 0xa54ff53aL;
	sc->hash[4] = 0x510e527fL;
	sc->hash[5] = 0x9b05688cL;
	sc->hash[6] = 0x1f83d9abL;
	sc->hash[7] = 0x5be0cd19L;
	sc->bufferLength = 0L;
}


void CSHA::SHA256Update(SSHA256Context *sc, const void *vdata, size_t len)
{
	const unsigned char *data = (unsigned char *)vdata;
	size_t bufferBytesLeft;
	size_t bytesToCopy;
	int needBurn = 0;

#ifdef SHA256_FAST_COPY
	if (sc->bufferLength) {
		bufferBytesLeft = 64L - sc->bufferLength;

		bytesToCopy = bufferBytesLeft;
		if (bytesToCopy > len)
			bytesToCopy = len;

		memcpy (&sc->buffer.bytes[sc->bufferLength], data, bytesToCopy);

		sc->totalLength += bytesToCopy * 8L;

		sc->bufferLength += bytesToCopy;
		data += bytesToCopy;
		len -= bytesToCopy;

		if (sc->bufferLength == 64L) {
			SHA256Guts (sc, sc->buffer.words);
			needBurn = 1;
			sc->bufferLength = 0L;
		}
	}

	while (len > 63L) {
		sc->totalLength += 512L;

		SHA256Guts (sc, data);
		needBurn = 1;

		data += 64L;
		len -= 64L;
	}

  if (len) {
    memcpy (&sc->buffer.bytes[sc->bufferLength], data, len);

    sc->totalLength += len * 8L;

    sc->bufferLength += len;
  }
#else /* SHA256_FAST_COPY */
	while (len) {
		bufferBytesLeft = 64L - sc->bufferLength;

		bytesToCopy = bufferBytesLeft;
		if (bytesToCopy > len)
			bytesToCopy = len;

		memcpy (&sc->buffer.bytes[sc->bufferLength], data, bytesToCopy);

		sc->totalLength += bytesToCopy * 8L;

		sc->bufferLength += (unsigned int)bytesToCopy;
		data += bytesToCopy;
		len -= bytesToCopy;

		if (sc->bufferLength == 64L) {
			SHA256Guts (sc, sc->buffer.words);
			needBurn = 1;
			sc->bufferLength = 0L;
		}
	}
#endif /* SHA256_FAST_COPY */

	if (needBurn)
		burnStack (sizeof (unsigned int[74]) + sizeof (unsigned int *[6]) + sizeof (int));
}



void CSHA::SHA256Final(SSHA256Context *sc, unsigned char hash[SHA256_DIGEST_LENGTH])
{
	unsigned int bytesToPad;
	unsigned long long lengthPad;
	int i;

	bytesToPad = 120L - sc->bufferLength;
	if (bytesToPad > 64L)
		bytesToPad -= 64L;

	lengthPad = BYTESWAP64(sc->totalLength);

	SHA256Update (sc, padding, bytesToPad);
	SHA256Update (sc, &lengthPad, 8L);

	if (hash) {
		for (i = 0; i < SHA256_HASH_WORDS; i++) {
		#ifdef SHA256_FAST_COPY
			*((unsigned int *) hash) = BYTESWAP(sc->hash[i]);
		#else /* SHA256_FAST_COPY */
			hash[0] = (unsigned char) (sc->hash[i] >> 24);
			hash[1] = (unsigned char) (sc->hash[i] >> 16);
			hash[2] = (unsigned char) (sc->hash[i] >> 8);
			hash[3] = (unsigned char) (sc->hash[i]);
		#endif /* SHA256_FAST_COPY */
			hash += 4;
		}
	}
}


void CSHA::SHA256Guts(CSHA::SSHA256Context *sc, const unsigned int *cbuf)
{
	unsigned int buf[64];
	unsigned int *W, *W2, *W7, *W15, *W16;
	unsigned int a, b, c, d, e, f, g, h;
	unsigned int t1, t2;
	const unsigned int *Kp;
	int i;

	W = buf;

	for (i = 15; i >= 0; i--) {
		*(W++) = BYTESWAP(*cbuf);
		cbuf++;
	}

	W16 = &buf[0];
	W15 = &buf[1];
	W7 = &buf[9];
	W2 = &buf[14];

	for (i = 47; i >= 0; i--) {
		*(W++) = SHA256_sigma1(*W2) + *(W7++) + SHA256_sigma0(*W15) + *(W16++);
		W2++;
		W15++;
	}

	a = sc->hash[0];
	b = sc->hash[1];
	c = sc->hash[2];
	d = sc->hash[3];
	e = sc->hash[4];
	f = sc->hash[5];
	g = sc->hash[6];
	h = sc->hash[7];

	Kp = K;
	W = buf;

#ifndef SHA256_UNROLL
#define SHA256_UNROLL 1
#endif /* !SHA256_UNROLL */

#if SHA256_UNROLL == 1
	for (i = 63; i >= 0; i--)
		DO_ROUND();
#elif SHA256_UNROLL == 2
	for (i = 31; i >= 0; i--) {
		DO_ROUND(); DO_ROUND();
	}
#elif SHA256_UNROLL == 4
	for (i = 15; i >= 0; i--) {
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	}
#elif SHA256_UNROLL == 8
	for (i = 7; i >= 0; i--) {
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	}
#elif SHA256_UNROLL == 16
	for (i = 3; i >= 0; i--) {
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	}
#elif SHA256_UNROLL == 32
	for (i = 1; i >= 0; i--) {
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
		DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	}
#elif SHA256_UNROLL == 64
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
	DO_ROUND(); DO_ROUND(); DO_ROUND(); DO_ROUND();
#else
#error "SHA256_UNROLL must be 1, 2, 4, 8, 16, 32, or 64!"
#endif

	sc->hash[0] += a;
	sc->hash[1] += b;
	sc->hash[2] += c;
	sc->hash[3] += d;
	sc->hash[4] += e;
	sc->hash[5] += f;
	sc->hash[6] += g;
	sc->hash[7] += h;
}


/////////////////////////////////////////////////////////
void CSHA::SHA512Init(SSHA512Context * sc)
{
	memset(sc, 0, sizeof(*sc));

	sc->h[0] = 0x6a09e667f3bcc908ULL;
	sc->h[1] = 0xbb67ae8584caa73bULL;
	sc->h[2] = 0x3c6ef372fe94f82bULL;
	sc->h[3] = 0xa54ff53a5f1d36f1ULL;
	sc->h[4] = 0x510e527fade682d1ULL;
	sc->h[5] = 0x9b05688c2b3e6c1fULL;
	sc->h[6] = 0x1f83d9abfb41bd6bULL;
	sc->h[7] = 0x5be0cd19137e2179ULL;

	sc->Nl = 0;
	sc->Nh = 0;
	sc->num = 0;
	sc->md_len = SHA512_DIGEST_LENGTH;
}

void CSHA::SHA512Update(SSHA512Context * sc, const void * vdata, size_t len)
{
	unsigned long long l;
	unsigned char *p = sc->u.p;
	const unsigned char *data = (const unsigned char *)vdata;

	if (len == 0)
		return ;

	l = (sc->Nl + (((unsigned long long)len) << 3)) & (unsigned long long)(0xffffffffffffffff);
	if (l < sc->Nl)
		sc->Nh++;
	if (sizeof(len) >= 8)
		sc->Nh += (((unsigned long long)len) >> 61);
	sc->Nl = l;

	if (sc->num != 0) {
		size_t n = sizeof(sc->u) - sc->num;

		if (len < n) {
			memcpy(p + sc->num, data, len), sc->num += (unsigned int)len;
			return ;
		}
		else {
			memcpy(p + sc->num, data, n), sc->num = 0;
			len -= n, data += n;
			SHA512BlockDataOrder(sc, p, 1);
		}
	}

	if (len >= sizeof(sc->u)) {
# ifndef SHA512_BLOCK_CAN_MANAGE_UNALIGNED_DATA
		if ((size_t)data % sizeof(sc->u.d[0]) != 0)
			while (len >= sizeof(sc->u))
				memcpy(p, data, sizeof(sc->u)),
				SHA512BlockDataOrder(sc, p, 1),
				len -= sizeof(sc->u), data += sizeof(sc->u);
		else
# endif
			SHA512BlockDataOrder(sc, data, len / sizeof(sc->u)),
			data += len, len %= sizeof(sc->u), data -= len;
	}

	if (len != 0)
		memcpy(p, data, len), sc->num = (int)len;

	return ;
}

void CSHA::SHA512Final(SSHA512Context * sc, unsigned char hash[SHA512_DIGEST_LENGTH])
{
	unsigned char *p = (unsigned char *)sc->u.p;
	size_t n = sc->num;

	p[n] = 0x80;                /* There always is a room for one */
	n++;
	if (n > (sizeof(sc->u) - 16))
		memset(p + n, 0, sizeof(sc->u) - n), n = 0,
		SHA512BlockDataOrder(sc, p, 1);

	memset(p + n, 0, sizeof(sc->u) - 16 - n);
# ifdef  B_ENDIAN //大端
	c->u.d[SHA_LBLOCK - 2] = c->Nh;
	c->u.d[SHA_LBLOCK - 1] = c->Nl;
# else
	p[sizeof(sc->u) - 1] = (unsigned char)(sc->Nl);
	p[sizeof(sc->u) - 2] = (unsigned char)(sc->Nl >> 8);
	p[sizeof(sc->u) - 3] = (unsigned char)(sc->Nl >> 16);
	p[sizeof(sc->u) - 4] = (unsigned char)(sc->Nl >> 24);
	p[sizeof(sc->u) - 5] = (unsigned char)(sc->Nl >> 32);
	p[sizeof(sc->u) - 6] = (unsigned char)(sc->Nl >> 40);
	p[sizeof(sc->u) - 7] = (unsigned char)(sc->Nl >> 48);
	p[sizeof(sc->u) - 8] = (unsigned char)(sc->Nl >> 56);
	p[sizeof(sc->u) - 9] = (unsigned char)(sc->Nh);
	p[sizeof(sc->u) - 10] = (unsigned char)(sc->Nh >> 8);
	p[sizeof(sc->u) - 11] = (unsigned char)(sc->Nh >> 16);
	p[sizeof(sc->u) - 12] = (unsigned char)(sc->Nh >> 24);
	p[sizeof(sc->u) - 13] = (unsigned char)(sc->Nh >> 32);
	p[sizeof(sc->u) - 14] = (unsigned char)(sc->Nh >> 40);
	p[sizeof(sc->u) - 15] = (unsigned char)(sc->Nh >> 48);
	p[sizeof(sc->u) - 16] = (unsigned char)(sc->Nh >> 56);
# endif

	SHA512BlockDataOrder(sc, p, 1);

	if (hash == 0)
		return;

	switch (sc->md_len) {
		/* Let compiler decide if it's appropriate to unroll... */
	case SHA384_DIGEST_LENGTH:
		for (n = 0; n < SHA384_DIGEST_LENGTH / 8; n++) {
			unsigned long long t = sc->h[n];

			*(hash++) = (unsigned char)(t >> 56);
			*(hash++) = (unsigned char)(t >> 48);
			*(hash++) = (unsigned char)(t >> 40);
			*(hash++) = (unsigned char)(t >> 32);
			*(hash++) = (unsigned char)(t >> 24);
			*(hash++) = (unsigned char)(t >> 16);
			*(hash++) = (unsigned char)(t >> 8);
			*(hash++) = (unsigned char)(t);
		}
		break;
	case SHA512_DIGEST_LENGTH:
		for (n = 0; n < SHA512_DIGEST_LENGTH / 8; n++) {
			unsigned long long t = sc->h[n];

			*(hash++) = (unsigned char)(t >> 56);
			*(hash++) = (unsigned char)(t >> 48);
			*(hash++) = (unsigned char)(t >> 40);
			*(hash++) = (unsigned char)(t >> 32);
			*(hash++) = (unsigned char)(t >> 24);
			*(hash++) = (unsigned char)(t >> 16);
			*(hash++) = (unsigned char)(t >> 8);
			*(hash++) = (unsigned char)(t);
		}
		break;
		/* ... as well as make sure md_len is not abused. */
	default:
		return ;
	}

	return ;
}


void CSHA::SHA512BlockDataOrder(SSHA512Context *ctx, const void *in, size_t num)
{
	const unsigned long long *W = (unsigned long long *)in;
	unsigned long long a, b, c, d, e, f, g, h, s0, s1, T1;
	unsigned long long X[16];
	int i;

	while (num--) {

		a = ctx->h[0];
		b = ctx->h[1];
		c = ctx->h[2];
		d = ctx->h[3];
		e = ctx->h[4];
		f = ctx->h[5];
		g = ctx->h[6];
		h = ctx->h[7];

#   ifdef B_ENDIAN
		T1 = X[0] = W[0];
		ROUND_00_15(0, a, b, c, d, e, f, g, h);
		T1 = X[1] = W[1];
		ROUND_00_15(1, h, a, b, c, d, e, f, g);
		T1 = X[2] = W[2];
		ROUND_00_15(2, g, h, a, b, c, d, e, f);
		T1 = X[3] = W[3];
		ROUND_00_15(3, f, g, h, a, b, c, d, e);
		T1 = X[4] = W[4];
		ROUND_00_15(4, e, f, g, h, a, b, c, d);
		T1 = X[5] = W[5];
		ROUND_00_15(5, d, e, f, g, h, a, b, c);
		T1 = X[6] = W[6];
		ROUND_00_15(6, c, d, e, f, g, h, a, b);
		T1 = X[7] = W[7];
		ROUND_00_15(7, b, c, d, e, f, g, h, a);
		T1 = X[8] = W[8];
		ROUND_00_15(8, a, b, c, d, e, f, g, h);
		T1 = X[9] = W[9];
		ROUND_00_15(9, h, a, b, c, d, e, f, g);
		T1 = X[10] = W[10];
		ROUND_00_15(10, g, h, a, b, c, d, e, f);
		T1 = X[11] = W[11];
		ROUND_00_15(11, f, g, h, a, b, c, d, e);
		T1 = X[12] = W[12];
		ROUND_00_15(12, e, f, g, h, a, b, c, d);
		T1 = X[13] = W[13];
		ROUND_00_15(13, d, e, f, g, h, a, b, c);
		T1 = X[14] = W[14];
		ROUND_00_15(14, c, d, e, f, g, h, a, b);
		T1 = X[15] = W[15];
		ROUND_00_15(15, b, c, d, e, f, g, h, a);
#   else
		T1 = X[0] = PULL64(W[0]);
		ROUND_00_15(0, a, b, c, d, e, f, g, h);
		T1 = X[1] = PULL64(W[1]);
		ROUND_00_15(1, h, a, b, c, d, e, f, g);
		T1 = X[2] = PULL64(W[2]);
		ROUND_00_15(2, g, h, a, b, c, d, e, f);
		T1 = X[3] = PULL64(W[3]);
		ROUND_00_15(3, f, g, h, a, b, c, d, e);
		T1 = X[4] = PULL64(W[4]);
		ROUND_00_15(4, e, f, g, h, a, b, c, d);
		T1 = X[5] = PULL64(W[5]);
		ROUND_00_15(5, d, e, f, g, h, a, b, c);
		T1 = X[6] = PULL64(W[6]);
		ROUND_00_15(6, c, d, e, f, g, h, a, b);
		T1 = X[7] = PULL64(W[7]);
		ROUND_00_15(7, b, c, d, e, f, g, h, a);
		T1 = X[8] = PULL64(W[8]);
		ROUND_00_15(8, a, b, c, d, e, f, g, h);
		T1 = X[9] = PULL64(W[9]);
		ROUND_00_15(9, h, a, b, c, d, e, f, g);
		T1 = X[10] = PULL64(W[10]);
		ROUND_00_15(10, g, h, a, b, c, d, e, f);
		T1 = X[11] = PULL64(W[11]);
		ROUND_00_15(11, f, g, h, a, b, c, d, e);
		T1 = X[12] = PULL64(W[12]);
		ROUND_00_15(12, e, f, g, h, a, b, c, d);
		T1 = X[13] = PULL64(W[13]);
		ROUND_00_15(13, d, e, f, g, h, a, b, c);
		T1 = X[14] = PULL64(W[14]);
		ROUND_00_15(14, c, d, e, f, g, h, a, b);
		T1 = X[15] = PULL64(W[15]);
		ROUND_00_15(15, b, c, d, e, f, g, h, a);
#   endif

		for (i = 16; i < 80; i += 16) {
			ROUND_16_80(i, 0, a, b, c, d, e, f, g, h, X);
			ROUND_16_80(i, 1, h, a, b, c, d, e, f, g, X);
			ROUND_16_80(i, 2, g, h, a, b, c, d, e, f, X);
			ROUND_16_80(i, 3, f, g, h, a, b, c, d, e, X);
			ROUND_16_80(i, 4, e, f, g, h, a, b, c, d, X);
			ROUND_16_80(i, 5, d, e, f, g, h, a, b, c, X);
			ROUND_16_80(i, 6, c, d, e, f, g, h, a, b, X);
			ROUND_16_80(i, 7, b, c, d, e, f, g, h, a, X);
			ROUND_16_80(i, 8, a, b, c, d, e, f, g, h, X);
			ROUND_16_80(i, 9, h, a, b, c, d, e, f, g, X);
			ROUND_16_80(i, 10, g, h, a, b, c, d, e, f, X);
			ROUND_16_80(i, 11, f, g, h, a, b, c, d, e, X);
			ROUND_16_80(i, 12, e, f, g, h, a, b, c, d, X);
			ROUND_16_80(i, 13, d, e, f, g, h, a, b, c, X);
			ROUND_16_80(i, 14, c, d, e, f, g, h, a, b, X);
			ROUND_16_80(i, 15, b, c, d, e, f, g, h, a, X);
		}

		ctx->h[0] += a;
		ctx->h[1] += b;
		ctx->h[2] += c;
		ctx->h[3] += d;
		ctx->h[4] += e;
		ctx->h[5] += f;
		ctx->h[6] += g;
		ctx->h[7] += h;

		W += 16;
	}
}

std::string CSHA::StringToSHA512(const std::string & szSrc, bool bLowercase)
{
	return StringToSHA512(szSrc.c_str(), szSrc.size(), bLowercase);
}

std::string CSHA::StringToSHA512(const char * pSrc, size_t iLen, bool bLowercase)
{
	if (nullptr == pSrc || 0 == iLen)
		return "";

	SSHA512Context SHA512Context;
	unsigned char hash[SHA512_DIGEST_LENGTH];

	SHA512Init(&SHA512Context);

	SHA512Update(&SHA512Context, pSrc, iLen);
	SHA512Final(&SHA512Context, hash);

	//转字符串
	return BytesToHexString(hash, SHA512_DIGEST_LENGTH, bLowercase);
}

bool CSHA::StringToSHA512(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen)
{
	return StringToSHA512(szSrc.c_str(), szSrc.length(), pDst, iDstBufLen);
}

bool CSHA::StringToSHA512(const char* pSrc, size_t iLen, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (nullptr == pSrc || 0 == iLen || nullptr == pDst || 0 == iDstBufLen)
		return false;

	SSHA512Context SHA512Context;
	unsigned char hash[SHA512_DIGEST_LENGTH];

	SHA512Init(&SHA512Context);

	SHA512Update(&SHA512Context, pSrc, iLen);
	SHA512Final(&SHA512Context, hash);

	//复制结果
	memcpy(pDst, hash, (iDstBufLen <= SHA512_DIGEST_LENGTH ? iDstBufLen : SHA512_DIGEST_LENGTH));

	return true;
}

std::string CSHA::FileToSHA512(const std::string & szFileName, bool bLowercase)
{
	uint8_t buff[SHA512_DIGEST_LENGTH + 1]{ '\0' };

	if (FileToSHA512(szFileName, buff, SHA512_DIGEST_LENGTH))
	{
		return BytesToHexString(buff, SHA512_DIGEST_LENGTH, bLowercase);
	}
	else
		return "";
}

bool CSHA::FileToSHA512(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (szFileName.empty() || !CheckFileExist(szFileName) || nullptr == pDst || 0 == iDstBufLen)
		return false;

	std::ifstream FileIn(szFileName.c_str(), std::ios::in | std::ios::binary);
	if (FileIn.fail())
		return false;

	SSHA512Context SHA512Context;
	unsigned char hash[SHA512_DIGEST_LENGTH];

	SHA512Init(&SHA512Context);

	const std::streamsize iBufLen = 1024;
	char buf[iBufLen] = { '\0' };
	uint32_t readLen = 0;

	while (!FileIn.eof())
	{
		FileIn.read(buf, iBufLen);
		readLen = (uint32_t)FileIn.gcount();

		if (readLen > 0)
			SHA512Update(&SHA512Context, (const uint8_t*)buf, readLen);
	}

	SHA512Final(&SHA512Context, hash);
	FileIn.close();

	//复制结果
	memcpy(pDst, hash, (iDstBufLen <= SHA512_DIGEST_LENGTH ? iDstBufLen : SHA512_DIGEST_LENGTH));

	return true;
}

/////////////////////////////////////////////////////////

void CSHA::SHA384Init(SSHA384Context * sc)
{
	memset(sc, 0, sizeof(*sc));

	sc->h[0] = 0x6a09e667f3bcc908ULL;
	sc->h[1] = 0xbb67ae8584caa73bULL;
	sc->h[2] = 0x3c6ef372fe94f82bULL;
	sc->h[3] = 0xa54ff53a5f1d36f1ULL;
	sc->h[4] = 0x510e527fade682d1ULL;
	sc->h[5] = 0x9b05688c2b3e6c1fULL;
	sc->h[6] = 0x1f83d9abfb41bd6bULL;
	sc->h[7] = 0x5be0cd19137e2179ULL;

	sc->Nl = 0;
	sc->Nh = 0;
	sc->num = 0;
	sc->md_len = SHA384_DIGEST_LENGTH;
}

void CSHA::SHA384Update(SSHA384Context * sc, const void * vdata, size_t len)
{
	return SHA512Update((SSHA512Context *)sc,vdata,len);
}

void CSHA::SHA384Final(SSHA384Context * sc, unsigned char hash[SHA384_DIGEST_LENGTH])
{
	return SHA512Final((SSHA512Context *)sc,hash);
}

std::string CSHA::StringToSHA384(const std::string & szSrc, bool bLowercase)
{
	return StringToSHA384(szSrc.c_str(), szSrc.size(), bLowercase);
}

std::string CSHA::StringToSHA384(const char * pSrc, size_t iLen, bool bLowercase)
{
	if (nullptr == pSrc || 0 == iLen)
		return "";

	SSHA384Context SHA384Context;
	unsigned char hash[SHA384_DIGEST_LENGTH];

	SHA384Init(&SHA384Context);

	SHA384Update(&SHA384Context, pSrc, iLen);
	SHA384Final(&SHA384Context, hash);

	//转字符串
	return BytesToHexString(hash, SHA384_DIGEST_LENGTH, bLowercase);
}

bool CSHA::StringToSHA384(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen)
{
	return StringToSHA384(szSrc.c_str(), szSrc.length(), pDst, iDstBufLen);
}

bool CSHA::StringToSHA384(const char* pSrc, size_t iLen, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (nullptr == pSrc || 0 == iLen || nullptr == pDst || 0 == iDstBufLen)
		return false;

	SSHA384Context SHA384Context;
	unsigned char hash[SHA384_DIGEST_LENGTH];

	SHA384Init(&SHA384Context);

	SHA384Update(&SHA384Context, pSrc, iLen);
	SHA384Final(&SHA384Context, hash);

	//复制结果
	memcpy(pDst, hash, (iDstBufLen <= SHA384_DIGEST_LENGTH ? iDstBufLen : SHA384_DIGEST_LENGTH));

	return true;
}

std::string CSHA::FileToSHA384(const std::string & szFileName, bool bLowercase)
{
	uint8_t buff[SHA384_DIGEST_LENGTH + 1]{ '\0' };

	if (FileToSHA384(szFileName, buff, SHA384_DIGEST_LENGTH))
	{
		return BytesToHexString(buff, SHA384_DIGEST_LENGTH, bLowercase);
	}
	else
		return "";
}

bool CSHA::FileToSHA384(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (szFileName.empty() || !CheckFileExist(szFileName) || nullptr == pDst || 0 == iDstBufLen)
		return false;

	std::ifstream FileIn(szFileName.c_str(), std::ios::in | std::ios::binary);
	if (FileIn.fail())
		return false;

	SSHA384Context SHA384Context;
	unsigned char hash[SHA384_DIGEST_LENGTH];

	SHA384Init(&SHA384Context);

	const std::streamsize iBufLen = 1024;
	char buf[iBufLen] = { '\0' };
	uint32_t readLen = 0;

	while (!FileIn.eof())
	{
		FileIn.read(buf, iBufLen);
		readLen = (uint32_t)FileIn.gcount();

		if (readLen > 0)
			SHA384Update(&SHA384Context, (const uint8_t*)buf, readLen);
	}

	SHA384Final(&SHA384Context, hash);
	FileIn.close();

	//复制结果
	memcpy(pDst, hash, (iDstBufLen <= SHA384_DIGEST_LENGTH ? iDstBufLen : SHA384_DIGEST_LENGTH));

	return true;
}