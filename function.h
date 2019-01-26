#ifndef FUNCTION_LIBRARY_H
#define FUNCTION_LIBRARY_H

#include <memory>
#include <cstddef>
#include <variant>
#include <array>
#include <cstring>

namespace my {
    template<typename>
    class function;

    template<typename Result, typename... Args>
    class function<Result (Args...)> {
        struct base_holder;
        typedef std::array<std::byte, 16> smallT;
        typedef std::unique_ptr<base_holder> bigT;
    public:
        function() noexcept : invoker(nullptr), small(false) {}
        function(std::nullptr_t) noexcept : invoker(nullptr), small(false) {}
        function(function const& other) : small(other.small) {
            if (small) {
                invoker = std::get<smallT>(other.invoker);
            } else {
                invoker = std::get<bigT>(other.invoker)->copy();
            }
        }

        function(function && other) noexcept {
            swap(other);
        }

        template <typename Function>
        function(Function func) {
            if (sizeof(holder<Function>) <= sizeof(smallT)) {
                small = true;
                new(std::get<smallT>(invoker).data()) holder<Function> (std::move(func));
            } else {
                small = false;
                invoker = std::make_unique<holder<Function>>(std::move(func));
            }
        }

        ~function() {
            if (small) {
                reinterpret_cast<base_holder *>(std::get<smallT>(invoker).data())->~base_holder();
            }
        }

        function& operator=(function const& other) {
            if (!small) {
                std::get<bigT>(invoker).release();
            }
            function(other).swap(*this);
            return *this;
        }

        function& operator=(function && other) noexcept {
            swap(other);
            return *this;
        }

        void swap(function & other) {
            std::swap(small, other.small);
            invoker.swap(other.invoker);
        }

        explicit operator bool() const noexcept {
            return small || std::get<bigT>(invoker);
        }

        Result operator()(Args&&... args) {
            if (small) {
                return reinterpret_cast<base_holder *>(std::get<smallT>(invoker).data())->run(std::forward<Args>(args)...);
            } else {
                return std::get<bigT>(invoker)->run(std::forward<Args>(args)...);
            }
        }


    private:
        struct base_holder {
            base_holder() = default;
            virtual ~base_holder() = default;
            virtual Result run(Args&&... args) = 0;
            virtual std::unique_ptr<base_holder> copy() = 0;
            void operator=(base_holder const &) = delete;
            base_holder(base_holder const &) = delete;
        };

        template <typename Function>
        struct holder : base_holder {
            holder(Function func) : base_holder(), _function(func) {}
            std::unique_ptr<base_holder> copy() override {
                return std::make_unique<holder<Function>>(_function);
            }

            Result run(Args&&... args) override {
                return _function(std::forward<Args>(args)...);
            }

            virtual ~holder() = default;

        private:
            Function _function;
        };
        mutable std::variant<smallT, bigT> invoker;
        bool small;
    };
}

#endif