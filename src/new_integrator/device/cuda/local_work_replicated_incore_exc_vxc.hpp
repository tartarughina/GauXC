#pragma once

#include <memory>

#include <gauxc/types.hpp>
#include <gauxc/basisset.hpp>
#include <gauxc/molecule.hpp>
#include <gauxc/molmeta.hpp>
#include <gauxc/xc_task.hpp>

#include <gauxc/new_xc_integrator/xc_integrator_state.hpp>

#include "device/xc_device_data.hpp"

namespace GauXC {

namespace integrator::cuda {

using host_task_iterator = std::vector<XCTask>::iterator;

template <typename F, size_t n_deriv>
void local_work_replicated_incore_exc_vxc_impl(
  XCWeightAlg            weight_alg,
  XCIntegratorState      state,
  const functional_type& func,
  const BasisSet<F>&     basis,
  const Molecule   &     mol,
  const MolMeta    &     meta,
  XCDeviceData<F>  &     device_data,
  host_task_iterator     local_work_begin,
  host_task_iterator     local_work_end,
  const F*               P,
  F*                     VXC,
  F*                     exc,
  F*                     n_el
);


template <typename F, typename... Args>
inline void local_work_replicated_incore_exc_vxc( size_t n_deriv, Args&&... args ) {
  if( n_deriv == 0 )
    local_work_replicated_incore_exc_vxc_impl<F,0>( std::forward<Args>(args)... );
  else if( n_deriv == 1 )
    local_work_replicated_incore_exc_vxc_impl<F,1>( std::forward<Args>(args)... );
  else
    throw std::runtime_error("MGGA NYI");
}


}
}
