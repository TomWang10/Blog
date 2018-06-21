# 右值引用

----------
### 1. 什么是左值和右值

有几种区分方法:  
* 左值就是即能出现在等号左边也能出现在等号右变的变量(表达式), 右值就只能出现在等号的右边  
* 能取地址的就是左值，不能取地址的就是右值  

```c++
int i, j, k;
i = 1; // i是左值, 1是右值
i = j; // i, j 都是左值 1 = i; // 错误, 1是右值
```

### 2. 什么是左值引用和右值引用

如果左值同时又有`&`标识符，就被称为`左值引用`

```c++
int i = 0;
int& j = i; // j为左值引用
int& k = 1; // 错误，右值是不可修改的，所以k被绑定到1是语法错误
const int& k2 = 1 // 正确，const 左值引用不能被修改
```

右值通过`&&`符号表明它是一个`右值引用`，只能绑定到右值上

```c++
int i = 0;
int&& j = i; // 错误，右值引用不能绑定到左值上
int&& k = 1; // 正确
int&& k2 = i + 42; // 正确, i + 42是右值
int& k3 = i + 42; // 错误
```

### 3.右值引用它要解决什么问题，怎么使用

c++11引入右值引用主要解决两个问题:  
#### 1.对需要拷贝的`临时对象(右值)`进行`移动`操作，提升效率  

在c++11以前，因为想避免深拷贝的性能损失，我们把本应该写成

```c++
vector<int> GetResource(int args); // (1)
```

写成了

```c++
void GetResouce(int args, vector<int>* output); // (2)
```

在c++中，我们可以大胆的写成(1)这种形式，如果返回的是栈对象，就会优先调用移动构造函数，而不是拷贝构造函数  
在我们写函数声明的时候，只有当:  
1. 函数内需要copy参数时
2. 需要将参数保存到栈外内存时  

我们可以把参数声明为右值引用，比如标准库里的`vertor`的`push_back`  

```c++
void push_back(const T& v);
void push_back(T&& v);
```

如果只满足第一个条件，我们可以使用  

```c++
void Func(const T& v)
{
    // 这里直接调用一次copy构造函数
    local l = v;
}
```

#### 2.在模板函数中按参数的实际类型进行转发  

```c++
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
    // 使用forward把值的类型原样转发给Fun
    Fun(std::forward<T>(v));
}

int main()
{
    ForwardValue(1);
    int a = 1;
    ForwardValue(a);
    return 0;
}
```
