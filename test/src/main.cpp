#include <iostream>
#include "../include/Public.hpp"
#include "SecurityModule/include/MyOpenssl.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	cout << CPublic::DateTimeString(3) << " >>" << "Hello world !" << endl;

	unsigned char key[16]{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06 };
	unsigned char iv[16]{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06 };

	string szTemp, szTemp2, szHash, szError;
	COpenssl aes(false);

	aes.Hash(COpenssl::E_MD_MD5, "123", szHash, false, &szError);
	cout << "md5:" << szHash << endl;

	aes.SetKeyAndIV(COpenssl::E_KEY_64, (const char*)key, 8, (const char*)iv, 16, szError);

	int iRet = aes.Encrypt(COpenssl::E_CIPHER_AES, COpenssl::E_KEY_64, "123456sw", szTemp, szError);
	cout << iRet << ":" << szTemp << " => " << szError << endl;

	iRet = aes.Decrypt(COpenssl::E_CIPHER_AES, COpenssl::E_KEY_64, szTemp, szTemp2, szError);
	cout << iRet << ":" << szTemp2 << " => " << szError << endl;

	//暂停
	system("pause");

	return 0;
}