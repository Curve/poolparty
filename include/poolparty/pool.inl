#pragma once

#include "pool.hpp"

#include <future>
#include <utility>

namespace poolparty
{
    template <typename T>
    struct interface
    {
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
        std::lock_guard guard{m_threads_mutex};

        for (auto &thread : m_threads)
        {
            thread.request_stop();
        }

        m_cond.notify_all();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::worker(std::stop_token token)
    {
        auto pred = [this, &token]()
        {
            return token.stop_requested() || (!m_pause && !m_queue.empty());
        };

        while (true)
        {
            Task task{};
            {
                std::unique_lock lock{m_queue_mutex};
                m_cond.wait(lock, pred);

                if (token.stop_requested())
                {
                    return;
                }

                task = std::move(interface::pop(m_queue));
            }
            std::invoke(task);
        }
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::pause()
    {
        m_pause.store(true);
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::resume()
    {
        m_pause.store(false);
        m_cond.notify_all();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    template <typename... As>
    void pool<Queue, Task, Ts...>::emplace(As &&...arguments)
    {
        {
            std::lock_guard lock{m_queue_mutex};
            interface::emplace(m_queue, std::forward<As>(arguments)...);
        }

        m_cond.notify_one();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    template <bool Forget, auto... TaskParams, typename Func, typename... As>
    auto pool<Queue, Task, Ts...>::submit(Func &&callback, As &&...arguments)
    {
        if constexpr (!Forget)
        {
            using task_t = decltype(std::packaged_task{std::forward<Func>(callback)});
            auto ptr     = std::make_shared<task_t>(std::forward<Func>(callback));

            auto task = [ptr, ... arguments = std::forward<As>(arguments)]()
            {
                std::invoke(*ptr, arguments...);
            };

            emplace(std::move(task), TaskParams...);

            return ptr->get_future();
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
        std::lock_guard guard{m_threads_mutex};

        auto thread = [this](std::stop_token token)
        {
            worker(std::move(token));
        };

        return m_threads.emplace_back(thread).get_stop_source();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    void pool<Queue, Task, Ts...>::cleanup()
    {
        m_cond.notify_all();

        std::lock_guard guard{m_threads_mutex};

        for (auto it = m_threads.begin(); it != m_threads.end();)
        {
            if (it->get_stop_token().stop_requested())
            {
                it = m_threads.erase(it);
                continue;
            }

            ++it;
        }
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    bool pool<Queue, Task, Ts...>::paused() const
    {
        return m_pause.load();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    std::size_t pool<Queue, Task, Ts...>::size()
    {
        std::lock_guard guard{m_threads_mutex};
        return m_threads.size();
    }

    template <template <typename...> class Queue, task_like Task, typename... Ts>
    std::size_t pool<Queue, Task, Ts...>::tasks()
    {
        std::lock_guard guard{m_queue_mutex};
        return m_queue.size();
    }
} // namespace poolparty
