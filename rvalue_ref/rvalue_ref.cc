#include <iostream>

using namespace std;

class Test
{
public:
    Test(): value_(new int(0)) { cout << "constructor\n";}
    Test(const Test& t): value_(new int(*t.value_)) { cout << "copy constructor\n";}
    /*
    Test(Test&& t): value_(t.value_) { 
        value_ = nullptr;
        cout << "move constructor\n"; 
    }
    */
    ~Test() { cout << "destructor\n"; }

    int* value_;
};

Test GetTest()
{
    return Test();
}

void Fun(const int&)
{
    std::cout << "lvalue" << std::endl;
}

void Fun(int&& i)
{
    std::cout << "rvalue" << std::endl;
}

template <typename T>
void ForwardValue(T&& v)
{
    Fun(std::forward<T>(v));
}

int main()
{
    ForwardValue(1);
    int a = 1;
    ForwardValue(a);
    //Test test = GetTest();
    return 0;
}
