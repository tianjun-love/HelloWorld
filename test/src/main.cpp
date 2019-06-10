#include <iostream>
#include "../include/Public.hpp"
#include "LogFile/include/LogFile.hpp"
#include "SecurityModule/include/MyOpenssl.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	string szTemp, szTemp2, szHash, szError;
	COpenssl aes(false);

	//aes.RSAGenerateKeyPair("pub.pem", "pri.pem", "123456", 2048, szError);

	unsigned char *pOut = nullptr;
	unsigned int uiOutSize = 0;
	bool bx = aes.RSASignFile("pri.pem", "123456", COpenssl::E_MD_SHA1, "pub.pem", pOut, uiOutSize, szError);

	bx = aes.RSAVerifySign("pub.pem", COpenssl::E_MD_SHA1, "pri.pem", pOut, uiOutSize, szError);

	delete[] pOut;
	pOut = nullptr;

	return 0;

	CLogFile log("../log/log", "debug", "day");

	log << E_LOG_LEVEL_DEBUG << "Hello world !" << logendl;

	unsigned char key[16]{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06 };
	unsigned char iv[16]{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06 };

	aes.Hash(COpenssl::E_MD_MD5, "123", szHash, false, &szError);
	log << E_LOG_LEVEL_DEBUG << "123 md5:" << szHash << logendl;

	aes.SetKeyAndIV(COpenssl::E_KEY_64, (const char*)key, 8, (const char*)iv, 16, szError);

	int iRet = aes.Encrypt(COpenssl::E_CIPHER_AES, COpenssl::E_KEY_64, "123456sw", szTemp, szError);
	log << E_LOG_LEVEL_DEBUG << iRet << ":" << szTemp << " => " << szError << logendl;

	iRet = aes.Decrypt(COpenssl::E_CIPHER_AES, COpenssl::E_KEY_64, szTemp, szTemp2, szError);
	log << E_LOG_LEVEL_DEBUG << iRet << ":" << szTemp2 << " => " << szError << logendl;

	//关闭日志
	log.Close();

	//暂停
	system("pause");

	return 0;
}