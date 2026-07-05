#pragma once

#if defined(_MSC_VER) && _MSC_VER >= 1934
    #include <functional>

namespace luauapi {
    template <typename Sig>
    using move_only_function = std::move_only_function<Sig>;
}
#elif defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
    #include <functional>

namespace luauapi {
    template <typename Sig>
    using move_only_function = std::move_only_function<Sig>;
}
#else
    #include <std23/move_only_function.h>

namespace luauapi {
    template <typename Sig>
    using move_only_function = std23::move_only_function<Sig>;
}
#endif
