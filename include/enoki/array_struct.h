#pragma once

NAMESPACE_BEGIN(enoki)

// -----------------------------------------------------------------------
//! @{ \name Adapter and routing functions for dynamic data structures
// -----------------------------------------------------------------------

template <typename T, typename>
struct struct_support {
    static constexpr bool IsDynamic = false;
    using Dynamic = T;

    static ENOKI_INLINE size_t slices(const T &) { return 1; }
    static ENOKI_INLINE size_t packets(const T &) { return 1; }
    static ENOKI_INLINE void set_slices(const T &, size_t) { }

    template <typename T2> static ENOKI_INLINE decltype(auto) slice(T2&& value, size_t) { return value; }
    template <typename T2> static ENOKI_INLINE decltype(auto) slice_ptr(T2&& value, size_t) { return &value; }
    template <typename T2> static ENOKI_INLINE decltype(auto) packet(T2&& value, size_t) { return value; }
    template <typename T2> static ENOKI_INLINE decltype(auto) ref_wrap(T2&& value) { return value; }

    template <typename Mem>
    static ENOKI_INLINE size_t compress(Mem &mem, const T &value, bool mask) {
        size_t count = mask ? 1 : 0;
        *mem = value;
        mem += count;
        return count;
    }

    static ENOKI_INLINE T zero(size_t) { return T(0); }
    static ENOKI_INLINE T empty(size_t) { T x; return x; }

    static ENOKI_INLINE detail::MaskedValue<T> masked(T &value, bool mask) {
        return detail::MaskedValue<T>{ value, mask };
    }
};

template <>
struct struct_support<void, int> { using Dynamic = void; };

template <typename T> ENOKI_INLINE T zero(size_t size) {
    return struct_support_t<T>::zero(size);
}

template <typename T> ENOKI_INLINE T empty(size_t size) {
    return struct_support_t<T>::empty(size);
}

template <typename T> ENOKI_INLINE size_t packets(const T &value) {
    return struct_support_t<T>::packets(value);
}

template <typename T> ENOKI_INLINE size_t slices(const T &value) {
    return struct_support_t<T>::slices(value);
}

template <typename T> ENOKI_NOINLINE void set_slices(T &value, size_t size) {
    struct_support_t<T>::set_slices(value, size);
}

template <typename T> ENOKI_INLINE decltype(auto) packet(T &value, size_t i) {
    return struct_support_t<T>::packet(value, i);
}

template <typename T> ENOKI_INLINE decltype(auto) slice(T &value, size_t i) {
    return struct_support_t<T>::slice(value, i);
}

template <typename T> ENOKI_INLINE decltype(auto) slice_ptr(T &value, size_t i) {
    return struct_support_t<T>::slice_ptr(value, i);
}

template <typename T> ENOKI_INLINE decltype(auto) ref_wrap(T &value) {
    return struct_support_t<T>::ref_wrap(value);
}

template <typename Mem, typename Value, typename Mask>
ENOKI_INLINE size_t compress(Mem &mem, const Value &value, const Mask& mask) {
    return struct_support_t<Value>::compress(mem, value, mask);
}

template <typename T> using is_dynamic = std::bool_constant<struct_support_t<T>::IsDynamic>;
template <typename T> constexpr bool is_dynamic_v = is_dynamic<T>::value;
template <typename T> using enable_if_dynamic_t = enable_if_t<is_dynamic_v<T>>;
template <typename T> using enable_if_static_t = enable_if_t<!is_dynamic_v<T>>;

template <typename T>
using make_dynamic_t = typename struct_support_t<T>::Dynamic;

template <typename T>
struct struct_support<T, enable_if_static_array_t<T>> {
    static constexpr bool IsDynamic = is_dynamic_v<value_t<T>>;
    static constexpr size_t Size = T::Size;

    using Dynamic = std::conditional_t<
        array_depth_v<T> == 1, DynamicArray<std::decay_t<T>>,
        typename T::template ReplaceValue<make_dynamic_t<value_t<T>>>>;

    static ENOKI_INLINE size_t slices(const T &value) { return enoki::slices(value.x()); }
    static ENOKI_INLINE size_t packets(const T& value) { return enoki::packets(value.x()); }

    static ENOKI_INLINE void set_slices(T &value, size_t size) {
        for (size_t i = 0; i < Size; ++i)
            enoki::set_slices(value.coeff(i), size);
    }


    static ENOKI_INLINE T zero(size_t size) {
        ENOKI_MARK_USED(size);
        if constexpr (array_depth_v<T> == 1) {
            return T::zero_();
        } else {
            T result;
            for (size_t i = 0; i < Size; ++i)
                result.coeff(i) = enoki::zero<value_t<T>>(size);
            return result;
        }
    }

    static ENOKI_INLINE T empty(size_t size) {
        ENOKI_MARK_USED(size);
        if constexpr (array_depth_v<T> == 1) {
            return T::empty_();
        } else {
            T result;
            for (size_t i = 0; i < Size; ++i)
                result.coeff(i) = enoki::empty<value_t<T>>(size);
            return result;
        }
    }

    static ENOKI_INLINE auto masked(T &value, const mask_t<T> &mask) {
        return detail::MaskedArray<T>{ value, mask };
    }

    template <typename T2>
    static ENOKI_INLINE decltype(auto) packet(T2 &value, size_t i) {
        ENOKI_MARK_USED(i);
        if constexpr (array_depth_v<T> == 1)
            return value;
        else
            return packet(value, i, std::make_index_sequence<Size>());
    }

    template <typename T2>
    static ENOKI_INLINE decltype(auto) slice(T2 &value, size_t i) {
        if constexpr (array_depth_v<T> == 1)
            return value.coeff(i);
        else
            return slice(value, i, std::make_index_sequence<Size>());
    }

    template <typename T2>
    static ENOKI_INLINE decltype(auto) slice_ptr(T2 &value, size_t i) {
        if constexpr (array_depth_v<T> == 1)
            return value.data() + i;
        else
            return slice_ptr(value, i, std::make_index_sequence<Size>());
    }

    template <typename T2>
    static ENOKI_INLINE decltype(auto) ref_wrap(T2 &value) {
        if constexpr (array_depth_v<T> == 1)
            return value;
        else
            return ref_wrap(value, std::make_index_sequence<Size>());
    }

    template <typename Mem>
    static ENOKI_INLINE size_t compress(Mem &mem, const expr_t<T>& value, const mask_t<expr_t<T>> &mask) {
        if constexpr (is_array_v<Mem>) {
            size_t result = 0;
            for (size_t i = 0; i < Size; ++i)
                result = enoki::compress(mem.coeff(i), value.coeff(i), mask.coeff(i));
            return result;
        } else {
            return value.compress_(mem, mask);
        }
    }

private:
    template <typename T2, size_t... Is>
    static ENOKI_INLINE decltype(auto) packet(T2 &value, size_t i, std::index_sequence<Is...>) {
        using Value = decltype(enoki::packet(value.coeff(0), i));
        using Return = typename T::template ReplaceValue<Value>;
        return Return(enoki::packet(value.coeff(Is), i)...);
    }

    template <typename T2, size_t... Is>
    static ENOKI_INLINE decltype(auto) slice(T2 &value, size_t i, std::index_sequence<Is...>) {
        using Value = decltype(enoki::slice(value.coeff(0), i));
        using Return = typename T::template ReplaceValue<Value>;
        return Return(enoki::slice(value.coeff(Is), i)...);
    }

    template <typename T2, size_t... Is>
    static ENOKI_INLINE decltype(auto) slice_ptr(T2 &value, size_t i, std::index_sequence<Is...>) {
        using Value = decltype(enoki::slice_ptr(value.coeff(0), i));
        using Return = typename T::template ReplaceValue<Value>;
        return Return(enoki::slice_ptr(value.coeff(Is), i)...);
    }

    template <typename T2, size_t... Is>
    static ENOKI_INLINE decltype(auto) ref_wrap(T2 &value, std::index_sequence<Is...>) {
        using Value = decltype(enoki::ref_wrap(value.coeff(0)));
        using Return = typename T::template ReplaceValue<Value>;
        return Return(enoki::ref_wrap(value.coeff(Is))...);
    }
};

template <typename T>
struct struct_support<T, enable_if_dynamic_array_t<T>> {
    static constexpr bool IsDynamic = true;
    using Dynamic = T;

    static ENOKI_INLINE T zero(size_t size) { return T::zero_(size); }
    static ENOKI_INLINE T empty(size_t size) { return T::empty_(size); }

    static ENOKI_INLINE auto masked(T &value, const mask_t<T> &mask) {
        return detail::MaskedArray<T>{ value, mask };
    }

    static ENOKI_INLINE size_t packets(const T &value) { return value.packets(); }
    static ENOKI_INLINE size_t slices(const T &value) { return value.size(); }
    static ENOKI_INLINE void set_slices(T &value, size_t size) { value.resize(size); }
    static ENOKI_INLINE decltype(auto) packet(const T &value, size_t i) { return value.packet(i); }
    static ENOKI_INLINE decltype(auto) packet(T &value, size_t i) { return value.packet(i); }
    static ENOKI_INLINE decltype(auto) slice(const T &value, size_t i) { return value.coeff(i); }
    static ENOKI_INLINE decltype(auto) slice(T &value, size_t i) { return value.coeff(i); }
    static ENOKI_INLINE decltype(auto) slice_ptr(const T &value, size_t i) { return value.data() + i; }
    static ENOKI_INLINE decltype(auto) slice_ptr(T &value, size_t i) { return value.data() + i; }
    static ENOKI_INLINE auto ref_wrap(T &value) { return value.ref_wrap_(); }
    static ENOKI_INLINE auto ref_wrap(const T &value) { return value.ref_wrap_(); }

    template <typename Mem>
    static ENOKI_INLINE size_t compress(Mem &mem, const T& value, const mask_t<T> &mask) {
        return value.compress_(mem, mask);
    }
};

namespace detail {
    /// Recursive helper function used by enoki::shape
    template <typename T>
    void extract_shape_recursive(size_t *out, size_t i, const T &array) {
        if constexpr (is_array_v<T>) {
            *out = array.derived().size();
            if constexpr (is_array_v<value_t<T>>)
                extract_shape_recursive(out + 1, i + 1, array.derived().coeff(0));
        }
    }

    template <typename T>
    bool is_ragged_recursive(const T &a, const size_t *shape) {
        if constexpr(is_array_v<T>) {
            size_t size = a.derived().size();
            if (*shape != size)
                return false;
            bool match = true;
            if (is_dynamic_v<value_t<T>>) {
                for (size_t i = 0; i < size; ++i)
                    match &= is_ragged_recursive(a.derived().coeff(i), shape + 1);
            } else {
                is_ragged_recursive(a.derived().coeff(0), shape + 1);
            }
            return match;
        } else {
            return true;
        }
    }

    template <typename T>
    ENOKI_INLINE void set_shape_recursive(T &a, const size_t *shape) {
        if constexpr(is_array_v<T>) {
            size_t size = a.derived().size();
            a.resize(*shape);
            if (is_dynamic_v<value_t<T>>) {
                for (size_t i = 0; i < size; ++i)
                    set_shape_recursive(a.derived().coeff(i), shape + 1);
            } else {
                set_shape_recursive(a.derived().coeff(0), shape + 1);
            }
        }
    }
};

/// Extract the shape of a nested array as an std::array
template <typename T, typename Result = std::array<size_t, array_depth_v<T>>>
Result shape(const T &array) {
    Result result;
    detail::extract_shape_recursive(result.data(), 0, array);
    return result;
}

template <typename T>
void set_shape(T &a, const std::array<size_t, array_depth_v<T>> &value) {
    detail::set_shape_recursive(a, value.data());
}

template <typename T> bool ragged(const T &a) {
    return !detail::is_ragged_recursive(a, shape(a).data());
}

//! @}
// -----------------------------------------------------------------------

NAMESPACE_END(enoki)