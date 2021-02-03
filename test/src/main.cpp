#include <iostream>
#include "../include/XmlTest.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	string szTemp, szError;
	CXmlTest x;

	x.m_szName = "TianJun";
	x.m_iAge = 30;
	x.m_szGender = "man";
	x.m_szTestStr = "xxooxooxoxoxoxxoxo";
	x.m_FriendVec.push_back("ma1");
	x.m_FriendVec.push_back("xd2");
	x.m_szLike1 = "xxoo";
	x.m_szLike2 = "basket";
	x.m_szOther = "eat something.";

	if (x.SaveFile("tj.txt", szError))
		cout << "save success." << endl;
	else
		cout << "save failed:" << szError << endl;

	//暂停
#ifdef _WIN32
	system("pause");
#endif

	return 0;
}