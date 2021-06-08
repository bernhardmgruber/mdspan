/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 2.0
//              Copyright (2019) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Christian R. Trott (crtrott@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#pragma once

#include "macros.hpp"
#include "fixed_layout_impl.hpp"
#include "static_array.hpp"
#include "extents.hpp"
#include "trait_backports.hpp"
#include "depend_on_instantiation.hpp"

#include <algorithm>
#include <numeric>
#include <array>

namespace std {
namespace experimental {

struct layout_stride {
  template <class Extents>
  class mapping {
    static_assert(detail::__depend_on_instantiation_v<Extents, false>, "layout_stride::mapping instantiated with invalid extents type.");
  };

  template <size_t... Exts>
  class mapping<
    std::experimental::extents<Exts...>
  > : private detail::__no_unique_address_emulation<
        detail::__compressed_pair<
          ::std::experimental::extents<Exts...>,
          ::std::experimental::dextents<sizeof...(Exts)>
        >
      >
  {
  public:
    using extents_type = experimental::extents<Exts...>;
  private:

    using idx_seq = make_index_sequence<sizeof...(Exts)>;

    //----------------------------------------------------------------------------

    using __strides_storage_t = ::std::experimental::dextents<sizeof...(Exts)>;
    using __member_pair_t = detail::__compressed_pair<extents_type, __strides_storage_t>;
    using __base_t = detail::__no_unique_address_emulation<__member_pair_t>;

    MDSPAN_FORCE_INLINE_FUNCTION constexpr __strides_storage_t const&
    __strides_storage() const noexcept { return this->__base_t::__ref().__second(); }
    MDSPAN_FORCE_INLINE_FUNCTION _MDSPAN_CONSTEXPR_14 __strides_storage_t&
    __strides_storage() noexcept { return this->__base_t::__ref().__second(); }

    //----------------------------------------------------------------------------

    template <class>
    friend class mapping;

    //----------------------------------------------------------------------------

    // Workaround for non-deducibility of the index sequence template parameter if it's given at the top level
    template <class>
    struct __impl_deduction_workaround;

    template <size_t... Idxs>
    struct __impl_deduction_workaround<index_sequence<Idxs...>>
    {
      template <class OtherExtents>
      MDSPAN_INLINE_FUNCTION
      static constexpr bool _eq_impl(mapping const& self, mapping<OtherExtents> const& other) noexcept {
        return _MDSPAN_FOLD_AND((self.template __stride<Idxs>() == other.template __stride<Idxs>()) /* && ... */);
      }
      template <class OtherExtents>
      MDSPAN_INLINE_FUNCTION
      static constexpr bool _not_eq_impl(mapping const& self, mapping<OtherExtents> const& other) noexcept {
        return _MDSPAN_FOLD_OR((self.template __stride<Idxs>() != other.template __stride<Idxs>()) /* || ... */);
      }

      template <class... Integral>
      MDSPAN_FORCE_INLINE_FUNCTION
      static constexpr size_t _call_op_impl(mapping const& self, Integral... idxs) noexcept {
        return _MDSPAN_FOLD_PLUS_RIGHT((idxs * self.template __stride<Idxs>()), /* + ... + */ 0);
      }

      MDSPAN_INLINE_FUNCTION
      static constexpr size_t _req_span_size_impl(mapping const& self) noexcept {
        // assumes no negative strides; not sure if I'm allowed to assume that or not
        return __impl::_call_op_impl(self, (self.extents().template __extent<Idxs>() - 1)...) + 1;
      }
    };

    // Can't use defaulted parameter in the __impl_deduction_workaround template because of a bug in MSVC warning C4348.
    using __impl = __impl_deduction_workaround<make_index_sequence<sizeof...(Exts)>>;


    //----------------------------------------------------------------------------

    MDSPAN_INLINE_FUNCTION constexpr explicit
    mapping(
      __base_t&& __b
    ) : __base_t(::std::move(__b)) {}

    //----------------------------------------------------------------------------

  public: // (but not really)
    template <size_t N>
    struct __static_stride_workaround {
      static constexpr size_t value = dynamic_extent;
    };

    template <size_t N>
    MDSPAN_FORCE_INLINE_FUNCTION
    constexpr size_t __stride() const noexcept {
      return __strides_storage().extent(N);
    }

    MDSPAN_INLINE_FUNCTION
    static constexpr mapping
    __make_mapping(
      detail::__partially_static_sizes<Exts...>&& __exts,
      detail::__partially_static_sizes<detail::__make_dynamic_extent_integral<Exts>()...>&& __strs
    ) noexcept {
      // call the private constructor we created for this purpose
      return mapping(
        __base_t{
          __member_pair_t(
            extents_type::__make_extents_impl(::std::move(__exts)),
            __strides_storage_t{::std::move(__strs)}
          )
        }
      );
    }

  public:

    //--------------------------------------------------------------------------------

    MDSPAN_INLINE_FUNCTION_DEFAULTED constexpr mapping() noexcept = default;
    MDSPAN_INLINE_FUNCTION_DEFAULTED constexpr mapping(mapping const&) noexcept = default;
    MDSPAN_INLINE_FUNCTION_DEFAULTED constexpr mapping(mapping&&) noexcept = default;
    MDSPAN_INLINE_FUNCTION_DEFAULTED _MDSPAN_CONSTEXPR_14_DEFAULTED
    mapping& operator=(mapping const&) noexcept = default;
    MDSPAN_INLINE_FUNCTION_DEFAULTED _MDSPAN_CONSTEXPR_14_DEFAULTED
    mapping& operator=(mapping&&) noexcept = default;
    MDSPAN_INLINE_FUNCTION_DEFAULTED ~mapping() noexcept = default;

    // TODO @proposal-bug layout stride needs this constructor
    MDSPAN_INLINE_FUNCTION
    constexpr
    mapping(
      std::experimental::extents<Exts...> const& e,
      array<size_t, __strides_storage_t::rank()> const& strides
    ) noexcept
      : __base_t(__base_t{__member_pair_t(e, __strides_storage_t(strides))})
    { }


    //--------------------------------------------------------------------------------

    MDSPAN_INLINE_FUNCTION constexpr extents_type extents() const noexcept {
      return this->__base_t::__ref().__first();
    };

    MDSPAN_INLINE_FUNCTION constexpr bool is_unique() const noexcept { return true; }
    // TODO @proposal-bug this wording for this is (at least slightly) broken (should at least be "... stride(p[0]) == 1...")
    MDSPAN_INLINE_FUNCTION _MDSPAN_CONSTEXPR_14 bool is_contiguous() const noexcept {
      // TODO @testing test layout_stride is_contiguous()
      auto rem = array<size_t, sizeof...(Exts)>{ };
      std::iota(rem.begin(), rem.end(), size_t(0));
      auto next_idx_iter = std::find_if(
        rem.begin(), rem.end(),
        [&](size_t i) { return this->stride(i) == 1;  }
      );
      if(next_idx_iter != rem.end()) {
        size_t prev_stride_times_prev_extent =
          this->extents().extent(*next_idx_iter) * this->stride(*next_idx_iter);
        // "remove" the index
        constexpr size_t removed_index_sentinel = -1;
        *next_idx_iter = removed_index_sentinel;
        int found_count = 1;
        while (found_count != sizeof...(Exts)) {
          next_idx_iter = std::find_if(
            rem.begin(), rem.end(),
            [&](size_t i) {
              return i != removed_index_sentinel
                && this->stride(i) * this->extents().extent(i) == prev_stride_times_prev_extent;
            }
          );
          if (next_idx_iter != rem.end()) {
            // "remove" the index
            *next_idx_iter = removed_index_sentinel;
            ++found_count;
            prev_stride_times_prev_extent = stride(*next_idx_iter) * this->extents().extent(*next_idx_iter);
          } else { break; }
        }
        return found_count == sizeof...(Exts);
      }
      return false;
    }
    MDSPAN_INLINE_FUNCTION constexpr bool is_strided() const noexcept { return true; }

    MDSPAN_INLINE_FUNCTION static constexpr bool is_always_unique() noexcept { return true; }
    MDSPAN_INLINE_FUNCTION static constexpr bool is_always_contiguous() noexcept {
      return false;
    }
    MDSPAN_INLINE_FUNCTION static constexpr bool is_always_strided() noexcept { return true; }

    MDSPAN_TEMPLATE_REQUIRES(
      class... Indices,
      /* requires */ (
        sizeof...(Indices) == sizeof...(Exts) &&
        _MDSPAN_FOLD_AND(_MDSPAN_TRAIT(is_constructible, Indices, size_t) /*&& ...*/)
      )
    )
    MDSPAN_FORCE_INLINE_FUNCTION
    constexpr size_t operator()(Indices... idxs) const noexcept {
      return __impl::_call_op_impl(*this, idxs...);
    }

    MDSPAN_INLINE_FUNCTION
    constexpr size_t stride(size_t r) const noexcept {
      return __strides_storage().extent(r);
    }

    MDSPAN_INLINE_FUNCTION
    constexpr size_t required_span_size() const noexcept {
      // assumes no negative strides; not sure if I'm allowed to assume that or not
      return __impl::_req_span_size_impl(*this);
    }

    // TODO @proposal-bug these (and other analogous operators) should be non-member functions
    // TODO @proposal-bug these should do more than just compare extents!

    template<class OtherExtents>
    MDSPAN_INLINE_FUNCTION
    friend constexpr bool operator==(mapping const& lhs, mapping<OtherExtents> const& rhs) noexcept {
      return __impl::_eq_impl(lhs, rhs);
    }

    template<class OtherExtents>
    MDSPAN_INLINE_FUNCTION
    friend constexpr bool operator!=(mapping const& lhs, mapping<OtherExtents> const& rhs) noexcept {
      return __impl::_not_eq_impl(lhs, rhs);
    }

  };
};

} // end namespace experimental
} // end namespace std
