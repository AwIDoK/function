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
        function() noexcept : invoker(nullptr) {}
        function(std::nullptr_t) noexcept : invoker(nullptr) {}
        function(function const& other) : invoker(nullptr) {
            if (std::holds_alternative<smallT>(other.invoker)) {
                invoker = std::array<std::byte, 16>();
                reinterpret_cast<base_holder *>(std::get<smallT>(other.invoker).data())->in_place_copy(std::get<smallT>(invoker).data());
            } else {
                invoker = std::get<bigT>(other.invoker)->copy();
            }
        }

        function(function && other) noexcept {
            swap(other);
        }

        template <typename Function>
        function(Function func) : invoker(nullptr) {
            if (sizeof(holder<Function>) <= sizeof(smallT)) {
                invoker = std::array<std::byte, 16>();
                new(std::get<smallT>(invoker).data()) holder<Function> (std::move(func));
            } else {
                invoker = std::make_unique<holder<Function>>(std::move(func));
            }
        }

        ~function() {
            if (std::holds_alternative<smallT>(invoker)) {
                reinterpret_cast<base_holder *>(std::get<smallT>(invoker).data())->~base_holder();
            }
        }

        function& operator=(function const& other) {
            function(other).swap(*this);
            return *this;
        }

        function& operator=(function && other) noexcept {
            swap(other);
            return *this;
        }

        void swap(function & other) {
            std::swap(invoker, other.invoker);
        }

        explicit operator bool() const noexcept {
            return std::holds_alternative<smallT>(invoker) || std::get<bigT>(invoker);
        }

        Result operator()(Args&&... args) {
            if (std::holds_alternative<smallT>(invoker)) {
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
            virtual void in_place_copy(std::byte* place) const = 0;
            void operator=(base_holder const &) = delete;
            base_holder(base_holder const &) = delete;
        };

        template <typename Function>
        struct holder : base_holder {
            holder(Function func) : base_holder(), _function(func) {}
            std::unique_ptr<base_holder> copy() override {
                return std::make_unique<holder<Function>>(_function);
            }
            void in_place_copy(std::byte * const place) const override {
                new(place) holder<Function>(_function);
            }

            Result run(Args&&... args) override {
                return _function(std::forward<Args>(args)...);
            }

            virtual ~holder() = default;

        private:
            Function _function;
        };
        mutable std::variant<bigT, smallT> invoker;
    };
}

#endif