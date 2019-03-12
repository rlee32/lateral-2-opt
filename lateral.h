#pragma once

#include "Swap.h"
#include "TourModifier.h"
#include "solver.h"

namespace lateral {

inline bool is_valid_move(const TourModifier& tour
    , primitives::point_id_t i
    , primitives::point_id_t j
    , primitives::length_t current_length
    , primitives::length_t desired_cost)
{
    auto new_length {tour.length_map().compute_length(i, j)};
    new_length += tour.length_map().compute_length(tour.next(i), tour.next(j));
    return new_length == current_length + desired_cost;
}

inline std::vector<Swap> find_swaps(const TourModifier& tour
    , primitives::length_t cost)
{
    std::vector<Swap> swaps;
    constexpr primitives::point_id_t start {0};
    // first segment cannot be compared with last segment.
    auto end {tour.prev(start)};
    const auto first_old_length {tour.length(start)};
    for (primitives::point_id_t i {tour.next(tour.next(start))}; i != end; i = tour.next(i))
    {
        const auto current_length {first_old_length + tour.length(i)};
        if (is_valid_move(tour, start, i, current_length, cost))
        {
            swaps.push_back({start, i, cost});
        }
    }

    end = tour.prev(end);
    for (primitives::point_id_t i {tour.next(start)}; i != end; i = tour.next(i))
    {
        const auto first_old_length {tour.length(i)};
        auto j {tour.next(tour.next(i))};
        while (j != start)
        {
            const auto current_length {first_old_length + tour.length(j)};
            if (is_valid_move(tour, i, j, current_length, cost))
            {
                swaps.push_back({i, j, cost});
            }
            j = tour.next(j);
        }
    }
    return swaps;
}

inline void test_range(const TourModifier& tour)
{
    constexpr primitives::length_t max_cost {100};
    for (primitives::length_t i {0}; i < max_cost; ++i)
    {
        find_swaps(tour, i);
    }
}

inline Swap restricted_first_improvement(const TourModifier& tour
    , const primitives::point_id_t first_min
    , const primitives::point_id_t first_max
    , const primitives::point_id_t second_min
    , const primitives::point_id_t second_max)
{
    constexpr primitives::point_id_t start {0};
    // first segment cannot be compared with last segment.
    auto end {tour.prev(start)};
    const auto first_old_length {tour.length(start)};
    for (primitives::point_id_t i {tour.next(tour.next(start))}; i != end; i = tour.next(i))
    {
        const auto current_length {first_old_length + tour.length(i)};
        const auto improvement {solver::compute_improvement(tour, start, i, current_length)};
        if (improvement > 0)
        {
            if (first_min == std::min(start, tour.next(start))
                and first_max == std::max(start, tour.next(start))
                and second_min == std::min(i, tour.next(i))
                and second_max == std::max(i, tour.next(i)))
            {
                continue;
            }
            return {start, i, improvement};
        }
    }

    end = tour.prev(end);
    for (primitives::point_id_t i {tour.next(start)}; i != end; i = tour.next(i))
    {
        const auto first_old_length {tour.length(i)};
        auto j {tour.next(tour.next(i))};
        while (j != start)
        {
            const auto current_length {first_old_length + tour.length(j)};
            const auto improvement {solver::compute_improvement(tour, i, j, current_length)};
            if (improvement > 0)
            {
                if (first_min == std::min(start, tour.next(start))
                    and first_max == std::max(start, tour.next(start))
                    and second_min == std::min(i, tour.next(i))
                    and second_max == std::max(i, tour.next(i)))
                {
                    continue;
                }
                return {i, j, improvement};
            }
            j = tour.next(j);
        }
    }
    return {};
}

inline TourModifier perturbation_climb(const std::vector<Swap>& swaps, const TourModifier& tour)
{
    const auto original_length {tour.length()};
    const auto original_points {tour.order()};
    for (const auto& swap : swaps)
    {
        TourModifier new_tour(tour);
        new_tour.move(swap.a, swap.b);
        while (true)
        {
            const auto new_swap {restricted_first_improvement(new_tour
                , std::min(swap.a, swap.b)
                , std::max(swap.a, swap.b)
                , std::min(tour.next(swap.a), tour.next(swap.b))
                , std::max(tour.next(swap.a), tour.next(swap.b)))};
            if (new_swap.improvement == 0)
            {
                break;
            }
            new_tour.move(new_swap.a, new_swap.b);
        }
        solver::hill_climb(new_tour);
        if (new_tour.length() < original_length)
        {
            return new_tour;
        }
    }
    return tour;
}

inline TourModifier perturbation_climb(const TourModifier& tour, primitives::length_t cost)
{
    const auto swaps {find_swaps(tour, cost)};
    return perturbation_climb(swaps, tour);
}

inline TourModifier perturbation_climb(const TourModifier& tour)
{
    const auto original_length {tour.length()};
    constexpr primitives::length_t max_cost {1000};
    for (primitives::length_t i {0}; i < max_cost; ++i)
    {
        const auto new_tour {perturbation_climb(tour, i)};
        if (new_tour.length() < original_length)
        {
            return new_tour;
        }
    }
    return tour;
}

} // namespace lateral
