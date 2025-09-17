#include "../include/MultiplexerSelector.hpp"
#include "../include/MiniEventLog.hpp"

#include <vector>
#include <chrono>
#include <limits>
#include <iostream>

#include "../include/InheritedFromIO_Multiplexer/Select_multiplexer.hpp"
#ifdef __linux__
#include "../include/InheritedFromIO_Multiplexer/Epoll_multiplexer.hpp"
#endif
#ifdef __APPLE__
#include "../include/InheritedFromIO_Multiplexer/Kqueue_multiplexer.hpp"
#endif

namespace MiniEventWork {
#ifdef _WIN32
#include "../include/InheritedFromIO_Multiplexer/IOCP_multiplexer.hpp"
#endif

namespace {
    struct Candidate {
        const char* name;
        std::function<std::unique_ptr<IOMultiplexer>()> factory;
    };

    // 运行一个更稳健的微基准：多次 trial，每次 trial 连续 rounds 次 dispatch(0)，取每个 trial 的平均值，最终返回这些 trial 的中位数。
    // 这样可以减少瞬时调度噪声与突发干扰的影响。
    double micro_bench(IOMultiplexer& mux, int rounds = 100, int trials = 3) {
        std::vector<Channel*> sink; // 不使用
        using clock = std::chrono::steady_clock;
        std::vector<double> trial_scores;
        trial_scores.reserve(trials);

        for (int t = 0; t < trials; ++t) {
            try {
                auto t0 = clock::now();
                for (int i = 0; i < rounds; ++i) {
                    // dispatch may throw or behave unexpectedly on some platforms; guard it
                    mux.dispatch(0, sink);
                    sink.clear();
                }
                auto t1 = clock::now();
                auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                trial_scores.push_back(static_cast<double>(ns) / rounds);
            } catch (const std::exception &ex) {
                log_warn("micro_bench: candidate dispatch threw exception: %s", ex.what());
                // push a very large score to deprioritize this candidate
                trial_scores.push_back(std::numeric_limits<double>::infinity());
            } catch (...) {
                log_warn("micro_bench: candidate dispatch threw unknown exception");
                trial_scores.push_back(std::numeric_limits<double>::infinity());
            }
        }

        // 取中位数以减小异常 trial 的影响
        std::sort(trial_scores.begin(), trial_scores.end());
        if (trial_scores.empty()) return std::numeric_limits<double>::infinity();
        return trial_scores[trial_scores.size() / 2];
    }
}

std::unique_ptr<IOMultiplexer> choose_best_multiplexer() {
    std::vector<Candidate> candidates;

    // 平台候选
#ifdef __linux__
    candidates.push_back({"epoll", []{ return std::make_unique<EpollMultiplexer>(); }});
#endif
#ifdef __APPLE__
    candidates.push_back({"kqueue", []{ return std::make_unique<KqueueMultiplexer>(); }});
#endif
#ifdef _WIN32
    candidates.push_back({"iocp", []{ return std::make_unique<IOCPMultiplexer>(); }});
#endif
    // 通用回退
    candidates.push_back({"select", []{ return std::make_unique<SelectMultiplexer>(); }});

    double best_score = std::numeric_limits<double>::infinity();
    std::unique_ptr<IOMultiplexer> best;
    const char* best_name = "";

    for (auto& c : candidates) {
        try {
            auto mux = c.factory();
            if (!mux) continue;
            double score = micro_bench(*mux, /*rounds=*/100, /*trials=*/3);
            log_info("Mux candidate %s: median %f ns/dispatch(0)", c.name, score);
            if (score < best_score) {
                best_score = score;
                best = std::move(mux);
                best_name = c.name;
            }
        } catch (const std::exception &ex) {
            log_warn("Multiplexer candidate %s failed to initialize: %s", c.name, ex.what());
            continue;
        } catch (...) {
            log_warn("Multiplexer candidate %s failed to initialize with unknown error", c.name);
            continue;
        }
    }

    if (!best) {
        log_error("No IO multiplexer available; fallback to Select");
        return std::make_unique<SelectMultiplexer>();
    }
    std::cout << "Using best multiplexer by micro-bench: " << best_name << std::endl;
    return best;
}

} // namespace MiniEventWork
