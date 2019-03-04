#pragma once

#include <deque>
#include <cassert>
#include <mutex>
#include <condition_variable>
#include <chrono>

/////////////////////////////////////////////////
/**
* @brief �̰߳�ȫ����
*/
template<typename T>
class ThreadQueue
{
public:
    ThreadQueue() :_size(0) {};

public:

    typedef std::deque<T> queue_type;

    /**
    * @brief ��ͷ����ȡ����, û��������ȴ�.
    *
    * @param t
    * @param millsecond   �����ȴ�ʱ��(ms)
    *                    0 ��ʾ������
    *                      -1 ���õȴ�
    * @return bool: true, ��ȡ������, false, ������
    */
    bool pop_front(T& t, size_t millsecond = 0);

    /**
    * @brief �����ݵ����к��.
    *
    * @param t
    */
    void push_back(const T& t);

    /**
    * @brief  �����ݵ����к��.
    *
    * @param vt
    */
    void push_back(const queue_type &qt);

    /**
    * @brief  �����ݵ�����ǰ��.
    *
    * @param t
    */
    void push_front(const T& t);

    /**
    * @brief  �����ݵ�����ǰ��.
    *
    * @param vt
    */
    void push_front(const queue_type &qt);

    /**
    * @brief  �ȵ������ݲŽ���.
    *
    * @param q
    * @param millsecond  �����ȴ�ʱ��(ms)
    *                   0 ��ʾ������
    *                      -1 ���Ϊ�����õȴ�
    * @return �����ݷ���true, �����ݷ���false
    */
    bool swap(queue_type &q, size_t millsecond = 0);

    /**
    * @brief  ���д�С.
    *
    * @return size_t ���д�С
    */
    size_t size();

    /**
    * @brief  ��ն���
    */
    void clear();

    /**
    * @brief  �Ƿ�����Ϊ��.
    *
    * @return bool Ϊ�շ���true�����򷵻�false
    */
    bool empty();

protected:
    /**
    * @brief ֪ͨ�ȴ��ڶ���������̶߳��ѹ���
    */
    void notifyT();

    void notify();


protected:
    /**
    * ����
    */
    queue_type          _queue;

    /**
    * ���г���
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
            //��ʱ��
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
            //��ʱ��
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
