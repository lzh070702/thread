# 线程池中的 C++ 核心特性讲解

## 1. Lambda 表达式

### 代码中的使用

```cpp
// 工作线程的 lambda
thd.emplace_back([this]() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !works.empty() || !runflag; });
            // ...
        }
        if (task) task();
    }
});

// 任务包装的 lambda
works.emplace([task]() { (*task)(); });
```

### 捕获列表详解

```cpp
[this]              // 捕获 this 指针，访问成员变量
[task]() { ... }    // 以值方式捕获 task（拷贝）
```

| 语法 | 含义 |
|-----|------|
| `[]` | 不捕获 |
| `[=]` | 值捕获所有变量 |
| `[&]` | 引用捕获所有变量 |
| `[this]` | 捕获 this 指针 |
| `[x]` | 值捕获 x |
| `[&x]` | 引用捕获 x |

### C++14 初始化捕获（广义捕获）

```cpp
[func = std::forward<F>(f),           // 用表达式初始化捕获变量
 args = std::make_tuple(...)]()       // 捕获 tuple
```

**为什么需要**：
```cpp
// C++11 无法直接捕获右值
[std::move(ptr)]      // ❌ 编译错误

// C++14 可以
[ptr = std::move(ptr)] // ✅ 正确
```

---

## 2. 左值与右值

### 核心概念

```cpp
int x = 10;           // x 是左值（有名字，可取地址）
int&& r = 20;         // 20 是右值（临时值，无名字）
```

| 特性 | 左值 | 右值 |
|-----|------|------|
| 有名字 | ✅ | ❌ |
| 可取地址 | ✅ | ❌ |
| 可赋值 | ✅ | ❌ |
| 生命周期 | 持久 | 短暂 |

### 代码中的体现

```cpp
// works.front() 返回左值引用
std::function<void()> task = std::move(works.front());
//                              ^^^^
//                              将左值转为右值引用，触发移动而非拷贝
```

### 移动语义的作用

```cpp
// 不用 move：拷贝构造（深拷贝，分配新内存）
std::function<void()> task = works.front();

// 使用 move：移动构造（转移资源所有权，高效）
std::function<void()> task = std::move(works.front());
// 执行后 works.front() 变为"空"状态
```

---

## 3. 移动语义

### 代码中的使用

```cpp
// 1. 从队列中移动取出任务
task = std::move(works.front());

// 2. 完美转发中的移动
[func = std::forward<F>(f)]  // 根据 F 的值类别决定移动或拷贝
```

### 为什么需要移动

```cpp
std::function 内部可能持有：
- 动态分配的内存（存储捕获的变量）
- 函数指针或虚函数表指针

// 拷贝：深拷贝所有资源（慢）
// 移动：转移指针所有权（快）
```

### 移动后的状态

```cpp
auto task = std::move(works.front());
// works.front() 仍然有效，但处于"未指定状态"
// 通常为空或默认构造状态
// 随后 works.pop() 将其销毁
```

---

## 4. 完美转发

### 代码中的使用

```cpp
template <typename F, typename... Args>
auto enqueue(F&& f, Args&&... args) -> ...
//           ^^^^    ^^^^^^^^^^^^
//           万能引用（Universal Reference）
```

### 万能引用 vs 右值引用

```cpp
template<typename T> void foo(T&& param);        // ✅ 万能引用
void bar(std::vector<int>&& param);              // ❌ 右值引用
template<typename T> void baz(std::vector<T>&&); // ❌ 右值引用

// 万能引用：T 是推导类型，&& 是万能引用
// 右值引用：具体类型后的 && 是右值引用
```

### 引用折叠规则

```cpp
T&  &   -> T&   // 左值引用 + & = 左值引用
T&  &&  -> T&   // 左值引用 + && = 左值引用
T&& &   -> T&   // 右值引用 + & = 左值引用
T&& &&  -> T&&  // 右值引用 + && = 右值引用
```

### std::forward 的作用

```cpp
[func = std::forward<F>(f)]  // 保持值类别转发
```

| 传入类型 | F 推导为 | forward<F> 结果 |
|---------|---------|----------------|
| 左值 | `T&` | `T&`（左值引用） |
| 右值 | `T` | `T&&`（右值引用） |

**为什么不用 std::move**：
```cpp
// move 总是转为右值
std::move(x)  // 总是 T&&

// forward 根据原始类型决定
std::forward<T>(x)  // T& -> T&, T -> T&&
```

### 线程池中的应用

```cpp
template <typename F, typename... Args>
auto enqueue(F&& f, Args&&... args) {
    // 1. 万能引用接收任意值类别
    // 2. forward 保持原始值类别传递给 lambda
    [func = std::forward<F>(f),
     args = std::make_tuple(std::forward<Args>(args)...)]() {
        // ...
    }
}
```

---

## 5. 参数打包

### 代码中的使用

```cpp
args = std::make_tuple(std::forward<Args>(args)...)
```

### 参数包展开

```cpp
// 调用 enqueue(func, a, b, c)
// Args... 推导为 Arg1, Arg2, Arg3
// 展开为：
args = std::make_tuple(
    std::forward<Arg1>(a),
    std::forward<Arg2>(b),
    std::forward<Arg3>(c)
);
// 类型：std::tuple<Arg1, Arg2, Arg3>
```

### 为什么打包

```cpp
// 问题：需要延迟执行，但参数现在就确定了
// 解决：将参数保存到 tuple 中

// 1. 延迟执行
auto task = std::make_shared<std::packaged_task<return_type()>>(
    [args = std::make_tuple(...)]() mutable {  // 现在打包
        // 稍后执行时才解包调用
        return std::apply(func, args);
    }
);

// 2. 避免悬空引用
// 如果直接捕获引用，函数返回后参数可能已销毁
// 打包成 tuple 是值拷贝，安全
```

### std::apply 解包

```cpp
// C++17
return std::apply(func, args);
// 将 tuple 元素作为参数展开调用
// std::tuple<int, double> args = {1, 2.0};
// std::apply(func, args) 等价于 func(1, 2.0);
```

---

## 6. std::future

### 代码中的使用

```cpp
auto enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    
    // 创建 packaged_task
    auto task = std::make_shared<std::packaged_task<return_type()>>(...);
    
    // 获取 future
    std::future<return_type> res = task->get_future();
    
    // 将任务入队
    works.emplace([task]() { (*task)(); });
    
    return res;  // 返回 future 给用户
}
```

### future 的作用

```cpp
// 用户代码
auto future = pool.enqueue(add, 3, 5);
int result = future.get();  // 阻塞等待结果
```

### 与 packaged_task 的关系

```cpp
std::packaged_task<int()> task([]() { return 42; });
//              ^^^^^^^^^^
//              函数签名：无参数，返回 int

std::future<int> fut = task.get_future();
// future 类型与 task 的返回类型匹配

task();  // 执行（可在任意线程）
int result = fut.get();  // 获取结果
```

### 为什么用 shared_ptr

```cpp
// packaged_task 不可拷贝，只能移动
// 但我们需要：
// 1. 将 task 存入队列（需要拷贝或共享）
// 2. 保留获取结果的能力

auto task = std::make_shared<std::packaged_task<...>>(...);
std::future<...> res = task->get_future();  // 保留结果通道

works.emplace([task]() { (*task)(); });  // 共享所有权给队列
// shared_ptr 的拷贝是廉价的（引用计数）
```

### future 的关键方法

| 方法 | 作用 |
|-----|------|
| `get()` | 阻塞等待结果，只能调用一次 |
| `wait()` | 阻塞等待完成，不获取值 |
| `valid()` | 检查是否有关联状态 |

```cpp
auto f = pool.enqueue(func);
if (f.valid()) {
    auto result = f.get();  // 阻塞直到任务完成
}
// f.get() 之后 f.valid() 变为 false
```

---

## 流程总结

```
用户调用：
pool.enqueue(func, arg1, arg2)
       │
       ▼
┌─────────────────────────────────────┐
│ 1. 万能引用接收参数                  │
│    F&& f, Args&&... args            │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 2. 完美转发打包参数                  │
│    args = make_tuple(forward(args)...) │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 3. 创建 packaged_task               │
│    包装：[func, args]() {           │
│        return apply(func, args);    │
│    }                                │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 4. 获取 future                      │
│    res = task->get_future()         │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│ 5. 任务入队（shared_ptr 共享）       │
│    works.emplace([task](){...})     │
└─────────────────────────────────────┘
       │
       ▼
    返回 future 给用户
       │
       ▼
用户调用 future.get() ──→ 阻塞等待 ──→ 获取结果
```
