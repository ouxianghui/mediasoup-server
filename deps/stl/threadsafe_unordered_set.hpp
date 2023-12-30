

#pragma once

#include <mutex>
#include <memory>
#include <utility>
#include <functional>
#include <shared_mutex>
#include <unordered_set>

namespace std
{
    template <
              typename _Value,
              typename _Hash = hash<_Value>,
              typename _Pred = equal_to<_Value>,
              typename _Alloc = allocator<_Value>
             >
    class threadsafe_unordered_set
    {
    public:
        typedef _Value                                      key_type;
        typedef key_type                                    value_type;
        typedef _Hash                                       hasher;
        typedef _Pred                                       key_equal;
        typedef _Alloc                                      allocator_type;
        typedef value_type&                                 reference;
        typedef const value_type&                           const_reference;
        
    private:
        typedef std::unordered_set<value_type, hasher, key_equal, allocator_type> __set_type;
        
        mutable std::shared_timed_mutex __mutex_;
        __set_type __internal_set_;
        
    public:
        typedef          __set_type                         set_type;
        typedef typename __set_type::pointer                pointer;
        typedef typename __set_type::const_pointer          const_pointer;
        typedef typename __set_type::size_type              size_type;
        typedef typename __set_type::difference_type        difference_type;
        typedef typename __set_type::iterator               iterator;
        typedef typename __set_type::const_iterator         const_iterator;
        typedef typename __set_type::local_iterator         local_iterator;
        typedef typename __set_type::const_local_iterator   const_local_iterator;
        
    public:
        threadsafe_unordered_set() : __internal_set_() {}
        threadsafe_unordered_set(const set_type& __s) : __internal_set_(__s) {}
        threadsafe_unordered_set(set_type&& __s) : __internal_set_(std::move(__s)) {}
        threadsafe_unordered_set(initializer_list<value_type> __il) : __internal_set_(__il) {}
        
        template <class _InputIterator>
        threadsafe_unordered_set(_InputIterator __f, _InputIterator __l) : __internal_set_(__f, __l) {}
        
        threadsafe_unordered_set(const threadsafe_unordered_set&) = delete;
        threadsafe_unordered_set& operator=(const threadsafe_unordered_set&) = delete;
        threadsafe_unordered_set(threadsafe_unordered_set&&) = delete;
        threadsafe_unordered_set& operator=(threadsafe_unordered_set&&) = delete;
        
    public:
        void swap(set_type& other)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            other.swap(__internal_set_);
        }
    
        bool empty() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.empty();
        }
        
        size_type size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.size();
        }
        
        size_type max_size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.max_size();
        }
        
        void operator=(const set_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_ = __v;
        }
        
        void operator=(initializer_list<set_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_ = __il;
        }
        
        set_type value()
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_;
        }
        
        set_type set_intersection(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r;
            for (auto v : s)
            {
                auto it = __internal_set_.find(v);
                if (it != __internal_set_.end())
                {
                    r.insert(v);
                }
            }
            return r;
        }
        
        set_type set_union(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r(__internal_set_);
            for (auto v : s)
            {
                auto it = r.find(v);
                if (it == r.end())
                {
                    r.insert(v);
                }
            }
            return r;
        }
        
        set_type set_different(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r(__internal_set_);
            for (auto v : s)
            {
                auto it = r.find(v);
                if (it != r.end())
                {
                    r.erase(v);
                }
            }
            return r;
        }
        
        set_type set_symmetric_difference(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r(s);
            for (auto v : __internal_set_)
            {
                auto it = s.find(v);
                if (it != s.end())
                {
                    r.erase(v);
                }
                else
                {
                    r.insert(v);
                }
            }
            return r;
        }
        
        template <class... _Args>
        bool emplace(_Args&&... __args)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.emplace(std::forward<_Args>(__args)...).second;
        }
        
        bool insert(const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.insert(__v).second;
        }
        
        void insert(initializer_list<value_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.insert(__il);
        }
        
        template <class _InputIterator>
        void insert(_InputIterator __f, _InputIterator __l)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.insert(__f, __l);
        }
        
        const std::pair<const value_type, bool> get(const key_type& __k)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            auto it = __internal_set_.find(__k);
            if (it == __internal_set_.end())
            {
                return std::make_pair(value_type(), false);
            }
            else
            {
                return std::make_pair(*it, true);
            }
        }
        
        void clear()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.clear();
        }
        
        bool contains(const key_type& __k)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            auto it = __internal_set_.find(__k);
            return it != __internal_set_.end();
        }
        
        size_type erase(const key_type& __k)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.erase(__k);
        }
        
        void for_each(std::function<void(const value_type&)> __bl)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            for (const auto& v : __internal_set_)
            {
                __bl(v);
            }
        }
        
        void for_each2(std::function<bool(const value_type&)> __bl)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            for (const auto& v : __internal_set_) {
                if (__bl(v)) {
                    return;
                }
            }
        }
        
        void for_all(std::function<void(const set_type&)> __bl)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            __bl(__internal_set_);
        }
    };
    
    
    template <
              typename _Value,
              typename _Hash = hash<_Value>,
              typename _Pred = equal_to<_Value>,
              typename _Alloc = allocator<_Value>
             >
    class threadsafe_unordered_multiset
    {
    public:
        typedef _Value                                      key_type;
        typedef key_type                                    value_type;
        typedef _Hash                                       hasher;
        typedef _Pred                                       key_equal;
        typedef _Alloc                                      allocator_type;
        typedef value_type&                                 reference;
        typedef const value_type&                           const_reference;
        
    private:
        typedef std::unordered_multiset<value_type, hasher, key_equal, allocator_type> __set_type;
        
        mutable std::shared_timed_mutex __mutex_;
        __set_type __internal_set_;
        
    public:
        typedef          __set_type                         set_type;
        typedef typename __set_type::pointer                pointer;
        typedef typename __set_type::const_pointer          const_pointer;
        typedef typename __set_type::size_type              size_type;
        typedef typename __set_type::difference_type        difference_type;
        typedef typename __set_type::iterator               iterator;
        typedef typename __set_type::const_iterator         const_iterator;
        typedef typename __set_type::local_iterator         local_iterator;
        typedef typename __set_type::const_local_iterator   const_local_iterator;
        
    public:
        threadsafe_unordered_multiset() : __internal_set_() {}
        threadsafe_unordered_multiset(const set_type& __s) : __internal_set_(__s) {}
        threadsafe_unordered_multiset(set_type&& __s) : __internal_set_(std::move(__s)) {}
        threadsafe_unordered_multiset(initializer_list<value_type> __il) : __internal_set_(__il) {}
        
        template <class _InputIterator>
        threadsafe_unordered_multiset(_InputIterator __f, _InputIterator __l) : __internal_set_(__f, __l) {}
        
        threadsafe_unordered_multiset(const threadsafe_unordered_multiset&) = delete;
        threadsafe_unordered_multiset& operator=(const threadsafe_unordered_multiset&) = delete;
        threadsafe_unordered_multiset(threadsafe_unordered_multiset&&) = delete;
        threadsafe_unordered_multiset& operator=(threadsafe_unordered_multiset&&) = delete;
        
    public:
        void swap(set_type& other)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            other.swap(__internal_set_);
        }
    
        bool empty() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.empty();
        }
        
        size_type size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.size();
        }
        
        size_type max_size() const
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.max_size();
        }
        
        void operator=(const set_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_ = __v;
        }
        
        void operator=(initializer_list<set_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_ = __il;
        }
        
        set_type value()
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_;
        }
        
        set_type set_intersection(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r;
            for (auto v : s)
            {
                auto it = __internal_set_.find(v);
                if (it != __internal_set_.end() && std::min(s.count(v), __internal_set_.count(v)) != r.count(v))
                {
                    r.insert(v);
                }
            }
            return r;
        }
        
        set_type set_union(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r(__internal_set_);
            for (auto v : s)
            {
                auto it = __internal_set_.find(v);
                if (it == __internal_set_.end())
                {
                    r.insert(v);
                }
                else
                {
                    if (std::max(s.count(v), __internal_set_.count(v)) != r.count(v))
                    {
                        r.insert(v);
                    }
                }
            }
            return r;
        }
        
        set_type set_different(const set_type& s)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            
            set_type r;
            for (auto v : __internal_set_)
            {
                auto it = s.find(v);
                if (it == s.end())
                {
                    r.insert(v);
                }
                else
                {
                    auto c = __internal_set_.count(v) - s.count(v);
                    if (c > 0 && c != r.count(v))
                    {
                        r.insert(v);
                    }
                }
            }
            return r;
        }
        
        template <class... _Args>
        void emplace(_Args&&... __args)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.emplace(std::forward<_Args>(__args)...);
        }
        
        void insert(const value_type& __v)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.insert(__v);
        }
        
        void insert(initializer_list<value_type> __il)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.insert(__il);
        }
        
        template <class _InputIterator>
        void insert(_InputIterator __f, _InputIterator __l)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.insert(__f, __l);
        }
        
        const std::pair<const value_type, bool> get(const key_type& __k)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            auto it = __internal_set_.find(__k);
            if (it == __internal_set_.end())
            {
                return std::make_pair(value_type(), false);
            }
            else
            {
                return std::make_pair(*it, true);
            }
        }
        
        void clear()
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            __internal_set_.clear();
        }
        
        bool contains(const key_type& __k)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            auto it = __internal_set_.find(__k);
            return it != __internal_set_.end();
        }
        
        size_type erase(const key_type& __k)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            return __internal_set_.erase(__k);
        }
        
        void for_each(std::function<void(const value_type&)> __bl)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            for (const auto& v : __internal_set_)
            {
                __bl(v);
            }
        }
        
        void for_each(const key_type& __k, std::function<void(const std::pair<iterator, iterator>&)> __bl)
        {
            std::shared_lock<std::shared_timed_mutex> lock(__mutex_);
            std::pair<iterator, iterator> r = __internal_set_.equal_range(__k);
            __bl(r);
        }
        
        void for_all(std::function<void(const set_type&)> __bl)
        {
            std::unique_lock<std::shared_timed_mutex> lock(__mutex_);
            
            __bl(__internal_set_);
        }
    };
}
