/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/

#ifndef CPPA_DETAIL_LIFTED_FUN_HPP
#define CPPA_DETAIL_LIFTED_FUN_HPP

#include "cppa/none.hpp"
#include "cppa/optional.hpp"

#include "cppa/skip_message.hpp"

#include "cppa/util/int_list.hpp"
#include "cppa/util/type_list.hpp"
#include "cppa/util/type_traits.hpp"
#include "cppa/util/left_or_right.hpp"

#include "cppa/detail/tuple_zip.hpp"
#include "cppa/detail/apply_args.hpp"

namespace cppa {
namespace detail {

class lifted_fun_zipper {
 public:
    template <typename F, typename T>
    auto operator()(const F& fun, T& arg) -> decltype(fun(arg)) const {
        return fun(arg);
    }
    // forward everything as reference if no guard/transformation is set
    template <typename T>
    auto operator()(const unit_t&, T& arg) const -> decltype(std::ref(arg)) {
        return std::ref(arg);
    }
};

template <typename T>
T& unopt(T& v) {
    return v;
}

template <typename T>
T& unopt(optional<T>& v) {
    return *v;
}

inline bool has_none() { return false; }

template <typename T, typename... Ts>
inline bool has_none(const T&, const Ts&... vs) {
    return has_none(vs...);
}

template <typename T, typename... Ts>
inline bool has_none(const optional<T>& v, const Ts&... vs) {
    return !v || has_none(vs...);
}

// allows F to have fewer arguments than the lifted_fun calling it
template <typename R, typename F>
class lifted_fun_invoker {

    typedef typename util::get_callable_trait<F>::arg_types arg_types;

    static constexpr size_t args = util::tl_size<arg_types>::value;

 public:
    lifted_fun_invoker(F& fun) : f(fun) {}

    template <typename... Ts>
    typename std::enable_if<sizeof...(Ts) == args, R>::type
    operator()(Ts&... args) const {
        if (has_none(args...)) return none;
        return f(unopt(args)...);
    }

    template <typename T, typename... Ts>
    typename std::enable_if<(sizeof...(Ts) + 1 > args), R>::type
    operator()(T& arg, Ts&... args) const {
        if (has_none(arg)) return none;
        return (*this)(args...);
    }

 private:
    F& f;
};

template <typename F>
class lifted_fun_invoker<bool, F> {

    typedef typename util::get_callable_trait<F>::arg_types arg_types;

    static constexpr size_t args = util::tl_size<arg_types>::value;

 public:
    lifted_fun_invoker(F& fun) : f(fun) {}

    template <typename... Ts>
    typename std::enable_if<sizeof...(Ts) == args, bool>::type
    operator()(Ts&&... args) const {
        if (has_none(args...)) return false;
        f(unopt(args)...);
        return true;
    }

    template <typename T, typename... Ts>
    typename std::enable_if<(sizeof...(Ts) + 1 > args), bool>::type
    operator()(T&& arg, Ts&&... args) const {
        if (has_none(arg)) return false;
        return (*this)(args...);
    }

 private:
    F& f;
};

/**
 * @brief A lifted functor consists of a set of projections, a plain-old
 *        functor and its signature. Note that the signature of the lifted
 *        functor might differ from the underlying functor, because
 *        of the projections.
 */
template <typename F, class ListOfProjections, typename... Args>
class lifted_fun {

 public:
    typedef typename util::get_callable_trait<F>::result_type result_type;

    // Let F be "R (Ts...)" then lifted_fun<F...> returns optional<R>
    // unless R is void in which case bool is returned
    typedef typename std::conditional<std::is_same<result_type, void>::value,
                                      bool, optional<result_type>>::type
    optional_result_type;

    typedef ListOfProjections projections_list;

    typedef typename util::tl_apply<projections_list, std::tuple>::type projections;

    typedef util::type_list<Args...> arg_types;

    lifted_fun() = default;

    lifted_fun(lifted_fun&&) = default;

    lifted_fun(const lifted_fun&) = default;

    lifted_fun& operator=(lifted_fun&&) = default;

    lifted_fun& operator=(const lifted_fun&) = default;

    lifted_fun(F f) : m_fun(std::move(f)) {}

    lifted_fun(F f, projections ps)
            : m_fun(std::move(f)), m_ps(std::move(ps)) {}

    /**
     * @brief Invokes @p fun with a lifted_fun of <tt>args...</tt>.
     */
    optional_result_type operator()(Args... args) {
        auto indices = util::get_indices(m_ps);
        lifted_fun_zipper zip;
        lifted_fun_invoker<optional_result_type, F> invoke{m_fun};
        return apply_args(
            invoke, indices,
            tuple_zip(zip, indices, m_ps, std::forward_as_tuple(args...)));
    }

 private:

    F m_fun;
    projections m_ps;

};

template <typename F, class ListOfProjections, class List>
struct get_lifted_fun;

template <typename F, class ListOfProjections, typename... Ts>
struct get_lifted_fun<F, ListOfProjections, util::type_list<Ts...>> {
    typedef lifted_fun<F, ListOfProjections, Ts...> type;
};

} // namespace detail
} // namespace cppa

#endif // CPPA_DETAIL_LIFTED_FUN_HPP
