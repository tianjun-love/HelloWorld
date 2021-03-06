﻿#include "../include/MyMD5.hpp"
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#else
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#endif // WIN32

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

CMD5::CMD5()
{
}

CMD5::~CMD5()
{
}

std::string CMD5::StringToMD5(const std::string &szSrc, bool bLowercase)
{
	return StringToMD5(szSrc.c_str(), (uint32_t)szSrc.length(), bLowercase);
}

std::string CMD5::StringToMD5(const char* pSrc, uint32_t iLen, bool bLowercase)
{
	if (nullptr == pSrc || iLen == 0)
		return "";

	md5_ctx *pCTX = init();
	if (pCTX)
	{
		update(pCTX, (const uint8_t*)pSrc, iLen);
		finalize(pCTX);

		std::string szRet = BytesToHexString(pCTX->digest, E_RESULT_SIZE, bLowercase);
		delete pCTX;

		return std::move(szRet);
	}
	else
		return "";
}

bool CMD5::StringToMD5(const std::string &szSrc, uint8_t* pDst, uint32_t iDstBufLen)
{
	return StringToMD5(szSrc.c_str(), (uint32_t)szSrc.length(), pDst, iDstBufLen);
}

bool CMD5::StringToMD5(const char* pSrc, uint32_t iLen, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (nullptr == pSrc || iLen == 0 || nullptr == pDst || iDstBufLen == 0)
		return false;

	md5_ctx *pCTX = init();
	if (pCTX)
	{
		update(pCTX, (const uint8_t*)pSrc, iLen);
		finalize(pCTX);

		//复制结果
		memcpy(pDst, pCTX->digest, (iDstBufLen <= E_RESULT_SIZE ? iDstBufLen : E_RESULT_SIZE));

		delete pCTX;
		return true;
	}
	else
		return false;
}

std::string CMD5::FileToMD5(const std::string& szFileName, bool bLowercase)
{
	uint8_t buff[E_RESULT_SIZE + 1]{ '\0' };

	if (FileToMD5(szFileName, buff, E_RESULT_SIZE))
	{
		return BytesToHexString(buff, E_RESULT_SIZE, bLowercase);
	}
	else
		return "";
}

bool CMD5::FileToMD5(const std::string& szFileName, uint8_t* pDst, uint32_t iDstBufLen)
{
	if (szFileName.empty() || nullptr == pDst || iDstBufLen == 0)
		return false;

	if (!CheckFileExist(szFileName))
	{
		return false;
	}

	std::ifstream FileIn(szFileName.c_str(), std::ios::in | std::ios::binary);
	if (FileIn.fail())
		return false;

	md5_ctx *pCTX = init();
	if (pCTX)
	{
		const std::streamsize iBufLen = 1024;
		char buf[iBufLen] = { '\0' };
		uint32_t readLen = 0;

		while (!FileIn.eof())
		{
			FileIn.read(buf, iBufLen);
			readLen = (uint32_t)FileIn.gcount();

			if (readLen > 0)
				update(pCTX, (const uint8_t*)buf, readLen);
		}

		finalize(pCTX);
		FileIn.close();

		//复制结果
		memcpy(pDst, pCTX->digest, (iDstBufLen <= E_RESULT_SIZE ? iDstBufLen : E_RESULT_SIZE));

		delete pCTX;
		return true;
	}
	else
	{
		FileIn.close();
		return false;
	}
}

// MD5 init
CMD5::md5_ctx* CMD5::init()
{
	md5_ctx *pCTX = new md5_ctx;
	if (pCTX)
	{
		pCTX->finalized = false;

		pCTX->count[0] = 0;
		pCTX->count[1] = 0;

		// load magic initialization constants.
		pCTX->state[0] = 0x67452301;
		pCTX->state[1] = 0xefcdab89;
		pCTX->state[2] = 0x98badcfe;
		pCTX->state[3] = 0x10325476;

		memset(pCTX->digest, 0, E_RESULT_SIZE);
	}

	return pCTX;
}

// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block
void CMD5::update(md5_ctx *pCTX, const uint8_t input[], uint32_t len)
{
	// compute number of bytes mod 64
	uint32_t index = pCTX->count[0] / 8 % E_BLOCK_SIZE;

	// Update number of bits
	if ((pCTX->count[0] += (len << 3)) < (len << 3))
		pCTX->count[1]++;
	pCTX->count[1] += (len >> 29);

	// number of bytes we need to fill in buffer
	uint32_t firstpart = 64 - index;
	uint32_t i = 0;

	// transform as many times as possible.
	if (len >= firstpart)
	{
		// fill buffer first, transform
		memcpy(&pCTX->buffer[index], input, firstpart);
		transform(pCTX, pCTX->buffer);

		// transform chunks of blocksize (64 bytes)
		for (i = firstpart; i + E_BLOCK_SIZE <= len; i += E_BLOCK_SIZE)
			transform(pCTX, &input[i]);

		index = 0;
	}
	else
		i = 0;

	// buffer remaining input
	memcpy(&pCTX->buffer[index], &input[i], len - i);
}

// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
void CMD5::finalize(md5_ctx *pCTX)
{
	static uint8_t padding[64] = {
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	if (!pCTX->finalized) {
		// Save number of bits
		uint8_t bits[8];
		encode(bits, pCTX->count, 8);

		// pad out to 56 mod 64.
		uint32_t index = pCTX->count[0] / 8 % 64;
		uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
		update(pCTX, padding, padLen);

		// Append length (before padding)
		update(pCTX, bits, 8);

		// Store state in digest
		encode(pCTX->digest, pCTX->state, E_RESULT_SIZE);

		// Zeroize sensitive information.
		memset(pCTX->buffer, 0, sizeof pCTX->buffer);
		memset(pCTX->count, 0, sizeof pCTX->count);

		pCTX->finalized = true;
	}
}

// apply MD5 algo on a block
void CMD5::transform(md5_ctx *pCTX, const uint8_t block[E_BLOCK_SIZE])
{
	uint32_t a = pCTX->state[0], b = pCTX->state[1], c = pCTX->state[2], d = pCTX->state[3], x[16];
	decode(x, block, E_BLOCK_SIZE);

	/* Round 1 */
	FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
	FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
	FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
	FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
	FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
	FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
	FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
	FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
	FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
	FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
	FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

											/* Round 2 */
	GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
	GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
	GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
	GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
	GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
	GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
	GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
	GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
	GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
	GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
	GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
	GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

											/* Round 3 */
	HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
	HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
	HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
	HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
	HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
	HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
	HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
	HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
	HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
	HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

										   /* Round 4 */
	II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
	II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
	II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
	II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
	II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
	II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
	II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
	II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
	II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
	II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

	pCTX->state[0] += a;
	pCTX->state[1] += b;
	pCTX->state[2] += c;
	pCTX->state[3] += d;

	// Zeroize sensitive information.
	memset(x, 0, sizeof x);
}

// decodes input (unsigned char) into output (uint32_t). Assumes len is a multiple of 4.
void CMD5::decode(uint32_t output[], const uint8_t input[], uint32_t len)
{
	for (uint32_t i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j + 1]) << 8) |
		(((uint32_t)input[j + 2]) << 16) | (((uint32_t)input[j + 3]) << 24);
}

// encodes input (uint32_t) into output (unsigned char). Assumes len is
// a multiple of 4.
void CMD5::encode(uint8_t output[], const uint32_t input[], uint32_t len)
{
	for (uint32_t i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = input[i] & 0xff;
		output[j + 1] = (input[i] >> 8) & 0xff;
		output[j + 2] = (input[i] >> 16) & 0xff;
		output[j + 3] = (input[i] >> 24) & 0xff;
	}
}

// F, G, H and I are basic MD5 functions.
inline uint32_t CMD5::F(uint32_t x, uint32_t y, uint32_t z) {
	return (x&y) | (~x&z);
}

inline uint32_t CMD5::G(uint32_t x, uint32_t y, uint32_t z) {
	return (x&z) | (y&~z);
}

inline uint32_t CMD5::H(uint32_t x, uint32_t y, uint32_t z) {
	return x^y^z;
}

inline uint32_t CMD5::I(uint32_t x, uint32_t y, uint32_t z) {
	return y ^ (x | ~z);
}

// rotate_left rotates x left n bits.
inline uint32_t CMD5::rotate_left(uint32_t x, uint32_t n) {
	return (x << n) | (x >> (32 - n));
}

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
inline void CMD5::FF(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a = rotate_left(a + F(b, c, d) + x + ac, s) + b;
}

inline void CMD5::GG(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a = rotate_left(a + G(b, c, d) + x + ac, s) + b;
}

inline void CMD5::HH(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a = rotate_left(a + H(b, c, d) + x + ac, s) + b;
}

inline void CMD5::II(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
	a = rotate_left(a + I(b, c, d) + x + ac, s) + b;
}

long long CMD5::BKDHash(const char* pSrc)
{
	long long hash = 0;
	const int seed = 131; //31,131,1313,13131,131313...

	while (*pSrc)
	{
		hash = hash * seed + *pSrc++;
	}

	return (hash & 0x7FFFFFFFFFFF); //最多15个数字
}

long long CMD5::APHash(const char* pSrc)
{
	long long hash = 0;

	for (int i = 0; *pSrc; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*pSrc++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*pSrc++) ^ (hash >> 5)));
		}
	}

	return (hash & 0x7FFFFFFFFFFF); //最多15个数字
}