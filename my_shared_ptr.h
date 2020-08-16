//
// Created by HP on 2020/8/15.
//

#ifndef STL_MY_ALLOCATOR_MY_SHARED_PTR_H
#define STL_MY_ALLOCATOR_MY_SHARED_PTR_H



// 底层引用计数类（不可拷贝、不可赋值，因为每块内存空间只能拥有一个引用计数，不存在多个引用计数类维护一个内存空间）
// 也就是它是独一无二的，永远保证一对一
// 抽象类
class __Sp_counted_base {
public:
    // 构造函数
    __Sp_counted_base() : use_count(1), weak_count(1) { }

    // 拷贝构造函数（不可拷贝）
    __Sp_counted_base(const __Sp_counted_base&) = delete;

    // 拷贝赋值运算符（不可赋值）
    __Sp_counted_base& operator=(const __Sp_counted_base&) = delete;

    // 析构函数
    virtual ~__Sp_counted_base() = default;

    // 获取use_count
    long _M_get_use_count() const {
        return use_count;
    }

    // 减少weak_count
    void _M_weak_release() {
        --weak_count;
        if (weak_count == 0)
            _M_destroy();
    }

    // 增加weak_count
    void _M_weak_add_ref() {
        ++weak_count;
    }

    // 减少use_count
    void _M_release() {
        --use_count;
        if (use_count == 0) {
            _M_dispose();
            _M_weak_release();
        }
    }

    // 增加use_count
    void _M_add_ref_copy() {
        ++use_count;
    }

    // 引用计数weak_count为0时，调用此函数回收引用计数对象（管理对象），也就是当前对象
    virtual void _M_destroy() {
        delete this;
    }

    // 引用计数use_count为0时，调用此函数回收内存空间，回收操作需要子类完成
    virtual void _M_dispose() = 0;

private:
    int use_count;  // 指针指向内存（被管理对象）的引用计数
    int weak_count; // 当前管理对象的引用计数
};

// 继承上一个抽象类，实际上也是引用计数类，加多了一个指向内存空间的指针
// 独一无二，不可拷贝和赋值。因为每一块内存只能由一个引用计数类来维护
// 永远保证一对一
template <class PTR, class Deleter>
class __Sp_counted_base_impl : public __Sp_counted_base {
public:
    // 构造函数，必须指定删除器
    __Sp_counted_base_impl(PTR p, Deleter d) : ptr(p), del(d) {}

    // 虚析构函数，使得可以用父类指针释放基类对象
    virtual ~__Sp_counted_base_impl() = default;

    __Sp_counted_base_impl(const __Sp_counted_base_impl&) = delete;
    __Sp_counted_base_impl& operator=(const __Sp_counted_base_impl&) = delete;

//    // 引用计数weak_count为0时，调用此函数回收引用计数对象（管理对象），也就是当前对象
//    virtual void _M_destroy() {
//        delete this;
//    }

    // 引用计数use_count为0时，调用此函数回收内存空间。实现父类的抽象方法
    virtual void _M_dispose() {
        // 借用del删除器（其实就是一个函数对象），默认是delete
        del(ptr);
    }

private:
    PTR ptr;     // 指向内存空间
    Deleter del;// 删除器，用于在引用计数use_count为0时析构对象，回收内存空间
};

// 默认的删除器，也就是将delete函数封装成一个函数对象
template <class T>
struct __Sp_deleter {
    void operator()(T* ptr) {
        delete ptr;
    }
};

template <class T>
class __shared_count {
public:

    // 构造函数
    __shared_count() noexcept : count_ptr (0) { }

    template <class PTR>
    __shared_count(PTR p) : count_ptr (0) {
        // 去掉PTR的指针属性，获取指针指向的类型，用于传给默认删除器
        typedef typename std::remove_pointer<PTR>::type Tp;
        // 新建引用计数对象，使用默认的删除器
        count_ptr = new __Sp_counted_base_impl<PTR, __Sp_deleter<Tp>>(p, __Sp_deleter<Tp>());
    }

    template <class PTR, class Deleter>
    __shared_count(PTR p, Deleter d) : count_ptr(0) {
        // 新建引用计数对象，并且使用指定的删除器
        count_ptr = new __Sp_counted_base_impl<PTR, Deleter>(p, d);
    }

    // 析构函数，说明外层shared_ptr被析构，引用计数减1
    ~__shared_count() {
        count_ptr->_M_release();
    }

    // 拷贝构造函数，说明外层shared_ptr被拷贝，引用计数加1
    __shared_count(const __shared_count& s) : count_ptr(s.count_ptr) {
        if (count_ptr != 0)
            count_ptr->_M_add_ref_copy();
    }

    // 拷贝赋值运算符
    __shared_count& operator=(const __shared_count& s) {
        if (s.count_ptr != count_ptr) {
            // 原来的引用计数减1
            if (count_ptr != 0)
                count_ptr->_M_release();
            // 新的引用计数加1
            if (s.count_ptr != 0)
                s.count_ptr->_M_add_ref_copy();
            count_ptr = s.count_ptr;
        }
        return *this;
    }

    long _M_get_use_count() const {
        return count_ptr->_M_get_use_count();
    }

private:
    __Sp_counted_base* count_ptr;
};

template <class T>
class weak_ptr {

};


template <class T>
class shared_ptr {
public:
    typedef T element_type;

    // 构造函数
    shared_ptr() noexcept : ptr(0), ref_count() { }

    template <class T1>
    explicit shared_ptr(T1* p) noexcept : ptr(p), ref_count(p) { }

    template <class T1, class Deleter>
    explicit shared_ptr(T1* p, Deleter d) : ptr(p), ref_count(p, d) { }

    // 拷贝构造函数
    shared_ptr(const shared_ptr& r) : ptr(r.ptr), ref_count(r.ref_count) { }

    template <class T1>
    shared_ptr(const shared_ptr<T1>& r) : ptr(r.ptr), ref_count(r.ref_count) { }

    template <class T1>
    shared_ptr(const weak_ptr<T1>& r) : ptr(r.ptr), ref_count(r.ref_count) { }

    // 拷贝赋值运算符
    shared_ptr& operator=(const shared_ptr& r) noexcept {
        this->ptr = r.ptr;
        this->ref_count = r.ref_count;
        return *this;
    }

    template <class T1>
    shared_ptr& operator=(const shared_ptr<T1>& r) noexcept {
        this->ptr = r.ptr;
        this->ref_count = r.ref_count;
        return *this;
    }

    // 析构函数
    ~shared_ptr() = default;

    // 获取指针
    element_type* get() {
        return ptr;
    }

    // 获取引用计数
    long use_count() const {
        return ref_count._M_get_use_count();
    }

private:
    element_type* ptr;
    __shared_count<T> ref_count;
};



#endif //STL_MY_ALLOCATOR_MY_SHARED_PTR_H
