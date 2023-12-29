

#pragma once

#include <stack>
#include <mutex>
#include <memory>
#include <functional>
#include <shared_mutex>

namespace std
{
    template <typename _Tp, typename _Container = deque<_Tp>>
    class threadsafe_stack
    {
    private:
        typedef std::stack<_Tp, _Container> __stack_type;
        
        mutable std::shared_timed_mutex __mutex_;
        __stack_type __internal_stack_;
        
    public:
        typedef          __stack_type                  stack_type;
        typedef typename __stack_type::container_type  container_type;
        typedef typename __stack_type::value_type      value_type;
        typedef typename __stack_type::reference       reference;
        typedef typename __stack_type::const_reference const_reference;
        typedef typename __stack_type::size_type       size_type;
        
    public:
        threadsafe_stack() : __internal_stack_() {}
        threadsafe_stack(const stack_type& __s) : __internal_stack_(__s) {}
        threadsafe_stack(stack_type&& __s) : __internal_stack_(std::move(__s)) {}
        explicit threadsafe_stack(const container_type& __c) : __internal_stack_(__c) {}
        explicit threadsafe_stack(container_type&& __c) : __internal_stack_(std::move(__c)) {}
        
        threadsafe_stack(const threadsafe_stack&) = delete;
        threadsafe_stack& operator=(const threadsafe_stack&) = delete;
        threadsafe_stack(threadsafe_stack&&) = delete;
        threadsafe_stack& operator=(threadsafe_stack&&) = delete;
        
    public:
        void swap(stack_type& other)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            other.swap(__internal_stack_);
        }
    
        bool empty() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_stack_.empty();
        }
        
        size_type size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_stack_.size();
        }
        
        const value_type& top()
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return static_cast<const value_type&>(__internal_stack_.top());
        }
        
        void push(const value_type& __x)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_stack_.push(__x);
        }
        
        void pop()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_stack_.pop();
        }
    };
}
