/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_SYMBOL_SET_HPP
#define PIRANHA_SYMBOL_SET_HPP

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <limits> // TODO check.
#include <stdexcept>
#include <type_traits>
#include <unordered_map> // TODO check.
#include <utility>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/detail/init_data.hpp>
#include <piranha/exceptions.hpp> // TODO check.
#include <piranha/symbol.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Symbol set.
/**
 * This class represents an ordered set of unique piranha::symbol objects, stored internally in a vector sorted
 * lexicographically according to the symbols' names. The individual piranha::symbol instances can be accessed via
 * iterators or the index operator.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * Move construction and move assignment will leave the moved-from object in a state equivalent to a
 * default-constructed object.
 */
class symbol_set
{
    bool check() const
    {
        // GCOV_EXCL_START
        // Check for sorted range.
        if (!std::is_sorted(begin(), end())) {
            return false;
        }
        if (size() < 2u) {
            return true;
        }
        // Check for duplicates.
        for (size_type i = 0u; i < size() - 1u; ++i) {
            if (m_values[i] == m_values[i + 1u]) {
                return false;
            }
        }
        // GCOV_EXCL_END
        return true;
    }

public:
    /// Size type.
    using size_type = std::vector<symbol>::size_type;
    /// Positions class.
    /**
     * This is a small utility class that can be used to determine the positions,
     * in a piranha::symbol_set \p a, of the symbols in the piranha::symbol_set \p b.
     * The positions are stored internally in an \p std::vector which is guaranteed
     * to be sorted in ascending order and which can be accessed via the iterators
     * provided by this class. If a symbol which is present in \p b is not present in
     * \p a, then it will be ignored.
     *
     * For instance, if the set \p a contains the symbols <tt>[B,C,D,E]</tt> and \p b
     * contains <tt>[A,B,D,F]</tt>, then an instance of this class constructed passing
     * \p a and \p b as parameters will contain the vector <tt>[0,2]</tt>.
     */
    class positions
    {
    public:
        /// Value type.
        /**
         * The positions are represented using the size type of piranha::symbol_set.
         */
        using value_type = size_type;
        /// Const iterator.
        using const_iterator = std::vector<value_type>::const_iterator;
        explicit positions(const symbol_set &, const symbol_set &);
        /// Deleted copy constructor.
        positions(const positions &) = delete;
        /// Defaulted move constructor.
        positions(positions &&) = default;

    private:
        positions &operator=(const positions &) = delete;
        positions &operator=(positions &&) = delete;

    public:
        /// Begin iterator.
        /**
         * @return an iterator to the begin of the internal vector.
         */
        const_iterator begin() const
        {
            return m_values.begin();
        }
        /// End iterator.
        /**
         * @return an iterator to the end of the internal vector.
         */
        const_iterator end() const
        {
            return m_values.end();
        }
        /// Last element.
        /**
         * **NOTE**: the behaviour will be undefined if the size of \p this is zero.
         *
         * @return a const reference to the last element.
         */
        const value_type &back() const
        {
            piranha_assert(m_values.size());
            return m_values.back();
        }
        /// Size.
        /**
         * @return the size of the internal vector.
         */
        std::vector<value_type>::size_type size() const
        {
            return m_values.size();
        }

    private:
        std::vector<value_type> m_values;
    };
    /// Positions map class.
    /**
     * This class is similar to piranha::symbol_set::positions. In addition to storing the positions
     * of the symbols from a map \p b with respect to a reference piranha::symbol_set \p a, it will also store
     * the instances of \p T that were originally mapped to the symbols in \p b.
     *
     * For instance, if \p T is \p int, \p a contains the symbols <tt>[B,C,D,E]</tt> and \p b is the map
     * <tt>[(A,10),(B,20),(D,30),(F,40)]</tt>, then an instance of this class constructed passing
     * \p a and \p b as parameters will contain the vector <tt>[(0,20),(2,30)]</tt>. The internal vector
     * is guaranteed to be sorted according to the position of the symbols.
     *
     * ## Type requirements ##
     *
     * \p T must satisfy piranha::is_mappable.
     */
    template <typename T>
    class positions_map
    {
#if !defined(PIRANHA_DOXYGEN_INVOKED)
        PIRANHA_TT_CHECK(is_mappable, T);
#endif
    public:
        /// Value type.
        /**
         * The stored value type is the <tt>(position,T)</tt> pair.
         */
        using value_type = std::pair<size_type, T>;
        /// Const iterator.
        using const_iterator = typename std::vector<value_type>::const_iterator;
        explicit positions_map(const symbol_set &, const std::unordered_map<symbol, T> &);
        /// Deleted copy constructor.
        positions_map(const positions_map &) = delete;
        /// Defaulted move constructor.
        positions_map(positions_map &&) = default;

    private:
        positions_map &operator=(const positions_map &) = delete;
        positions_map &operator=(positions_map &&) = delete;

    public:
        /// Begin iterator.
        /**
         * @return an iterator to the begin of the internal vector.
         */
        const_iterator begin() const
        {
            return m_pairs.begin();
        }
        /// End iterator.
        /**
         * @return an iterator to the end of the internal vector.
         */
        const_iterator end() const
        {
            return m_pairs.end();
        }
        /// Size.
        /**
         * @return the size of the internal vector.
         */
        typename std::vector<value_type>::size_type size() const
        {
            return m_pairs.size();
        }
        /// Last element.
        /**
         * **NOTE**: the behaviour will be undefined if the size of \p this is zero.
         *
         * @return a const reference to the last element.
         */
        const value_type &back() const
        {
            piranha_assert(m_pairs.size());
            return m_pairs.back();
        }

    private:
        std::vector<value_type> m_pairs;
    };
    /// Const iterator.
    using const_iterator = std::vector<symbol>::const_iterator;
    /// Defaulted default constructor.
    /**
     * This constructor will construct an empty set.
     */
    symbol_set() = default;
    /// Defaulted copy constructor.
    /**
     * @throws unspecified any exception thrown by the copy constructor of \p std::vector.
     */
    symbol_set(const symbol_set &) = default;
    /// Defaulted move constructor.
    symbol_set(symbol_set &&) = default;

private:
    // Enabler for ctor from iterator.
    template <typename Iterator>
    using it_ctor_enabler
        = enable_if_t<conjunction<is_input_iterator<Iterator>,
                                  std::is_constructible<symbol, decltype(*(std::declval<const Iterator &>()))>>::value,
                      int>;

public:
    /// Constructor from range.
    /**
     * \note
     * This constructor is enabled only if \p Iterator is an input iterator
     * from whose value type piranha::symbol is constructible.
     *
     * The set will be initialised with symbols constructed from the elements of the range.
     *
     * @param begin begin iterator.
     * @param end end iterator.
     *
     * @throws unspecified any exception thrown by operations on standard containers or by
     * the invoked constructor of piranha::symbol.
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit symbol_set(Iterator begin, Iterator end)
    {
        // Copy the values from the range into m_values.
        std::transform(begin, end, std::back_inserter(m_values),
                       [](const typename std::iterator_traits<Iterator>::value_type &x) { return symbol{x}; });
        // Sort them in ascending order.
        std::sort(m_values.begin(), m_values.end());
        // Remove duplicates.
        m_values.erase(std::unique(m_values.begin(), m_values.end()), m_values.end());
    }

private:
    template <typename T>
    using init_list_ctor_enabler = enable_if_t<std::is_constructible<symbol, const T &>::value, int>;

public:
    /// Constructor from initializer list.
    /**
     * \note
     * This constructor is enabled only if piranha::symbol is constructible from \p T.
     *
     * This constructor will use symbol_set::symbol_set(Iterator, Iterator) internally
     * to construct a piranha::symbol_set from the values in the input initializer list \p l.
     *
     * @param l list of objects used for the construction of the symbols.
     *
     * @throws unspecified any exception thrown by symbol_set::symbol_set(Iterator, Iterator).
     */
    template <typename T, init_list_ctor_enabler<T> = 0>
    explicit symbol_set(std::initializer_list<T> l) : symbol_set(l.begin(), l.end())
    {
    }
    /// Copy assignment operator.
    /**
     * @param other set to be assigned to \p this.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the copy constructor.
     */
    symbol_set &operator=(const symbol_set &other)
    {
        if (likely(this != &other)) {
            *this = symbol_set(other);
        }
        return *this;
    }
    /// Move assignment operator.
    /**
     * @param other assignment argument.
     *
     * @return reference to \p this.
     */
    symbol_set &operator=(symbol_set &&other) noexcept
    {
        // NOTE: here the idea is that in principle we want to be able to move-assign self,
        // and we don't want to rely on std::vector to support this. Hence, the explicit check.
        if (likely(this != &other)) {
            m_values = std::move(other.m_values);
        }
        return *this;
    }
    /// Trivial destructor.
    ~symbol_set()
    {
        // NOTE: here we should replace with bidirectional tt, if we ever implement it.
        PIRANHA_TT_CHECK(is_forward_iterator, const_iterator);
        piranha_assert(run_destruction_checks());
    }
    /// Index operator.
    /**
     * @param n index of the element to be accessed.
     *
     * @return const reference to the element at index \p n.
     */
    const symbol &operator[](const size_type &n) const
    {
        piranha_assert(n < size());
        return m_values[n];
    }
    /// Begin const iterator.
    /**
     * @return const iterator to the first element of the set, or end() if the set is empty.
     */
    const_iterator begin() const
    {
        return m_values.begin();
    }
    /// End const iterator.
    /**
     * @return const iterator to the element past the last element of the set.
     */
    const_iterator end() const
    {
        return m_values.end();
    }
    /// Add symbol to the set.
    /**
     * The insertion of \p s will preserve the order of the set.
     *
     * @param s piranha::symbol to be inserted.
     *
     * @return \p true if the insertion took place, \p false if the insertion did not take place
     * because an identical symbol is already present in the set.
     *
     * @throws unspecified any exception thrown by the public interface of \p std::vector.
     */
    bool add(const symbol &s)
    {
        const auto it = std::lower_bound(begin(), end(), s);
        if (unlikely(it != end() && *it == s)) {
            // The symbol is there already.
            return false;
        }
        m_values.insert(it, s);
        piranha_assert(check());
        return true;
    }
    /// Add symbol to the set.
    /**
     * Equivalent to constructing a piranha::symbol from \p name and then invoking the other overload of this method.
     *
     * @param name name of the piranha::symbol to be inserted.
     *
     * @return \p true if the insertion took place, \p false if the insertion did not take place
     * because an identical symbol is already present in the set.
     *
     * @throws unspecified any exception thrown by the other overload of this method or by the construction
     * of piranha::symbol from \p std::string.
     */
    bool add(const std::string &name)
    {
        return add(symbol(name));
    }
    /// Remove symbol from the set.
    /**
     * The removal of \p s will preserve the order of the set.
     *
     * @param s piranha::symbol to be removed.
     *
     * @return \p true if the removal took place, \p false if the removal did not take place
     * because the symbol \p s is not present in the set.
     *
     * @throws unspecified any exception thrown by the public interface of \p std::vector.
     */
    bool remove(const symbol &s)
    {
        const auto it = std::lower_bound(begin(), end(), s);
        if (likely(it != end() && *it == s)) {
            m_values.erase(it);
            piranha_assert(check());
            return true;
        }
        return false;
    }
    /// Remove symbol from the set.
    /**
     * Equivalent to constructing a piranha::symbol from \p name and then invoking the other overload of this method.
     *
     * @param name name of the piranha::symbol to be removed.
     *
     * @throws unspecified any exception thrown by the other overload of this method or by the construction
     * of piranha::symbol from \p std::string.
     */
    bool remove(const std::string &name)
    {
        return remove(symbol(name));
    }
    /// Set size.
    /**
     * @return the number of elements in the set.
     */
    size_type size() const
    {
        return m_values.size();
    }
    /// Merge with other set.
    /**
     * @param other merge argument.
     *
     * @return a new set containing the union of the elements present in \p this and \p other.
     *
     * @throws unspecified any exception thrown by the public interface of \p std::vector.
     */
    symbol_set merge(const symbol_set &other) const
    {
        symbol_set retval;
        retval.m_values.reserve(static_cast<size_type>(other.size() + size()));
        std::set_union(begin(), end(), other.begin(), other.end(), std::back_inserter(retval.m_values));
        piranha_assert(retval.check());
        return retval;
    }
    /// Set difference.
    /**
     * @param other difference argument.
     *
     * @return a new set containing the elements of \p this which are not present in \p other.
     *
     * @throws unspecified any exception thrown by the public interface of \p std::vector.
     */
    symbol_set diff(const symbol_set &other) const
    {
        symbol_set retval;
        retval.m_values.reserve(size());
        std::set_difference(begin(), end(), other.begin(), other.end(), std::back_inserter(retval.m_values));
        piranha_assert(retval.check());
        return retval;
    }
    /// Equality operator.
    /**
     * @param other comparison argument.
     *
     * @return \p true if \p this and \p other contain exactly the same symbols, \p false otherwise.
     */
    bool operator==(const symbol_set &other) const
    {
        return m_values == other.m_values;
    }
    /// Inequality operator.
    /**
     * @param other comparison argument.
     *
     * @return opposite of operator==().
     */
    bool operator!=(const symbol_set &other) const
    {
        return !(*this == other);
    }
    /// Index of symbol.
    /**
     * This method will return the index in the set of the input symbol \p s. If \p s is not in the set,
     * the size of the set is returned.
     *
     * @param s piranha::symbol whose index will be computed.
     *
     * @return the index of \p s in the set.
     *
     * @throws std::overflow_error if the size of the set is larger than an implementation-defined value.
     */
    size_type index_of(const symbol &s) const
    {
        // Need to check for potential overflows here.
        using diff_type = std::iterator_traits<const_iterator>::difference_type;
        using udiff_type = std::make_unsigned<diff_type>::type;
        // This is not an exact check, it is conservative by 1 unit in order to simplify the logic.
        if (unlikely(m_values.size() > static_cast<udiff_type>(std::numeric_limits<diff_type>::max()))) {
            piranha_throw(std::overflow_error, "potential overflow in the computaion of the "
                                               "index of a symbol");
        }
        const auto it = std::lower_bound(m_values.begin(), m_values.end(), s);
        if (it != m_values.end() && *it == s) {
            auto retval = it - m_values.begin();
            piranha_assert(retval >= diff_type(0));
            return static_cast<size_type>(retval);
        }
        return m_values.size();
    }

private:
    bool run_destruction_checks() const
    {
        // Run destruction checks only if we are not in the shutdown phase.
        if (shutdown()) {
            return true;
        }
        return check();
    }

private:
    std::vector<symbol> m_values;
};

/// Constructor from sets.
/**
 * The internal positions vector will contain the positions in \p a of the elements
 * of \p b appearing in the set \p a.
 *
 * @param a first set.
 * @param b second set.
 *
 * @throws unspecified any exception thrown by memory errors in standard containers.
 */
// NOTE: it might make sense here in the future to have a constructor from a range of strings instead.
// The rationale would be that like this we would avoid going through string -> symbol conversions
// when using this class from series.
inline symbol_set::positions::positions(const symbol_set &a, const symbol_set &b)
{
    size_type ia = 0u, ib = 0u;
    while (true) {
        if (ia == a.size() || ib == b.size()) {
            break;
        }
        if (a[ia] == b[ib]) {
            m_values.push_back(ia);
            ++ia;
            ++ib;
        } else if (a[ia] < b[ib]) {
            ++ia;
        } else {
            ++ib;
        }
    }
}

/// Constructor from map and set.
/**
 * The internal vector of pairs will contain the positions in \p a of the mapped values
 * of \p map appearing in the set \p a, and the mapped values themselves.
 *
 * @param a reference set.
 * @param map input map.
 *
 * @throws unspecified any exception thrown by memory errors in standard containers, by
 * the copy constructor of \p T, or by <tt>std::stable_sort()</tt>.
 */
template <typename T>
inline symbol_set::positions_map<T>::positions_map(const symbol_set &a, const std::unordered_map<symbol, T> &map)
{
    for (const auto &p : map) {
        const auto idx = a.index_of(p.first);
        if (idx != a.size()) {
            m_pairs.emplace_back(idx, p.second);
        }
    }
    std::stable_sort(m_pairs.begin(), m_pairs.end(),
                     [](const value_type &p1, const value_type &p2) { return p1.first < p2.first; });
    // Check that there are no duplicate positions.
    piranha_assert(std::unique(m_pairs.begin(), m_pairs.end(),
                               [](const value_type &p1, const value_type &p2) { return p1.first == p2.first; })
                   == m_pairs.end());
}
}

#endif
