/*
 * Copyright (C) 2024 Pierre Colin
 * This file is part of Condor.
 * 
 * Condor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, version 3.
 * 
 * Condor is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Condor.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CONDOR_HPP_INCLUDED
#define CONDOR_HPP_INCLUDED

#include <cstdlib>
#include <functional>
#include <map>
#include <stdexcept>
#include <variant>
#include <vector>

#include "condor.h"

namespace cdor {

class duel_matrix
{
	size_t n;
	std::vector<uintmax_t> matrix;

	public:
#if __cplusplus >= 202002L
	constexpr
#endif
	duel_matrix (const size_t n) : n (n), matrix (n * n, 0) {}

#if __cplusplus >= 202002L
	constexpr
#endif
	const uintmax_t &operator () (const size_t i, const size_t j) const {
		return matrix.at (i * n + j);
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	uintmax_t &operator () (const size_t i, const size_t j) {
		return matrix.at (i * n + j);
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	size_t size (void) const noexcept { return n; }

	void cast_safe (std::function<int (const size_t, const size_t)> blt) {
		// requires a side matrix to achieve strong exception guarantee
		std::vector<uint_fast8_t> add (matrix.size (), 0);
		for (size_t i = 1; i < n; i++) {
			for (size_t j = 0; j < i; j++) {
				const int cmp = blt (i, j);
				if (cmp != 0)
					++add[cmp > 0 ? i * n + j : j * n + i];
			}
		}
		std::transform (matrix.begin (), matrix.end (), add.cbegin (),
		                matrix.begin (), std::plus<> {});
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	void cast (std::function<int (const size_t, const size_t)> blt) {
		for (size_t i = 1; i < n; i++) {
			for (size_t j = 0; j < i; j++) {
				const int cmp = blt (i, j);
				if (cmp > 0)
					++matrix[i * n + j];
				else if (cmp < 0)
					++matrix[j * n + i];
			}
		}
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	const uintmax_t *data (void) const noexcept { return matrix.data (); }
};

class preorder_ballot
{
	std::map<size_t, std::pair<uintmax_t, uintmax_t>> ballot;

	public:
	preorder_ballot (void) : ballot () {}

	preorder_ballot (const std::map<size_t,
	                                std::pair<uintmax_t, uintmax_t>> &b) :
		ballot (b)
	{}

	void cast_into (duel_matrix &m) const {
		m.cast ([&](const size_t i, const size_t j) {
			try {
				const auto &[ai, bi] = ballot.at (i);
				const auto &[aj, bj] = ballot.at (j);
				return (ai > bj) - (bi < aj);
			} catch (std::out_of_range &e) {
				return 0;
			}
		});
	}

	void rank (const size_t v, const uintmax_t a, const uintmax_t b) {
		if (a > b)
			throw std::invalid_argument ("invalid rank bounds");
		ballot[v] = std::pair (a, b);
	}

	void unrank (const size_t v) {
		ballot.erase (v);
	}

	bool is_ranked (const size_t v) {
#if __cplusplus >= 202002L
		return ballot.contains (v);
#else
		return ballot.find (v) != ballot.cend ();
#endif
	}
};

class strategy;

class duel_graph
{
	size_t n;
	std::vector<char> matrix;

	friend strategy;

	CondorStrategy get_tagged_union (void) const noexcept {
		return cdor_optimal_strategy (n, matrix.data ());
	}

	public:
#if __cplusplus >= 202002L
	constexpr
#endif
	duel_graph (const size_t n) : n (n), matrix (n * n, 0) {}

#if __cplusplus >= 202002L
	constexpr
#endif
	duel_graph (const duel_matrix &m) : n (m.size ()), matrix (n * n, 0) {
		for (size_t i = 0; i < n; ++i) {
			for (size_t j = 0; j < n; ++j)
				matrix[i * n + j] |= m (i, j) > m (j, i);
		}
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	const char &operator () (const size_t i, const size_t j) const {
		return matrix.at (i * n + j);
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	char &operator () (const size_t i, const size_t j) {
		return matrix.at (i * n + j);
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	size_t size (void) const noexcept { return n; }

#if __cplusplus >= 202002L
	constexpr
#endif
	const char *data (void) const noexcept { return matrix.data (); }
};

class strategy_error : public std::runtime_error
{
	public:
	strategy_error (void) noexcept :
		std::runtime_error ("could not solve for optimal strategy")
	{}
};

class strategy
{
	std::variant<size_t, std::vector<double>> val;

	template <class R>
	static size_t play_mixed (R &rng, const std::vector<double> &vec) {
		std::uniform_real_distribution<double> dist (0.0, 1.0);
		double roll = dist (rng);
		for (size_t i = 0; i < vec.size (); i++) {
			if (roll < vec[i])
				return i;
			roll -= vec[i];
		}
		return vec.size () - 1;
	}

	public:
	strategy (const duel_graph &g) : val () {
		const CondorStrategy res = g.get_tagged_union ();
		if (res.type == CondorStrategy::CDOR_ERROR) {
			throw strategy_error ();
		} else if (res.type == CondorStrategy::CDOR_PURE) {
			val = res.val.pure;
		} else {
			val = std::vector<double> (res.val.mixed,
			                           res.val.mixed + g.size ());
			std::free (res.val.mixed);
		}
	}

	constexpr
	bool is_pure (void) const noexcept {
		return std::holds_alternative<size_t> (val);
	}

	constexpr
	bool is_mixed (void) const noexcept {
		return std::holds_alternative<std::vector<double>> (val);
	}

#if __cplusplus >= 202002L
	constexpr
#endif
	double operator [] (const size_t i) const {
		if (is_pure ())
			return i == std::get<size_t> (val);
		return std::get<std::vector<double>> (val).at (i);
	}

	template <class R>
	size_t play (R &rng) const {
		if (is_pure ())
			return std::get<size_t> (val);
		return play_mixed (rng, std::get<std::vector<double>> (val));
	}
};

}

#endif /* CONDOR_HPP_INCLUDED */
