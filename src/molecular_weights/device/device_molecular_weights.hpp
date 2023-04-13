/**
 * GauXC Copyright (c) 2020-2023, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of
 * any required approvals from the U.S. Dept. of Energy). All rights reserved.
 *
 * See LICENSE.txt for details
 */
#pragma once
#include "../molecular_weights_impl.hpp"
namespace GauXC::detail {

class DeviceMolecularWeights : public MolecularWeightsImpl {

public:

  DeviceMolecularWeights() = delete;
  virtual ~DeviceMolecularWeights() noexcept = default;
  DeviceMolecularWeights( const DeviceMolecularWeights& ) = delete;
  DeviceMolecularWeights( DeviceMolecularWeights&& ) noexcept = default;

  inline DeviceMolecularWeights(std::unique_ptr<LocalWorkDriver>&& lwd) :
    MolecularWeightsImpl(std::move(lwd)) {}

  void modify_weights(LoadBalancer&) const final;

};

template <typename... Args>
std::unique_ptr<MolecularWeightsImpl> 
  make_device_mol_weights_impl(Args&&... args) {
  return std::make_unique<DeviceMolecularWeights>(
    std::forward<Args>(args)...);
}

}
