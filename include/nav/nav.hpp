#pragma once
#include <array>
#include <optional>
#include <string_view>

#define PARENS ()
#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__)))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define MAP_OPERATOR(func, op, ...)                                            \
    __VA_OPT__(EXPAND(MAP_OPERATOR_HELPER(func, op, __VA_ARGS__)))
#define MAP_OPERATOR_HELPER(func, op, a1, ...)                                 \
    func(op a1), __VA_OPT__(MAP_OPERATOR_AGAIN PARENS(func, op, __VA_ARGS__))
#define MAP_OPERATOR_AGAIN() MAP_OPERATOR_HELPER


namespace nav::impl {
template <size_t N, class T>
constexpr T max_elem(T const* ptr) {
    if constexpr (N == 0) {
        return T {};
    } else {
        T max = ptr[0];
        for (size_t i = 1; i < N; i++) {
            if (ptr[i] > max) {
                max = ptr[i];
            }
        }
        return max;
    }
}
template <class T>
constexpr auto compare(T a, T b) {
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}
constexpr auto compare(std::string_view a, std::string_view b) {
    return a.compare(b);
}
template <class T>
constexpr void swap(T& a, T& b) {
    auto Tmp = static_cast<T&&>(a);
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(Tmp);
}
constexpr auto to_lower = [](char ch) -> char {
    if ('A' <= ch && ch <= 'Z') {
        return ch - 'A' + 'a';
    } else {
        return ch;
    }
};
template <class Key, class Value, class BaseT, BaseT Min, BaseT Max>
class indexed_map {
    constexpr static size_t ArraySize = Max - Min + 1;
    std::array<Value, ArraySize> vals {};
    std::array<bool, ArraySize> present {};

    Value default_value;

   public:
    template <size_t N>
    constexpr indexed_map(
        std::array<Key, N> const& keys,
        std::array<Value, N> const& values,
        Value default_value)
      : vals()
      , present()
      , default_value(default_value) {
        for (auto& val : vals) {
            val = default_value;
        }
        for (size_t i = 0; i < N; i++) {
            auto key_i = BaseT(keys[i]) - Min;
            if (!present[key_i]) {
                vals[key_i] = values[i];
                present[key_i] = true;
            }
        }
    }
    constexpr auto contains(Key key) const -> bool {
        auto i = BaseT(key);
        if (i < Min || i > Max) {
            return false;
        } else {
            return present[i - Min];
        }
    }
    constexpr auto get(Key key) const -> std::optional<Value> {
        return (*this)[key];
    }
    constexpr auto get(Key key, Value default_value) const -> Value {
        if (std::optional<Value> result = (*this)[key]) {
            return *result;
        } else {
            return default_value;
        }
    }
    constexpr auto operator[](Key key) const -> std::optional<Value> {
        auto i = BaseT(key);
        if (i < Min || i > Max || !present[i - Min]) {
            return std::nullopt;
        } else {
            return vals[i - Min];
        }
    }
};

// Stably sorts and de-duplicates an array, returning a new number of elements
// stably sorted with duplicates removed
template <size_t N, class T, class Cmp>
constexpr void sort(T* values, Cmp less) {
    if constexpr (N <= 1)
        return;
    else if constexpr (N == 2) {
        auto &a = values[0], b = values[1];
        if (less(b, a)) {
            impl::swap(a, b);
        }
        return;
    } else {
        sort<N / 2>(values, less);
        sort<N - N / 2>(values + N / 2, less);
        size_t first_half = N / 2;
        size_t second_half = N - N / 2;

        T buffer[N] {};

        T* a1 = values;
        T* a2 = values + N / 2;
        size_t i1 = 0, i2 = 0, dest = 0;
        for (;;) {
            // If either array is out of elements, copy the remaining elements
            // and then break
            if (i1 == first_half) {
                while (i2 < second_half) {
                    buffer[dest++] = a2[i2++];
                }
                break;
            } else if (i2 == second_half) {
                while (i1 < first_half) {
                    buffer[dest++] = a1[i1++];
                }
                break;
            } else {
                if (less(a2[i1], a1[i2])) {
                    buffer[dest++] = a2[i1++];
                } else {
                    buffer[dest++] = a1[i2++];
                }
            }
        }
        // Copy the elements back from buffer to the original array
        for (size_t i = 0; i < dest; i++) {
            values[i] = buffer[i];
        }
    }
}
// Stably sorts and de-duplicates an array, returning a new number of elements
// stably sorted with duplicates removed
template <size_t N, class T, class Cmp>
constexpr size_t sort_dedup(T* values, Cmp less) {
    if constexpr (N <= 1)
        return N;

    if constexpr (N == 2) {
        auto &a = values[0], b = values[1];
        if (less(a, b)) {
            return N;
        } else if (less(b, a)) {
            impl::swap(a, b);
            return N;
        } else {
            // We return 1 b/c we're discarding B
            return 1;
        }
    }

    size_t first_half = sort_dedup<N / 2>(values, less);
    size_t second_half = sort_dedup<N - N / 2>(values + N / 2, less);

    T buffer[N];

    T* a1 = values;
    T* a2 = values + N / 2;
    size_t i1 = 0, i2 = 0, dest = 0;
    for (;;) {
        // If either array is out of elements, copy the remaining elements and
        // then break
        if (i1 == first_half) {
            while (i2 < second_half) {
                buffer[dest++] = a2[i2++];
            }
            break;
        } else if (i2 == second_half) {
            while (i1 < first_half) {
                buffer[dest++] = a1[i1++];
            }
            break;
        } else {
            if (less(a1[i1], a2[i2])) {
                buffer[dest++] = a1[i1++];
            } else if (less(a2[i2], a1[i1])) {
                buffer[dest++] = a2[i2++];
            } else {
                // Discard the element from a2, since that's the duplicate
                buffer[dest++] = a1[i1++];
                i2++;
            }
        }
    }
    // Copy the elements back from buffer to the original array
    for (size_t i = 0; i < dest; i++) {
        values[i] = buffer[i];
    }
    // Return the number of elements we placed in the resulting array
    return dest;
}
template <class Key, class Value, size_t N>
class binary_dedup_map {
    struct Entry {
        Key key {};
        Value value {};
    };
    Value default_value {};
    std::array<Entry, N> entries;
    size_t count = 0;
    // count <= N (It's the count with duplicates removed)


   public:
    constexpr binary_dedup_map(
        std::array<Key, N> const& keys,
        std::array<Value, N> const& values,
        Value const& default_value)
      : default_value(default_value) {
        for (size_t i = 0; i < N; i++) {
            entries[i] = {keys[i], values[i]};
        }
        // Stably sort and deduplicate entries. Compute count to be the number
        // of entries after deduplication
        count = sort_dedup<N>(
            entries.data(),
            [](auto const& e1, auto const& e2) { return e1.key < e2.key; });
    }
    constexpr bool contains(Key key) const {
        size_t min = 0, max = count, i = count / 2;
        while (max - min > 1) {
            auto entry_key = entries[i];
            auto cmp = impl::compare(key, entries[i].key);
            if (cmp == 0) {
                return true;
            }
            if (cmp < 0) {
                max = i;
            } else {
                min = i;
            }
            i = (max + min) / 2;
        }
        return entries[i].key == key;
    }
    constexpr auto get(Key key) const -> std::optional<Value> {
        return (*this)[key];
    }
    constexpr auto get(Key key, Value default_value) const -> Value {
        if (std::optional<Value> result = (*this)[key]) {
            return *result;
        } else {
            return default_value;
        }
    }
    constexpr auto operator[](Key key) const -> std::optional<Value> {
        size_t min = 0, max = count, i = count / 2;
        while (max - min > 1) {
            auto entry_key = entries[i];
            auto cmp = impl::compare(key, entries[i].key);
            if (cmp == 0) {
                return entries[i].value;
            }
            if (cmp < 0) {
                max = i;
            } else {
                min = i;
            }
            i = (max + min) / 2;
        }
        return entries[i].key == key ? std::optional<Value> {entries[i].value}
                                     : std::nullopt;
    }
};

// A map that returns an optional when you index into it. Returns nullopt if the
// item wasn't found.
template <class Key, class Value, size_t N>
class binary_map {
    struct Entry {
        Key key {};
        Value value {};
    };
    std::array<Entry, N> entries;
    // count <= N (It's the count with duplicates removed)


   public:
    constexpr binary_map(
        std::array<Key, N> const& keys,
        std::array<Value, N> const& values) {
        for (size_t i = 0; i < N; i++) {
            entries[i] = {keys[i], values[i]};
        }
        // Stably sort and deduplicate entries. Compute count to be the number
        // of entries after deduplication
        impl::sort<N>(entries.data(), [](auto const& e1, auto const& e2) {
            return e1.key < e2.key;
        });
    }
    constexpr bool contains(Key key) const {
        size_t min = 0, max = N, i = N / 2;
        while (max - min > 1) {
            auto entry_key = entries[i];
            auto cmp = impl::compare(key, entries[i].key);
            if (cmp == 0) {
                return true;
            }
            if (cmp < 0) {
                max = i;
            } else {
                min = i;
            }
            i = (max + min) / 2;
        }
        return entries[i].key == key;
    }
    constexpr auto get(Key key) const -> std::optional<Value> {
        return (*this)[key];
    }
    constexpr auto get(Key key, Value default_value) const -> Value {
        if (std::optional<Value> result = (*this)[key]) {
            return *result;
        } else {
            return default_value;
        }
    }
    constexpr auto operator[](Key key) const -> std::optional<Value> {
        size_t min = 0, max = N, i = N / 2;
        while (max - min > 1) {
            auto entry_key = entries[i];
            auto cmp = impl::compare(key, entries[i].key);
            if (cmp == 0) {
                return entries[i].value;
            }
            if (cmp < 0) {
                max = i;
            } else {
                min = i;
            }
            i = (max + min) / 2;
        }
        return entries[i].key == key ? std::optional<Value> {entries[i].value}
                                     : std::nullopt;
    }
};

template <class Key, class Value, size_t N, class BaseT, BaseT Min, BaseT Max>
constexpr auto select_map(
    std::array<Key, N> const& keys,
    std::array<Value, N> const& values,
    Value const& default_value) {
    // If we have a large sparse map, select binary_dedup_map. Otherwise, select
    // the indexed map, which should be faster, but may use more memory
    if constexpr (Max - Min > 2 * N + 256) {
        return binary_dedup_map<Key, Value, N>(keys, values, default_value);
    } else {
        return indexed_map<Key, Value, BaseT, Min, Max>(
            keys,
            values,
            default_value);
    }
}

template <class BaseT, class T, size_t N>
constexpr auto min_base_value(std::array<T, N> const& arr) -> BaseT {
    if constexpr (N == 0) {
        return BaseT();
    } else {
        auto min_value = BaseT(arr[0]);
        for (size_t i = 1; i < N; i++) {
            min_value = std::min(min_value, BaseT(arr[i]));
        }
        return min_value;
    }
}
template <class BaseT, class T, size_t N>
constexpr auto max_base_value(std::array<T, N> const& arr) -> BaseT {
    if constexpr (N == 0) {
        return BaseT();
    } else {
        auto max_value = BaseT(arr[0]);
        for (size_t i = 1; i < N; i++) {
            max_value = std::max(max_value, BaseT(arr[i]));
        }
        return max_value;
    }
}
template <class EnumType>
struct EnumAssignmentGuard {
    EnumType value;

    template <class T>
    constexpr auto operator=(T&&) const {
        return EnumAssignmentGuard {value};
    }
    constexpr operator EnumType() const {
        return value;
    }
};
template <class T, class... Args>
constexpr auto make_array(Args&&... args) -> std::array<T, sizeof...(args)> {
    return {static_cast<Args&&>(args)...};
}
template <size_t BuffSize, size_t N>
constexpr auto static_cat(
    std::array<std::string_view, N> const& views,
    char sep) -> std::array<char, BuffSize> {
    std::array<char, BuffSize> arr {};
    size_t i = 0;
    for (auto view : views) {
        for (char ch : view) {
            arr[i++] = ch;
        }
        arr[i++] = sep;
    }
    return arr;
}
template <size_t N>
constexpr auto split_by_lengths_assuming_sep(
    std::array<size_t, N> const& lengths,
    char const* data) -> std::array<std::string_view, N> {
    std::array<std::string_view, N> segments;
    size_t offset = 0;
    for (size_t i = 0; i < N; i++) {
        segments[i] = std::string_view(data + offset, lengths[i]);
        offset += lengths[i] + 1;
    }
    return segments;
}

constexpr std::string_view trim_whitespace(std::string_view view) {
    size_t start = view.find_first_not_of(' ');
    if (start == std::string_view::npos) {
        return "";
    } else {
        size_t end = view.find_first_of(" =", start);
        if (end == std::string_view::npos) {
            return view.substr(start);
        } else {
            return view.substr(start, end - start);
        }
    }
}
template <size_t N, size_t StrSize>
constexpr auto split_trim(char const (&str)[StrSize])
    -> std::array<std::string_view, N> {
    std::string_view view(str, StrSize - 1);
    std::array<std::string_view, N> args;

    size_t start_pos = 0;
    for (size_t i = 0; i < N; i++) {
        auto new_pos = view.find(',');
        args[i] = trim_whitespace(view.substr(0, new_pos));
        if (new_pos != std::string_view::npos) {
            view = view.substr(new_pos + 1);
        } else {
            break;
        }
    }
    return args;
}
template <size_t N>
constexpr auto get_top_name(char const (&str)[N]) {
    std::string_view view(str, N - 1);
    auto pos = view.find_last_of(':');
    if (pos == view.npos) {
        return view;
    } else {
        return view.substr(pos + 1);
    }
}

template <
    class T,
    size_t N,
    class F,
    class Ret = decltype(std::declval<F>()(std::declval<T>()))>
constexpr auto map_array(std::array<T, N> const& arr, F func)
    -> std::array<Ret, N> {
    std::array<Ret, N> result {};
    for (size_t i = 0; i < N; i++) {
        result[i] = func(arr[i]);
    }
    return result;
}
template <class Elem, class Result, class F>
constexpr Result fold(Elem const* values, size_t N, Result initial, F func) {
    for (size_t i = 0; i < N; i++) {
        initial = func(initial, values[i]);
    }
    return initial;
}
template <class Enum>
struct traits_impl {};

/**
 * @brief Compute the smallest number of the form 2^N-1 such that 2^N-1 >= i
 *
 * @param i
 * @return constexpr size_t
 */
constexpr size_t bit_ceil_minus_1(size_t i) {
    size_t ceil = 0;
    while (ceil < i) {
        ceil <<= 1;
        ceil |= 1;
    }
    return ceil;
}
/**
 * @brief Compute the Levenshtein distance through MaxLength (this should be
 * used when one of the strings has a known max length, and so there's no need
 * to compare further than that. It also ensures that no memory allocations need
 * to occur.) Computes the distance as though the strings were truncated to
 * MaxLength
 *
 * @tparam MaxLength the maximum length to measure the distance out to.
 * @param strA the first string
 * @param strB the second string
 * @return int the distance
 */
template <size_t MaxLength>
constexpr auto levenshtein_distance =
    [](std::string_view strA, std::string_view strB) -> int {
    // for all i and j, dist[i,j] will hold the Levenshtein distance between
    // the first i characters of s and the first j characters of t
    int dist[MaxLength + 1][MaxLength + 1];
    size_t lenA = std::min(MaxLength, strA.size());
    size_t lenB = std::min(MaxLength, strB.size());

    // source prefixes can be transformed into empty string by
    // dropping all characters
    for (size_t ai = 0; ai <= lenA; ai++)
        dist[ai][0] = ai;

    // target prefixes can be reached from empty source prefix
    // by inserting every character
    for (size_t bi = 0; bi <= lenB; bi++)
        dist[0][bi] = bi;

    for (size_t bi = 0; bi < lenB; bi++) {
        for (size_t ai = 0; ai < lenA; ai++) {
            int substitutionCost = !(strA[ai] == strB[bi]);

            dist[ai + 1][bi + 1] = std::min(
                std::min(
                    dist[ai][bi + 1] + 1,         // deletion
                    dist[ai + 1][bi] + 1),        // insertion
                dist[ai][bi] + substitutionCost); // substitution
        }
    }
    return dist[lenA][lenB];
};

template <size_t MaxLength>
constexpr auto caseless_levenshtein_distance =
    [](std::string_view strA, std::string_view strB) -> int {
    char buffA[MaxLength];
    char buffB[MaxLength];

    size_t lenA = std::min(MaxLength, strA.size());
    size_t lenB = std::min(MaxLength, strB.size());

    for (size_t i = 0; i < lenA; i++) {
        buffA[i] = to_lower(strA[i]);
    }
    for (size_t i = 0; i < lenB; i++) {
        buffB[i] = to_lower(strB[i]);
    }
    return levenshtein_distance<MaxLength>(
        std::string_view(buffA, lenA),
        std::string_view(buffB, lenB));
};
/**
 * @brief Use fuzzy matching to find the index of the option closest to the
 * given value, from an array of options. If two options have the same distance,
 * takes the first one. Returns std::string_view::npos if num_options == 0
 *
 * @tparam MaxLength the maximum length to measure the levenshtein distance out
 * to.
 * @param options pointer to array of options to compare against
 * @param num_options number of options
 * @param value the value
 * @return size_t the index of the closest option
 */
template <size_t MaxLength, class EditDistance>
constexpr size_t fuzzy_match(
    std::string_view const* options,
    size_t num_options,
    std::string_view value,
    EditDistance dist,
    bool use_lowercase = true) {
    char buff[MaxLength];
    if (use_lowercase) {
        size_t N = std::min(MaxLength, value.size());
        for (size_t i = 0; i < N; i++) {
            buff[i] = to_lower(value[i]);
        }
        value = std::string_view(buff, N);
    }
    size_t best_i = std::string_view::npos;
    size_t best_dist = ~(size_t)0;
    for (size_t i = 0; i < num_options; i++) {
        int current_dist = dist(options[i], value);
        if (current_dist < best_dist) {
            best_dist = current_dist;
            best_i = i;
        }
    }
    return best_i;
}
} // namespace nav::impl

namespace nav {
template <class EnumType>
struct enum_traits : private impl::traits_impl<EnumType> {
   private:
    using super = impl::traits_impl<EnumType>;

   public:
    using enum_type = EnumType;
    using base_type = typename super::base_type;
    using super::values;
    constexpr static auto count = values.size();
    constexpr static auto name_lengths = impl::map_array(
        super::names_raw,
        [](std::string_view name) { return name.size(); });

   private:
    constexpr static size_t name_block_buffer_size = count
                                                   + impl::fold(
                                                         name_lengths.data(),
                                                         count,
                                                         0,
                                                         [](size_t acc,
                                                            size_t elem) {
                                                             return acc + elem;
                                                         });
    constexpr static auto name_block = impl::static_cat<name_block_buffer_size>(
        super::names_raw,
        '\0');
    constexpr static auto lowercase_name_block = map_array(
        name_block,
        impl::to_lower);

   public:
    /* A list of all the names in the enum, in declaration order */
    constexpr static auto names = impl::split_by_lengths_assuming_sep(
        name_lengths,
        name_block.data());

   public:
    /* All the enum names, but lowercase. Provided to support lookup
     * operations that ignore case. */
    constexpr static auto lowercase_names = impl::split_by_lengths_assuming_sep(
        name_lengths,
        lowercase_name_block.data());
    constexpr static size_t max_name_length = impl::max_elem<count>(
                                                    name_lengths.data());
    constexpr static base_type min = impl::min_base_value<base_type>(values);
    constexpr static base_type max = impl::max_base_value<base_type>(values);
    constexpr static auto values_to_names = impl::
        select_map<EnumType, std::string_view, count, base_type, min, max>(
            values,
            names,
            "<unnamed>");
    constexpr static auto
        names_to_values = impl::binary_map<std::string_view, EnumType, count>(
            names,
            values);
    constexpr static auto lowercase_names_to_values = impl::
        binary_map<std::string_view, EnumType, count>(lowercase_names, values);
    constexpr static std::optional<EnumType> get_value(std::string_view name) {
        return names_to_values[name];
    }
    constexpr static EnumType get_value(
        std::string_view name,
        EnumType alternative) {
        return names_to_values.get(name, alternative);
    }
    constexpr static std::optional<EnumType> get_value_ignore_case(
        std::string_view name) {
        if (name.size() > max_name_length) {
            return std::nullopt;
        } else {
            char buffer[max_name_length + 1];
            for (size_t i = 0; i < name.size(); i++) {
                buffer[i] = impl::to_lower(name[i]);
            }
            buffer[name.size()] = '\0';
            return lowercase_names_to_values[std::string_view(
                buffer,
                names.size())];
        }
    }
    constexpr static EnumType get_value_ignore_case(
        std::string_view name,
        EnumType alternative) {
        if (name.size() > max_name_length) {
            return alternative;
        } else {
            char buffer[max_name_length + 1];
            for (size_t i = 0; i < name.size(); i++) {
                buffer[i] = impl::to_lower(name[i]);
            }
            buffer[name.size()] = '\0';
            return lowercase_names_to_values.get(
                std::string_view(buffer, names.size()),
                alternative);
        }
    }
    constexpr static std::optional<std::string_view> get_name(EnumType value) {
        return values_to_names[value];
    }
    constexpr static std::string_view get_name(
        EnumType value,
        std::string_view alternative) {
        return values_to_names.get(value, alternative);
    }
    /** Distance metric used for fuzzy string matching. Defaults to
     * caseless levenshtein distance (levenshtein distance, ignoring case)
     */
    constexpr static auto distance_metric = impl::caseless_levenshtein_distance<
        impl::bit_ceil_minus_1(max_name_length * 2)>;
    /* Return the index of the value whose name most closely matches the
     * given name. Compare names based on levenshtein distance. Index is
     * in declaration order, so if the return value is i you can get the
     * corresponding value with values[i] and the corresponding name with
     * names[i]. Converts inputs and names to lowercase by default.*/
    constexpr static size_t find_fuzzy(
        std::string_view name,
        bool use_lowercase = true) {
        return impl::fuzzy_match<impl::bit_ceil_minus_1(max_name_length * 2)>(
            use_lowercase ? lowercase_names.data() : names.data(),
            count,
            name,
            distance_metric,
            use_lowercase);
    }
};
} // namespace nav

#define nav_declare_enum(EnumType, BaseType, ...)                              \
    enum class EnumType : BaseType { __VA_ARGS__ };                            \
    namespace nav::impl {                                                      \
    constexpr auto operator!(EnumType e) {                                     \
        return EnumAssignmentGuard<EnumType> {e};                              \
    }                                                                          \
    template <>                                                                \
    struct traits_impl<EnumType> {                                             \
        friend class ::nav::enum_traits<EnumType>;                             \
        using base_type = BaseType;                                            \
        using enum EnumType;                                                   \
        constexpr static std::string_view qualified_name = #EnumType;          \
        constexpr static std::string_view name = get_top_name(#EnumType);      \
        /* A list of all the values in the enum, in declaration order */       \
        constexpr static auto values = std::array {                            \
            MAP_OPERATOR(EnumType, !, __VA_ARGS__)};                           \
                                                                               \
       private:                                                                \
        constexpr static auto names_raw = split_trim<values.size()>(           \
            #__VA_ARGS__);                                                     \
    };                                                                         \
    } // namespace nav::impl
