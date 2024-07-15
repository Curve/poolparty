#pragma once

#include <queue>
#include <cstddef>
#include <functional>

#include <atomic>
#include <thread>

#include <concepts>
#include <type_traits>

#include <stop_token>
#include <condition_variable>

namespace poolparty
{
    template <typename T>
    concept task_like = requires() {
        requires std::is_move_constructible_v<T>;
        requires std::is_default_constructible_v<T>;
        requires std::constructible_from<T, std::function<void()>>;
    };

    template <typename T>
    struct interface;

    template <template <typename...> class Queue = std::queue, task_like Task = std::function<void()>, typename... Ts>
    class pool
    {
        using queue     = Queue<Task, Ts...>;
        using interface = interface<queue>;

      private:
        queue m_queue;
        std::mutex m_queue_mutex;

      private:
        std::mutex m_threads_mutex;
        std::vector<std::jthread> m_threads;

      private:
        std::atomic_bool m_pause;
        std::condition_variable m_cond;

      public:
        pool(std::size_t = std::thread::hardware_concurrency());

      public:
        ~pool();

      private:
        void worker(std::stop_token);

      public:
        void pause();
        void resume();

      public:
        template <typename... As>
        void emplace(As &&...);

      public:
        template <bool Forget = false, auto... TaskParams, typename Func, typename... As>
        [[nodiscard]] auto submit(Func &&, As &&...);

      public:
        std::stop_source add_thread();

      public:
        void cleanup();

      public:
        [[nodiscard]] bool paused() const;

      public:
        [[nodiscard]] std::size_t size();
        [[nodiscard]] std::size_t tasks();
    };
} // namespace poolparty

#include "pool.inl"
