#pragma once

#include <vector>
#include <numeric>

#include <gauxc/shell.hpp>

namespace GauXC {

/**
 *  @brief A class to manage a Gaussian type orbital (GTO) basis set
 *
 *  Extends std::vector<Shell<F>>
 *
 *  @tparam F Datatype representing the internal basis set storage
 */
template <typename F>
class BasisSet : public std::vector<Shell<F>> {

public:

  /**
   *  @brief Construct a BasisSet object
   *
   *  Delegates to std::vector<Shell<F>>::vector
   *
   *  @tparam Args Parameter pack for arguements that are passed to
   *  base constructor
   */
  template <typename... Args>
  BasisSet( Args&&... args ) :
    std::vector<Shell<F>>( std::forward<Args>(args)... )  { }

  /// Copy a BasisSet object
  BasisSet( const BasisSet& )     = default;

  /// Move a BasisSet object
  BasisSet( BasisSet&& ) noexcept = default;

  /// Copy-assign BasisSet object
  BasisSet& operator=( const BasisSet& ) = default;

  /// Move-assign BasisSet object
  BasisSet& operator=( BasisSet&& ) noexcept = default;

  /**
   *  @brief Return the number of GTO shells which comprise the BasisSet object
   *
   *  Delegates to std::vector<Shell<F>>::size
   *
   *  @returns the number of GTO shells which comprise the BasisSet object
   */
  inline int32_t nshells() const { return this->size(); }; 

  /**
   *  @brief Return the number of GTO basis functions which comprise the 
   *  BasisSet object.
   *
   *  This routine accumulates the shell sizes (accounting for Cart/Sph angular
   *  factors) for each shell in the basis set.
   *
   *  @returns the number of GTO basis functions which comprise the BasisSet
   *  object.
   */
  inline int32_t nbf()     const {
    return std::accumulate( this->cbegin(), this->cend(), 0ul,
      [](const auto& a, const auto& b) { 
        return a + b.size();
      } );
  };

  /**
   *  @brief Determine the number of basis functions contained in a
   *  specified subset of the BasisSet object.
   *
   *  Performs the following operation:
   *    for( i in shell_list ) nbf += size of shell i
   *
   *  @tparam IntegralIterator Iterator type representing the list of
   *  shell indices.
   *
   *  @param[in] shell_list_begin Start iterator for shell list
   *  @param[in] shell_list_end   End iterator for shell_list
   *  @returns   Number of basis functions in the specified shell subset.
   */
  template <typename IntegralIterator>
  inline int32_t nbf_subset( IntegralIterator shell_list_begin,
                             IntegralIterator shell_list_end ) const {
    int32_t _nbf = 0;
    for( auto it = shell_list_begin; it != shell_list_end; ++it )
      _nbf += std::vector<Shell<F>>::at(*it).size();
    return _nbf;
  }

}; // class BasisSet

} // namespace GauXC
