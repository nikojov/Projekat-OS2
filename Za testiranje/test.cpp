#include <iostream>
#include <vector>
#define  N 100000
int main() {

	std::vector<bool> test(N);
	if(N==test.size())
	std::cout << "uspeh" << std::endl;
	else std::cout << "neuspeh" << std::endl;
	int a;
	std::cin >> a;
	return 0;
}