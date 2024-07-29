#pragma once

#include "pool.hpp"

#include <future>
#include <utility>

namespace poolparty
{
    template <typename T>
    struct traits
    {
        static bool empty(T &queue)
        {
            return queue.empty();
        };

        template <typename... Ts>
        static void emplace(T &queue, Ts &&...arguments)
        {
            queue.emplace(std::forward<Ts>(arguments)...);
        };

        static auto pop(T &queue)
        {
            auto rtn = std::move(queue.front());
            queue.pop();

            return rtn;
        };
    };

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    pool<Queue, Task, Ts...>::pool(std::size_t threads)
    {
        for (auto i = 0u; threads > i; i++)
        {
            add_thread();
        }
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    pool<Queue, Task, Ts...>::~pool()
    {
        std::unique_lock lock{m_mutex};

        for (auto &thread : m_threads)
        {
            thread.request_stop();
        }

        lock.unlock();
        m_cond.notify_all();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::worker(std::stop_token token)
    {
        auto pred = [this, &token]()
        {
            return token.stop_requested() || (!m_pause && !traits::empty(m_queue));
        };

        while (true)
        {
            std::unique_lock lock{m_mutex};
            m_cond.wait(lock, pred);

            if (token.stop_requested())
            {
                break;
            }

            auto task = traits::pop(m_queue);
            lock.unlock();

            std::invoke(task);
        }

        m_cond.notify_all();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::pause()
    {
        std::lock_guard lock{m_mutex};
        m_pause = true;
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::resume()
    {
        {
            std::lock_guard lock{m_mutex};
            m_pause = false;
        }
        m_cond.notify_all();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    template <typename... As>
    void pool<Queue, Task, Ts...>::emplace(As &&...arguments)
    {
        {
            std::lock_guard lock{m_mutex};
            traits::emplace(m_queue, std::forward<As>(arguments)...);
        }
        m_cond.notify_one();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    template <bool Forget, auto... TaskParams, typename Func, typename... As>
    auto pool<Queue, Task, Ts...>::submit(Func &&callback, As &&...arguments)
    {
        if constexpr (!Forget)
        {
            using result_t = std::invoke_result_t<Func, As...>;

            auto promise = std::promise<result_t>{};
            auto rtn     = promise.get_future();

            // MSVC has a broken `std::packaged_task` implementation. Thus we're doing things manually here.

            auto fn = [promise = std::move(promise), callback = std::forward<Func>(callback),
                       ... arguments = std::forward<As>(arguments)]() mutable
            {
                if constexpr (not std::is_void_v<result_t>)
                {
                    promise.set_value(std::invoke(callback, arguments...));
                }
                else
                {
                    std::invoke(callback, arguments...);
                    promise.set_value();
                }
            };

            emplace(std::move(fn), TaskParams...);

            return rtn;
        }
        else
        {
            auto task = [callback = std::forward<Func>(callback), ... arguments = std::forward<As>(arguments)]()
            {
                std::invoke(callback, arguments...);
            };

            emplace(std::move(task), TaskParams...);
        }
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    std::stop_source pool<Queue, Task, Ts...>::add_thread()
    {
        std::lock_guard lock{m_mutex};

        auto thread = [this](std::stop_token token)
        {
            worker(std::move(token));
        };

        return m_threads.emplace_back(thread).get_stop_source();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::cleanup()
    {
        std::unique_lock lock{m_mutex};
        std::vector<std::jthread> pending;

        for (auto it = m_threads.begin(); it != m_threads.end();)
        {
            if (!it->get_stop_token().stop_requested())
            {
                ++it;
                continue;
            }

            pending.emplace_back(std::move(*it));
            it = m_threads.erase(it);
        }

        lock.unlock();
        m_cond.notify_all();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    bool pool<Queue, Task, Ts...>::paused() const
    {
        std::lock_guard lock{m_mutex};
        return m_pause;
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    std::size_t pool<Queue, Task, Ts...>::size() const
    {
        std::lock_guard lock{m_mutex};
        return m_threads.size();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    std::size_t pool<Queue, Task, Ts...>::tasks() const
    {
        std::lock_guard lock{m_mutex};
        return m_queue.size();
    }
} // namespace poolparty
