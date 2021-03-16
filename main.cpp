#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <experimental/coroutine>
#include <iostream>

struct result {
    struct promise_type {
        void return_void() {}
        std::experimental::suspend_never initial_suspend() noexcept { return {}; }
        std::experimental::suspend_never final_suspend() noexcept { return {}; }
        result get_return_object() { return {}; }
        void unhandled_exception() { std::terminate(); }
    };
};

auto yield(boost::asio::any_io_executor executor) {
    struct [[nodiscard]] awaitable {
        boost::asio::any_io_executor executor;
        bool await_ready() { return false; }
        void await_suspend(std::experimental::coroutine_handle<> coro) {
            boost::asio::post(executor, [coro] () mutable { coro.resume(); });
        }
        void await_resume() {}
    };
    return awaitable{std::move(executor)};
}

result coro(boost::asio::any_io_executor executor) {
    std::cout << "Before yield" << std::endl;
    co_await yield(executor);
    std::cout << "After yield" << std::endl;
}

int main() {
    boost::asio::io_context context;
    coro(context.get_executor());
    std::cout << "Hello, World!" << std::endl;
    context.run();
    return 0;
}
