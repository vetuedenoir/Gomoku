#ifndef CONFIG_HPP
# define CONFIG_HPP

# define GOMOKU_DEBUG  // comment out for benchmark mode

// profondeur de recursion minimax
#define DEPTH 8

# ifdef GOMOKU_DEBUG
#   define LOG_DEBUG(tag, msg) Logger::debug(tag, msg)
#   define LOG_INFO(tag, msg)  Logger::info(tag, msg)
#   define LOG_WARN(tag, msg)  Logger::warn(tag, msg)
#   define LOG_ERROR(tag, msg) Logger::error(tag, msg)
    static constexpr bool kCollectStats = true;
namespace detail { template<typename... Ts> inline void suppress(Ts&&...) {} }
#   define LOG_SUPPRESS(...)    ::detail::suppress(__VA_ARGS__)
# else
#   define LOG_DEBUG(tag, msg)
#   define LOG_INFO(tag, msg)
#   define LOG_WARN(tag, msg)
#   define LOG_ERROR(tag, msg)
namespace detail { template<typename... Ts> inline void suppress(Ts&&...) {} }
#   define LOG_SUPPRESS(...)    ::detail::suppress(__VA_ARGS__)
    static constexpr bool kCollectStats = false;
# endif

#endif