#include <set>

#include <gauxc/xc_integrator/xc_cuda_util.hpp>
#include <gauxc/util/cuda_util.hpp>
#include <gauxc/util/unused.hpp>

#include "cuda/cuda_weights.hpp"
#include "cuda/collocation_device.hpp"
#include "cuda/cuda_pack_density.hpp"
#include "cuda/cuda_inc_potential.hpp"
#include "cuda/cuda_eval_denvars.hpp"
#include "cuda/cuda_zmat.hpp"
#include "integrator_common.hpp"
  
#include "cuda/cublas_extensions.hpp"

#include "host/util.hpp"

namespace GauXC  {
namespace integrator::cuda {

using namespace GauXC::cuda::blas;

auto ranges_from_list( const std::vector<int32_t>& shell_list ) {

  std::vector< std::pair<int32_t,int32_t> > ranges;
  ranges.emplace_back( shell_list.front(), shell_list.back() );

  for( auto it = shell_list.begin(); it != shell_list.end()-1; ++it ) {
    if( *(it+1) - *it != 1 ) {
      ranges.back().second = *it;
      ranges.emplace_back( *(it+1), shell_list.back() );
    }
  }

  return ranges;

}


// Checks if B is a subset of A
template <typename C1, typename C2>
inline auto list_subset( const C1& A, const C2& B ) {
  return std::includes( A.begin(), A.end(), B.begin(), B.end() );
}

template <typename Integral>
inline auto integral_list_intersect( const std::vector<Integral>& A,
                                     const std::vector<Integral>& B ) {


  constexpr size_t sz_ratio = 100;
  const size_t A_sz = A.size();
  const size_t B_sz = B.size();

  const auto A_begin = A.begin();
  const auto A_end   = A.end();
  const auto B_begin = B.begin();
  const auto B_end   = B.end();

  // Fall through if query list is much larger than max list
  if( A_sz * sz_ratio < B_sz ) {
    for( const auto& val : A ) {
      if( std::binary_search( B_begin, B_end, val ) ) 
        return true;
    }
    return false;
  }

  // Fall through if max list is much larger than query list
  if( B_sz * sz_ratio < A_sz ) {
    for( const auto& val : B ) {
      if( std::binary_search( A_begin, A_end, val ) )
        return true;
    }
    return false;
  }

  // Default if lists are about the same size
  auto B_it = B_begin;
  auto A_it = A_begin;

  while( B_it != B_end and A_it != A_end ) {

    if( *B_it < *A_it ) {
      B_it = std::lower_bound( B_it, B_end, *A_it );
      continue;
    }

    if( *A_it < *B_it ) {
      A_it = std::lower_bound( A_it, A_end, *B_it );
      continue;
    }

    return true;

  }

  return false;


}






template <typename Integral>
inline auto integral_list_intersect( const std::vector<Integral>& A,
                                     const std::vector<Integral>& B,
                                     const uint32_t overlap_threshold_spec ) {

  const uint32_t max_intersect_sz  = std::min(A.size(), B.size());
  const uint32_t overlap_threshold = std::min( max_intersect_sz, 
                                               overlap_threshold_spec );

  constexpr size_t sz_ratio = 100;
  const size_t A_sz = A.size();
  const size_t B_sz = B.size();

  const auto A_begin = A.begin();
  const auto A_end   = A.end();
  const auto B_begin = B.begin();
  const auto B_end   = B.end();

  uint32_t overlap_count = 0;

  // Fall through if query list is much larger than max list
  if( A_sz * sz_ratio < B_sz ) {

    for( const auto& val : A ) {
      overlap_count += !!std::binary_search( B_begin, B_end, val );
      if( overlap_count == overlap_threshold ) return true;
    }
    return false;

  }

  // Fall through if max list is much larger than query list
  if( B_sz * sz_ratio < A_sz ) {
    for( const auto& val : B ) {
      overlap_count += !!std::binary_search( A_begin, A_end, val );
      if( overlap_count == overlap_threshold ) return true;
    }
    return false;
  }

  // Default if lists are about the same size
  auto B_it = B_begin;
  auto A_it = A_begin;

  while( B_it != B_end and A_it != A_end ) {

    if( *B_it < *A_it ) {
      B_it = std::lower_bound( B_it, B_end, *A_it );
      continue;
    }

    if( *A_it < *B_it ) {
      A_it = std::lower_bound( A_it, A_end, *B_it );
      continue;
    }

    // *A_it == *B_it if code reaches here
    overlap_count++;
    A_it++; B_it++; // Increment iterators
    if( overlap_count == overlap_threshold) return true;

  }

  return false;


}


template <typename F, size_t n_deriv>
void process_batches_cuda_replicated_density_shellbatched_p(
  util::Timer&           timer,
  XCWeightAlg            weight_alg,
  const functional_type& func,
  const BasisSet<F>&     basis,
  const Molecule   &     mol,
  const MolMeta    &     meta,
  XCCudaData<F>    &     cuda_data,
  host_task_iterator     local_work_begin,
  host_task_iterator     local_work_end,
  const F*               P,
  F*                     VXC,
  F*                     EXC,
  F*                     NEL
) {

  std::cout << "IN SHELL BATCHED\n" << std::flush;
  std::cout << "TOTAL NTASKS = " << std::distance( local_work_begin, local_work_end ) << std:: endl;
  std::cout << "TOTAL NBF    = " << basis.nbf() << std::endl;


  // Zero out final results
  timer.time_op( "XCIntegrator.ZeroHost", [&]() {
    *EXC = 0.;
    *NEL = 0.;
    std::memset( VXC, 0, basis.nbf()*basis.nbf()*sizeof(F) );
  });

#if 0
  size_t nbf     = basis.nbf();
  size_t nshells = basis.nshells();
  size_t natoms  = mol.size();

  // Allocate static quantities on device stack
  cuda_data.allocate_static_data( natoms, n_deriv, nbf, nshells );

  process_batches_cuda_replicated_density_incore_p<F,n_deriv>(
    weight_alg, func, basis, mol, meta, cuda_data, 
    local_work_begin, local_work_end, P, VXC, EXC, NEL
  );
#else

  auto nbe_comparator = []( const auto& task_a, const auto& task_b ) {
    return task_a.nbe < task_b.nbe;
  };


  size_t batch_iter = 0;
  auto task_begin = local_work_begin;

  const size_t natoms  = mol.size();

  while( task_begin != local_work_end ) {

    // Find task with largest NBE
    auto max_task = timer.time_op_accumulate("XCIntegrator.MaxTask", [&]() {
      return std::max_element( task_begin, local_work_end, nbe_comparator );
    } );

    const auto max_shell_list = max_task->shell_list; // copy for reset

    // Init uniion shell list to max shell list outside of loop
    std::set<int32_t> union_shell_set(max_shell_list.begin(), 
                                      max_shell_list.end());


    const uint32_t nbf_threshold = 5000;

    uint32_t overlap_threshold = 400;
    // Partition tasks into those which overlap max_task up to
    // specified threshold
    auto task_end = timer.time_op_accumulate("XCIntegrator.TaskPartition", [&]() {
      return std::partition( task_begin, local_work_end, [&](const auto& t) {
        return integral_list_intersect( max_shell_list, t.shell_list,
                                        overlap_threshold );
      } );
    } );


    // Take union of shell list for all overlapping tasks
    for( auto task_it = task_begin; task_it != task_end; ++task_it ) {
      std::set<int32_t> task_shell_set( task_it->shell_list.begin(), 
                                        task_it->shell_list.end() );
      union_shell_set.merge( task_shell_set );
    }








    std::cout << "FOUND " << std::distance( task_begin, task_end ) 
                          << " OVERLAPPING TASKS" << std::endl;


    std::vector<int32_t> union_shell_list( union_shell_set.begin(),
                                           union_shell_set.end() );

    // Try to add additional tasks given current union list
    task_end = timer.time_op_accumulate("XCIntegrator.TaskPartition", [&]() {
      return std::partition( task_end, local_work_end, [&]( const auto& t ) {
        return list_subset( union_shell_list, t.shell_list );
      } );
    } );

    std::cout << "FOUND " << std::distance( task_begin, task_end ) 
                          << " SUBTASKS" << std::endl;


    // Extract subbasis
    BasisSet<F> basis_subset; basis_subset.reserve(union_shell_list.size());
    timer.time_op_accumulate("XCIntegrator.CopySubBasis",[&]() {
      for( auto i : union_shell_list ) {
        basis_subset.emplace_back( basis.at(i) );
      }
      basis_subset.generate_shell_to_ao();
    });

    const size_t nshells = basis_subset.size();
    const size_t nbe     = basis_subset.nbf();
    std::cout << "TASK_UNION HAS:"   << std::endl
              << "  NSHELLS    = " <<  nshells << std::endl
              << "  NBE        = " <<  nbe     << std::endl;

    // Recalculate shell_list based on subbasis
    timer.time_op_accumulate("XCIntegrator.RecalcShellList",[&]() {
      for( auto _it = task_begin; _it != task_end; ++_it ) {
        auto union_list_idx = 0;
        auto& cur_shell_list = _it->shell_list;
        for( auto j = 0; j < cur_shell_list.size(); ++j ) {
          while( union_shell_list[union_list_idx] != cur_shell_list[j] )
            union_list_idx++;
          cur_shell_list[j] = union_list_idx;
        }
      }
    } );
    
    // Allocate host temporaries
    std::vector<F> P_submat_host(nbe*nbe), VXC_submat_host(nbe*nbe);
    F EXC_tmp, NEL_tmp;
    F* P_submat   = P_submat_host.data();
    F* VXC_submat = VXC_submat_host.data();

    // Extract subdensity
    auto [union_submat_cut, foo] = 
      integrator::gen_compressed_submat_map( basis, union_shell_list, 
        basis.nbf(), basis.nbf() );

    timer.time_op_accumulate("XCIntegrator.ExtractSubDensity",[&]() {
      detail::submat_set( basis.nbf(), basis.nbf(), nbe, nbe, P, basis.nbf(), 
                          P_submat, nbe, union_submat_cut );
    } );
   

    // Allocate static quantities on device stack
    cuda_data.allocate_static_data( natoms, n_deriv, nbe, nshells );


    // Process batches on device with subobjects
    process_batches_cuda_replicated_density_incore_p<F,n_deriv>(
      weight_alg, func, basis_subset, mol, meta, cuda_data, 
      task_begin, task_end, P_submat, VXC_submat, &EXC_tmp, &NEL_tmp
    );

    // Update full quantities
    *EXC += EXC_tmp;
    *NEL += NEL_tmp;
    timer.time_op_accumulate("XCIntegrator.IncrementSubPotential",[&]() {
      detail::inc_by_submat( basis.nbf(), basis.nbf(), nbe, nbe, VXC, basis.nbf(), 
                             VXC_submat, nbe, union_submat_cut );
    });


    // Reset shell_list to be wrt full basis
    timer.time_op_accumulate("XCIntegrator.ResetShellList",[&]() {
      for( auto _it = task_begin; _it != task_end; ++_it ) 
      for( auto j = 0; j < _it->shell_list.size();  ++j  ) {
        _it->shell_list[j] = union_shell_list[_it->shell_list[j]];
      }
    });


    // Update task iterator for next set of batches
    task_begin = task_end;

    batch_iter++;
  }


#endif

}


#define CUDA_IMPL( F, ND ) \
template \
void process_batches_cuda_replicated_density_shellbatched_p<F, ND>(\
  util::Timer&           timer,\
  XCWeightAlg            weight_alg,\
  const functional_type& func,\
  const BasisSet<F>&     basis,\
  const Molecule   &     mol,\
  const MolMeta    &     meta,\
  XCCudaData<F>    &     cuda_data,\
  host_task_iterator     local_work_begin,\
  host_task_iterator     local_work_end,\
  const F*               P,\
  F*                     VXC,\
  F*                     exc,\
  F*                     n_el\
) 

CUDA_IMPL( double, 0 );
CUDA_IMPL( double, 1 );

}
}

