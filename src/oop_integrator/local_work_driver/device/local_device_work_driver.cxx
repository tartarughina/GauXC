#include "local_device_work_driver_pimpl.hpp"
#include <stdexcept>

namespace GauXC {

LocalDeviceWorkDriver::LocalDeviceWorkDriver() : 
  pimpl_(nullptr) { }
LocalDeviceWorkDriver::LocalDeviceWorkDriver(pimpl_type&& ptr) :
  pimpl_( std::move(ptr) ){ }

LocalDeviceWorkDriver::~LocalDeviceWorkDriver() noexcept = default;

LocalDeviceWorkDriver::LocalDeviceWorkDriver( LocalDeviceWorkDriver&& other ) noexcept :
  pimpl_(std::move(other.pimpl_)) { }

#define throw_if_invalid_pimpl(ptr) \
  if(not ptr) throw std::runtime_error(std::string("INVALID LocalDeviceWorkDriver PIMPL: ") + std::string(__PRETTY_FUNCTION__) );



#define FWD_TO_PIMPL(NAME) \
void LocalDeviceWorkDriver::NAME( XCDeviceData* device_data ) { \
  throw_if_invalid_pimpl(pimpl_);                               \
  pimpl_->NAME(device_data);                                    \
}


FWD_TO_PIMPL(partition_weights)         // Partition weights

FWD_TO_PIMPL(eval_collocation)          // Collocation
FWD_TO_PIMPL(eval_collocation_gradient) // Collocation Gradient

FWD_TO_PIMPL(eval_xmat)                 // X matrix (P * B)

FWD_TO_PIMPL(eval_uvvar_lda)            // U/VVar LDA (density)
FWD_TO_PIMPL(eval_uvvar_gga)            // U/VVar GGA (density + grad, gamma)

FWD_TO_PIMPL(eval_zmat_lda_vxc)         // Eval Z Matrix LDA VXC
FWD_TO_PIMPL(eval_zmat_gga_vxc)         // Eval Z Matrix GGA VXC

FWD_TO_PIMPL(inc_vxc)                   // Increment VXC by Z 


std::unique_ptr<XCDeviceData> LocalDeviceWorkDriver::create_device_data() {
  throw_if_invalid_pimpl(pimpl_);
  return pimpl_->create_device_data();
}

}
