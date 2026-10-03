#pragma once

namespace exf {

template<class t_>
struct remove_reference {
    using type = t_;
};

template<class t_>
struct remove_reference<t_&> {
    using type = t_;
};

template<class t_>
struct remove_reference<t_&&> {
    using type = t_;
};

template<typename t_>
using remove_reference_t = remove_reference<t_>::type;

template<typename t_>
constexpr remove_reference<t_>::type&& move(t_&& arg) {
    return static_cast<remove_reference<t_>::type&&>(arg);
}

template<typename t_>
constexpr t_ min(const t_ value1, const t_ value2) noexcept {
    return value1 > value2 ? value2 : value1;
}

}
