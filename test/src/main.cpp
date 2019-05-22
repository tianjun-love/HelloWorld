#include <iostream>
#include "../include/Public.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	cout << CPublic::DateTimeString(3) << " >>" << "Hello world !" << endl;

	//暂停
	system("pause");

	return 0;
}