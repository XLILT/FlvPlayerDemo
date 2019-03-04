#pragma once

#include <deque>
#include <cassert>
#include <mutex>
#include <condition_variable>
#include <chrono>

/////////////////////////////////////////////////
/**
* @brief 线程安全队列
*/
template<typename T>
class ThreadQueue
{
public:
    ThreadQueue() :_size(0) {};

public:

    typedef std::deque<T> queue_type;

    /**
    * @brief 从头部获取数据, 没有数据则等待.
    *
    * @param t
    * @param millsecond   阻塞等待时间(ms)
    *                    0 表示不阻塞
    *                      -1 永久等待
    * @return bool: true, 获取了数据, false, 无数据
    */
    bool pop_front(T& t, size_t millsecond = 0);

    /**
    * @brief 放数据到队列后端.
    *
    * @param t
    */
    void push_back(const T& t);

    /**
    * @brief  放数据到队列后端.
    *
    * @param vt
    */
    void push_back(const queue_type &qt);

    /**
    * @brief  放数据到队列前端.
    *
    * @param t
    */
    void push_front(const T& t);

    /**
    * @brief  放数据到队列前端.
    *
    * @param vt
    */
    void push_front(const queue_type &qt);

    /**
    * @brief  等到有数据才交换.
    *
    * @param q
    * @param millsecond  阻塞等待时间(ms)
    *                   0 表示不阻塞
    *                      -1 如果为则永久等待
    * @return 有数据返回true, 无数据返回false
    */
    bool swap(queue_type &q, size_t millsecond = 0);

    /**
    * @brief  队列大小.
    *
    * @return size_t 队列大小
    */
    size_t size();

    /**
    * @brief  清空队列
    */
    void clear();

    /**
    * @brief  是否数据为空.
    *
    * @return bool 为空返回true，否则返回false
    */
    bool empty();

protected:
    /**
    * @brief 通知等待在队列上面的线程都醒过来
    */
    void notifyT();

    void notify();


protected:
    /**
    * 队列
    */
    queue_type          _queue;

    /**
    * 队列长度
    */
    size_t              _size;

    std::mutex          _mutex;
    std::condition_variable _cond;
};

template<typename T>
bool ThreadQueue<T>::pop_front(T& t, size_t millsecond)
{   
    std::unique_lock<std::mutex> unqlck(_mutex);

    if (_queue.empty())
    {
        if (millsecond == 0)
        {
            return false;
        }

        if (millsecond == (size_t)-1)
        {
            // WAIT(unqlck, _cond);
            _cond.wait(unqlck);
        }
        else
        {
            //超时了
            /*bool got = WAIT_TIMEOUT(unqlck, _cond, millsecond,
                [this] {
                    return !empty();
                });*/
            
            bool got = _cond.wait_for(unqlck, std::chrono::milliseconds(millsecond), [this] {
                return !_queue.empty();
            });

            if (!got)
            {
                return false;
            }
        }
    }

    if (_queue.empty())
    {
        return false;
    }

    t = _queue.front();
    _queue.pop_front();
    assert(_size > 0);
    --_size;

    return true;
}

template<typename T>
void ThreadQueue<T>::notifyT()
{    
    std::unique_lock<std::mutex> unqlck(_mutex);

    _cond.notify_all();
}

template<typename T>
void ThreadQueue<T>::push_back(const T& t)
{
    std::unique_lock<std::mutex> unqlck(_mutex);
    
    _cond.notify_one();

    _queue.push_back(t);
    ++_size;
}

template<typename T>
void ThreadQueue<T>::push_back(const queue_type &qt)
{
    std::unique_lock<std::mutex> unqlck(_mutex);

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while (it != itEnd)
    {
        _queue.push_back(*it);
        ++it;
        ++_size;

        _cond.notify_one();
    }
}

template<typename T>
void ThreadQueue<T>::push_front(const T& t)
{    
    std::unique_lock<std::mutex> unqlck(_mutex);

    _cond.notify_one();

    _queue.push_front(t);

    ++_size;
}

template<typename T>
void ThreadQueue<T>::push_front(const queue_type &qt)
{
    std::unique_lock<std::mutex> unqlck(_mutex);

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while (it != itEnd)
    {
        _queue.push_front(*it);
        ++it;
        ++_size;

        _cond.notify_one();
    }
}

template<typename T>
bool ThreadQueue<T>::swap(queue_type &q, size_t millsecond)
{
    std::unique_lock<std::mutex> unqlck(_mutex);

    if (_queue.empty())
    {
        if (millsecond == 0)
        {
            return false;
        }

        if (millsecond == (size_t)-1)
        {
            // WAIT(unqlck, _cond);
            _cond.wait(unqlck);
        }
        else
        {
            //超时了
            /*bool got = WAIT_TIMEOUT(unqlck, _cond, millsecond,
            [this] {
            return !empty();
            });*/

            bool got = _cond.wait_for(unqlck, std::chrono::milliseconds(millsecond), [this] {
                return !_queue.empty();
            });

            if (!got)
            {
                return false;
            }
        }
    }

    if (_queue.empty())
    {
        return false;
    }

    q.swap(_queue);
    //_size = q.size();
    _size = _queue.size();

    return true;
}

template<typename T>
size_t ThreadQueue<T>::size()
{
    std::unique_lock<std::mutex> unqlck(_mutex);
    //return _queue.size();

    return _size;
}

template<typename T>
void ThreadQueue<T>::clear()
{
    std::unique_lock<std::mutex> unqlck(_mutex);

    _queue.clear();
    _size = 0;
}

template<typename T>
bool ThreadQueue<T>::empty()
{
    std::unique_lock<std::mutex> unqlck(_mutex);

    return _queue.empty();
}
