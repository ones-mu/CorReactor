## const关键字放到成员函数声明的末尾

const关键字放到成员函数声明的末尾 表示这是一个常量成员函数(const member function)
const是函数签名的一部分，所以声明和定义必须同时出现。
主要作用是：1.不会修改类中的任何非静态成员变量（除非变量被声明为mutable）
除此之外，2.常量对象(类初始化对象的时候前面加const)只能调用const成员函数，不能调用非const成员函数。
3.可以同时存在const和非const的重载成员函数，编译器会根据对象的常量性选择调用合适的版本。

## c++中的static关键字

1.静态局部变量：延长局部变量的生命周期，使其在程序整个运行期间存在（而非仅在作用域内有效）。可以使得调用结束后仍保留其值。
有默认初始化（0或空指针）
2.静态全局变量/函数  类型与namespace
3.静态成员变量：用static关键字定义的成员变量属于类本身，而非类的单个对象，所有对象共享同一份静态变量。
其必须在类外单独初始化（初始化时不体现static关键字）。不占用对象的内存空间（存储在全局数据区）。可通过类名加冒号直接访问。
4.静态成员函数：用static关键字定义的成员函数属于类本身，而不是类的对象，不依赖对象实例即可调用。
无this指针，只能访问静态成员变量或调用其他静态函数（类外全局变量还是可以正常访问的）。可通过类名加冒号直接访问该函数。

## thread_local

thread_local 是 C++11 引入的存储期说明符（storage specifier），用于声明线程局部变量（Thread-Local Storage, TLS）。它的核心特性是：
    ​​线程独立性​​：每个线程拥有该变量的独立副本，线程之间互不干扰。
    ​​生命周期​​：
        对于静态变量（如你的例子）：生命周期与线程相同（线程创建时初始化，线程结束时销毁）。
        对于非静态局部变量：在首次进入作用域时初始化，线程退出时销毁。

## throw std::logic_error("pthread_join error")

c++标准库的异常类，
头文件是#include <stdexcept>
std::logic_error 继承自 std::exception，其派生类还包括 std::invalid_argument、std::domain_error 等

## hook

hook的目的就是为了将同步的io转成异步的io，让写逻辑的人用同步的方式性能上实现异步的性能，如果直接写异步就是一堆回调链路，很容易出问题

## std::transform

std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);

### std::vector::reserve

`std::vector::reserve` 是 C++ 标准库中 vector 容器的一个重要方法，用于预分配内存空间。

### 基本用法

```cpp
void reserve(size_type n);
```

### 功能说明

- `reserve(n)` 请求 vector 分配至少能容纳 n 个元素的连续内存空间
- 如果 n 大于当前 `capacity()`，则会重新分配内存，使 `capacity()` 至少为 n
- 如果 n 小于或等于当前 `capacity()`，则该方法不会产生任何效果
- 该方法**不会**改变 vector 的 `size()`，只是预分配内存

### 为什么使用 reserve

使用 `reserve` 的主要好处是：

1. **避免多次内存分配**：当你知道要存储大量元素时，可以一次性分配足够内存
2. **提高性能**：减少因自动扩容导致的内存重新分配和数据拷贝
3. **保持迭代器有效性**：预分配内存后，在容量范围内的插入操作不会使迭代器失效

### 示例代码

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec;
    
    // 预分配空间
    vec.reserve(100);
    
    std::cout << "Capacity after reserve: " << vec.capacity() << std::endl; // >= 100
    std::cout << "Size after reserve: " << vec.size() << std::endl;         // 0
    
    // 添加元素不会触发重新分配，直到超过预留容量
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i);
    }
    
    std::cout << "Size after adding elements: " << vec.size() << std::endl; // 100
    std::cout << "Capacity after adding elements: " << vec.capacity() << std::endl; // >= 100
    
    return 0;
}
```

### 注意事项

1. `reserve` 只是预留内存空间，不会创建实际元素
2. 与 `resize` 不同，`reserve` 不会改变 vector 的 `size()`
3. 实际分配的容量可能比请求的略大，这取决于具体实现
4. 过度预留可能导致内存浪费

### 性能考虑

在知道最终元素数量的情况下，使用 `reserve` 可以显著提高性能，特别是在处理大量数据时。这避免了 vector 在增长过程中多次分配内存和拷贝数据。
