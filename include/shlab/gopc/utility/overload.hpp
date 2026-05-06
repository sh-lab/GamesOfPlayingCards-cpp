#pragma once

namespace shlab::gopc::utility {

template <typename... Fs>
struct Overload : Fs... {
    using Fs::operator()...;
};

template <typename... Fs>
Overload(Fs...) -> Overload<Fs...>;

}  // namespace shlab::gopc::utility
