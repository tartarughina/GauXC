#include "molgrid_impl.hpp"
#include <gauxc/molgrid/defaults.hpp>

namespace GauXC {

MolGrid::MolGrid( const atomic_grid_map& ag ) :
  pimpl_( std::make_shared<detail::MolGridImpl>( ag ) ) { }
  
MolGrid::MolGrid( const MolGrid& )     = default;
MolGrid::MolGrid( MolGrid&& ) noexcept = default;
MolGrid::~MolGrid() noexcept = default;

size_t MolGrid::natoms_uniq() const { return pimpl_->natoms_uniq(); }

const Grid& MolGrid::get_grid( AtomicNumber Z ) const { 
  return pimpl_->get_grid(Z); 
}
Grid& MolGrid::get_grid( AtomicNumber Z ) { 
  return pimpl_->get_grid(Z); 
}

size_t MolGrid::max_nbatches() const {
  return pimpl_->max_nbatches();
}

}

