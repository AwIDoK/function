#ifndef FUNCTION_LIBRARY_H
#define FUNCTION_LIBRARY_H

#include <memory>
#include <cstddef>
#include <variant>
#include <array>

namespace my {
    template<typename>
    class function;

    template<typename Result, typename... Args>
    class function<Result (Args...)> {
        static const int SMALL_SIZE = 1;
        struct base_holder;
        typedef std::array<std::byte, SMALL_SIZE> smallT;
        typedef std::unique_ptr<base_holder> bigT;
    public:
        function() noexcept : invoker(nullptr) {}
        function(std::nullptr_t) noexcept : invoker(nullptr) {}
        function(function const& other) : invoker(nullptr) {
            if (std::holds_alternative<smallT>(other.invoker)) {
                if (std::holds_alternative<bigT>(invoker)) {
                    invoker = std::array<std::byte, SMALL_SIZE>();
                }
                reinterpret_cast<base_holder *>(std::get<smallT>(other.invoker).data())->in_place_copy(std::get<smallT>(invoker).data());
            } else {
                if (std::get<bigT>(other.invoker)) {
                    invoker = std::get<bigT>(other.invoker)->copy();
                } else {
                    invoker = nullptr;
                }
            }
        }

        function(function && other) noexcept {
            if (std::holds_alternative<bigT>(other.invoker)) {
                invoker.swap(other.invoker);
            } else {
                empty_small();
                if (std::holds_alternative<bigT>(invoker)) {
                    invoker = std::array<std::byte, SMALL_SIZE>();
                }
                reinterpret_cast<base_holder *>(std::get<smallT>(other.invoker).data())->in_place_copy(std::get<smallT>(invoker).data());
            }
        }

        template <typename Function>
        function(Function func) : invoker(nullptr) {
            if (sizeof(holder<Function>) <= sizeof(smallT)) {
                if (std::holds_alternative<bigT>(invoker)) {
                    invoker = std::array<std::byte, SMALL_SIZE>();
                }
                new(std::get<smallT>(invoker).data()) holder<Function> (std::move(func));
            } else {
                invoker = std::make_unique<holder<Function>>(std::move(func));
            }
        }

        ~function() {
            empty_small();
        }

        function& operator=(function const& other) {
            if (&other == this) {
                return *this;
            }
            function(other).swap(*this);
            return *this;
        }

        function& operator=(function && other) noexcept {
            if (&other == this) {
                return *this;
            }
            if (std::holds_alternative<bigT>(other.invoker)) {
                invoker.swap(other.invoker);
            } else {
                empty_small();
                if (std::holds_alternative<bigT>(invoker)) {
                    invoker = std::array<std::byte, SMALL_SIZE>();
                }
                ;
                reinterpret_cast<base_holder *>(std::get<smallT>(other.invoker).data())->in_place_move(std::get<smallT>(invoker).data());
            }
            return *this;
        }

        void swap(function & other) {
            if (&other != this) {
                function tmp(std::move(other));
                other = std::move(*this);
                *this = std::move(tmp);
            }
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
            virtual void in_place_move(std::byte* place) const = 0;
            void operator=(base_holder const &) = delete;
            base_holder(base_holder const &) = delete;
        };

        template <typename Function>
        struct holder : base_holder {
            holder(Function func) : base_holder(), _function(std::move(func)) {}
            std::unique_ptr<base_holder> copy() override {
                return std::make_unique<holder<Function>>(_function);
            }
            void in_place_copy(std::byte * const place) const override {
                new(place) holder<Function>(_function);
            }
            void in_place_move(std::byte * const place) const override {
                new(place) holder<Function>(std::move(_function));
            }

            Result run(Args&&... args) override {
                return _function(std::forward<Args>(args)...);
            }

            virtual ~holder() = default;

        private:
            Function _function;
        };
        mutable std::variant<bigT, smallT> invoker;
        void empty_small() {
            if (std::holds_alternative<smallT>(invoker)) {
                reinterpret_cast<base_holder *>(std::get<smallT>(invoker).data())->~base_holder();
            }
        }
    };
}

#endif