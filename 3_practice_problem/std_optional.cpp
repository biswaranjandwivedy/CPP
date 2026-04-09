Gemini said
Implementing std::optional is an excellent exercise in understanding manual memory management, object lifecycles, and placement new.

The core challenge of implementing std::optional<T> is that it cannot simply contain a member T value;. If it did, T would be default-constructed when the optional is empty, which is unacceptable because:

T might not have a default constructor.

Default constructing T might be computationally expensive.

It violates the semantic meaning of "empty" (an empty optional should not instantiate a T at all).

To solve this, we must use an uninitialized block of raw memory and construct the object in-place only when a value is provided.

Here is a robust, production-grade implementation of a custom Optional<T>:

C++
#include <iostream>
#include <utility>
#include <stdexcept>
#include <type_traits>

template <typename T>
class Optional {
private:
    // 1. Raw Storage:
    // We use a union to allocate the exact amount of memory needed for T,
    // with the correct alignment, but WITHOUT calling T's constructor.
    union {
        char dummy_; // Active when empty
        T value_;    // Active when holding a value
    };
    
    bool has_value_;

    // Helper to safely destroy the held value if it exists
    void destroy() noexcept {
        if (has_value_) {
            value_.~T();
            has_value_ = false;
        }
    }

public:
    // 2. Constructors

    // Default constructor (Empty)
    constexpr Optional() noexcept : dummy_('\0'), has_value_(false) {}

    // Construct from a value (Copy)
    Optional(const T& value) : has_value_(true) {
        // Placement new: Construct T in the pre-allocated memory of value_
        new (&value_) T(value);
    }

    // Construct from a value (Move)
    Optional(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) 
        : has_value_(true) {
        new (&value_) T(std::move(value));
    }

    // 3. Destructor
    ~Optional() {
        destroy();
    }

    // 4. Copy and Move Semantics (Rule of Five)

    // Copy Constructor
    Optional(const Optional& other) : has_value_(other.has_value_) {
        if (other.has_value_) {
            new (&value_) T(other.value_);
        } else {
            dummy_ = '\0';
        }
    }

    // Move Constructor
    Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>) 
        : has_value_(other.has_value_) {
        if (other.has_value_) {
            new (&value_) T(std::move(other.value_));
        } else {
            dummy_ = '\0';
        }
    }

    // Copy Assignment
    Optional& operator=(const Optional& other) {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                value_ = other.value_; // Both have values, just assign
            } else if (!has_value_ && other.has_value_) {
                new (&value_) T(other.value_); // We are empty, construct from other
                has_value_ = true;
            } else if (has_value_ && !other.has_value_) {
                destroy(); // Other is empty, destroy our value
            }
        }
        return *this;
    }

    // Move Assignment
    Optional& operator=(Optional&& other) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                value_ = std::move(other.value_);
            } else if (!has_value_ && other.has_value_) {
                new (&value_) T(std::move(other.value_));
                has_value_ = true;
            } else if (has_value_ && !other.has_value_) {
                destroy();
            }
        }
        return *this;
    }

    // 5. Observers

    constexpr bool has_value() const noexcept {
        return has_value_;
    }

    explicit constexpr operator bool() const noexcept {
        return has_value_;
    }

    // Safe access with bounds checking
    T& value() {
        if (!has_value_) throw std::bad_optional_access();
        return value_;
    }

    const T& value() const {
        if (!has_value_) throw std::bad_optional_access();
        return value_;
    }

    // Unsafe access (faster, UB if empty)
    T* operator->() noexcept { return &value_; }
    const T* operator->() const noexcept { return &value_; }

    T& operator*() noexcept { return value_; }
    const T& operator*() const noexcept { return value_; }

    // 6. Modifiers

    void reset() noexcept {
        destroy();
    }

    // Construct a value in-place, forwarding arguments directly to T's constructor
    template <typename... Args>
    T& emplace(Args&&... args) {
        destroy(); // Clean up existing value if any
        new (&value_) T(std::forward<Args>(args)...);
        has_value_ = true;
        return value_;
    }
};
Key Technical Details
The union Trick: By placing char dummy_; and T value_; inside an anonymous union, we tell the compiler to allocate enough bytes to hold the largest member (which is T), aligned to T's requirements. Critically, because it is a union, C++ will not automatically call the constructor of T.

Placement new: In the constructors and emplace, we use new (&value_) T(...). This is "placement new". It tells the compiler: "Do not allocate memory on the heap. Instead, take this existing memory address (&value_) and execute T's constructor right there."

Explicit Destructor Calls: Because we manually constructed T using placement new, the compiler will not automatically destroy it. When the Optional goes out of scope (or is reset), we must manually call value_.~T() to trigger T's destructor, preventing memory/resource leaks if T manages heap memory or file handles.

std::forward in emplace: The emplace method takes variadic templates (Args&&...) and perfectly forwards them to T's constructor. This is crucial for performance, as it avoids constructing a temporary T and moving it into the optional.
