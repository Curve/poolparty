#include <boost/ut.hpp>

#include <poolparty/pool.hpp>

#include <vector>
#include <algorithm>

using namespace boost::ut;

struct priority_task
{
    std::move_only_function<void()> callback;
    std::size_t priority;

  public:
    auto operator()()
    {
        return std::invoke(callback);
    }

    bool operator<(const priority_task &other) const
    {
        return priority < other.priority;
    }
};

// `std::priority_queue` does not play nice with move-only elements.
// This is a problem as the internally used task captures move-only objects.

template <typename... Ts>
struct poolparty::traits<std::vector<Ts...>>
{
    using queue_t = std::vector<Ts...>;

    static bool empty(queue_t &queue)
    {
        return queue.empty();
    };

    template <typename... As>
    static void emplace(queue_t &queue, As &&...arguments)
    {
        queue.emplace_back(std::forward<As>(arguments)...);
        std::push_heap(queue.begin(), queue.end());
    };

    static auto pop(queue_t &queue)
    {
        std::pop_heap(queue.begin(), queue.end());

        auto rtn = std::move(queue.back());
        queue.pop_back();

        return rtn;
    };
};

// NOLINTNEXTLINE
suite<"priority"> priority_test = []()
{
    poolparty::pool<std::vector, priority_task> pool{1};

    expect(eq(pool.size(), 1));
    expect(eq(pool.tasks(), 0));

    pool.pause();

    std::vector<int> order;

    auto fut0 = pool.submit<false, 1>([&]() { order.emplace_back(1); });
    auto fut1 = pool.submit<false, 2>([&]() { order.emplace_back(2); });
    auto fut2 = pool.submit<false, 3>([&]() { order.emplace_back(3); });
    auto fut5 = pool.submit<false, 6>([&]() { order.emplace_back(6); });
    auto fut3 = pool.submit<false, 4>([&]() { order.emplace_back(4); });
    auto fut4 = pool.submit<false, 5>([&]() { order.emplace_back(5); });

    pool.resume();

    fut0.wait();
    fut1.wait();
    fut2.wait();
    fut3.wait();
    fut4.wait();
    fut5.wait();

    expect(eq(order[0], 6));
    expect(eq(order[1], 5));
    expect(eq(order[2], 4));
    expect(eq(order[3], 3));
    expect(eq(order[4], 2));
    expect(eq(order[5], 1));

    expect(eq(pool.tasks(), 0));
};
