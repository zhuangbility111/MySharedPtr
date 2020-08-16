#include <iostream>
#include "my_shared_ptr.h"

void print(shared_ptr<int> tmp) {
    std::cout << *(tmp.get()) << std::endl;
    *(tmp.get()) = 11;
    std::cout << tmp.use_count() << std::endl;
    shared_ptr<int> new_temp(new int(12));
    new_temp = tmp;
    std::cout << *(new_temp.get()) << std::endl;
    std::cout << new_temp.use_count() << std::endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    shared_ptr<int> s(new int(10));
    print(s);
    std::cout << *(s.get()) << std::endl;
    std::cout << s.use_count() << std::endl;
    return 0;
}