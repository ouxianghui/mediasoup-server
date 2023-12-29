

#pragma once

#include <mutex>
#include <deque>
#include <memory>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <shared_mutex>

namespace std
{
    template <typename _Tp, typename _Allocator = allocator<_Tp>>
    class threadsafe_deque
    {
    private:
        typedef std::deque<_Tp, _Allocator> __deque_type;
        
        mutable std::shared_timed_mutex __mutex_;
        __deque_type __internal_queue_;
        
    public:
        typedef _Tp                                             value_type;
        typedef _Allocator                                      allocator_type;
        
        typedef          __deque_type                           deque_type;
        typedef typename __deque_type::reference                reference;
        typedef typename __deque_type::const_reference          const_reference;
        typedef typename __deque_type::size_type                size_type;
        typedef typename __deque_type::difference_type          difference_type;
        typedef typename __deque_type::pointer                  pointer;
        typedef typename __deque_type::const_pointer            const_pointer;
        typedef typename __deque_type::iterator                 iterator;
        typedef typename __deque_type::const_iterator           const_iterator;
        typedef typename __deque_type::reverse_iterator         reverse_iterator;
        typedef typename __deque_type::const_reverse_iterator   const_reverse_iterator;
        
    public:
        threadsafe_deque() : __internal_queue_() {}
        explicit threadsafe_deque(size_type __n) : __internal_queue_(__n) {}
        threadsafe_deque(size_type __n, const value_type& __v) : __internal_queue_(__n, __v) {}
        threadsafe_deque(const deque_type& __l) : __internal_queue_(__l) {}
        threadsafe_deque(deque_type&& __l) : __internal_queue_(std::move(__l)) {}
        threadsafe_deque(initializer_list<value_type> __il) : __internal_queue_(__il) {}
        
        template <class _InputIterator>
        threadsafe_deque(_InputIterator __f, _InputIterator __l) : __internal_queue_(__f, __l) {}
        
        threadsafe_deque(const threadsafe_deque&) = delete;
        threadsafe_deque& operator=(const threadsafe_deque&) = delete;
        threadsafe_deque(threadsafe_deque&&) = delete;
        threadsafe_deque& operator=(threadsafe_deque&&) = delete;
        
    public:
        template <class _InputIterator>
        void assign(_InputIterator __f, _InputIterator __l)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.assign(__f, __l);
        }
        
        void assign(size_type __n, const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.assign(__n, __v);
        }
        
        void assign(initializer_list<value_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.assign(__il);
        }
    
        void swap(deque_type& other)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            other.swap(__internal_queue_);
        }
        
        bool empty() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_queue_.empty();
        }
        
        size_type size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_queue_.size();
        }
        
        size_type max_size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_queue_.max_size();
        }
        
        void resize(size_type __n)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.resize(__n);
        }
        
        void resize(size_type __n, const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.resize(__n, __v);
        }
        
        void shrink_to_fit()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.shrink_to_fit();
        }
        
        const value_type& front() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return static_cast<const value_type&>(__internal_queue_.front());
        }
        
        const value_type& back() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return static_cast<const value_type&>(__internal_queue_.back());
        }
        
        void push_front(const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.push_front(__v);
        }
        
        void push_back(const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.push_back(__v);
        }
        
        void pop_front()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.pop_front();
        }
        
        void pop_back()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.pop_back();
        }
        
        void clear()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_.clear();
        }
        
        const value_type& operator[](size_type __n) const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_queue_[__n];
        }
        
        const value_type& at(size_type __n) const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_queue_.at(__n);
        }
        
        void set(size_type __n, const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_[__n] = __v;
        }
        
        void operator=(const deque_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_ = __v;
        }
        
        void operator=(initializer_list<deque_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_queue_ = __il;
        }
        
        deque_type value()
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_queue_;
        }
        
        void erase(std::function<bool(const value_type&)> __comp)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            for (const_iterator it = __internal_queue_.begin(); it != __internal_queue_.end();)
            {
                if (__comp(*it))
                {
                    it = __internal_queue_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    
        void erase(std::size_t __pos)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
        
            iterator it = __internal_queue_.begin();
    
            std::advance(it, __pos);
        
            __internal_queue_.erase(it);
        }
        
        template <typename _Pred>
        std::pair<const value_type, bool> find_and_erase(_Pred __pred)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            const_iterator it = std::find_if(__internal_queue_.begin(), __internal_queue_.end(), __pred);
            if (it != __internal_queue_.end())
            {
                value_type r = *it;
                __internal_queue_.erase(it);
                return std::make_pair(r, true);
            }
            
            return std::make_pair(value_type(), false);
        }
        
        void insert(std::function<const_iterator(const deque_type&)> __pos, const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            const_iterator pos = __pos(__internal_queue_);
            
            __internal_queue_.insert(pos, __v);
        }
        
        void insert(std::function<const_iterator(const deque_type&)> __pos, size_type __n, const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            const_iterator pos = __pos(__internal_queue_);
            
            __internal_queue_.insert(pos, __n, __v);
        }
        
        template <class _InputIterator>
        void insert(std::function<const_iterator(const deque_type&)> __pos, _InputIterator __f, _InputIterator __l)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            const_iterator pos = __pos(__internal_queue_);
            
            __internal_queue_.insert(pos, __f, __l);
        }
        
        void insert(std::function<const_iterator(const deque_type&)> __pos, initializer_list<value_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            const_iterator pos = __pos(__internal_queue_);
            
            __internal_queue_.insert(pos, __il);
        }
    
        void insert(std::size_t __pos, const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            iterator it = __internal_queue_.begin();
        
            std::advance(it, __pos);
            
            __internal_queue_.insert(it, __v);
        }
        
        void for_each(std::function<void(const value_type&)> __bl)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            for (const auto& v : __internal_queue_)
            {
                __bl(v);
            }
        }
        
        void for_all(std::function<void(const deque_type&)> __bl)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            __bl(__internal_queue_);
        }
    };
}
