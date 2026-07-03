#include <chrono>
#include <vector>
#include <numeric>
#include <optional>
#include <stdexcept>

class Tracker
{
    public:
        using ms     = double;
        using Clock  = std::chrono::steady_clock;
        using Point  = std::chrono::time_point<Clock>;

        void start()
        {
            _start   = Clock::now();
            _running = true;
        }

        ms stop()
        {
            if (!_running)
                throw std::logic_error("Tracker: stop() called without start()");

            const ms elapsed = std::chrono::duration<ms, std::milli>(Clock::now() - _start).count();
            _entries.push_back(elapsed);
            _runningSum += elapsed;
            _running    = false;
            return elapsed;
        }

        void reset()
        {
            _entries.clear();
            _runningSum = 0.0;
            _running    = false;
        }

        ms       average()  const { return _entries.empty() ? 0.0 : _runningSum / _entries.size(); }
        ms       last()     const { return _entries.empty() ? 0.0 : _entries.back(); }
        ms       total()    const { return _runningSum; }
        size_t   count()    const { return _entries.size(); }
        bool     running()  const { return _running; }

        const std::vector<ms>& entries() const { return _entries; }

    private:
        std::vector<ms> _entries;
        ms              _runningSum = 0.0;
        Point           _start;
        bool            _running    = false;
};