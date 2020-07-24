#include "load_balancer_impl.hpp"

namespace GauXC::detail {

LoadBalancerImpl::LoadBalancerImpl( MPI_Comm comm, const Molecule& mol, 
  const MolGrid& mg, const basis_type& basis, std::shared_ptr<MolMeta> molmeta ) :
  comm_(comm), 
  mol_( std::make_shared<Molecule>(mol) ),
  mg_( std::make_shared<MolGrid>(mg)  ),
  basis_( std::make_shared<basis_type>(basis) ),
  molmeta_( molmeta ) { }

LoadBalancerImpl::LoadBalancerImpl( MPI_Comm comm, const Molecule& mol, 
  const MolGrid& mg, const basis_type& basis, const MolMeta& molmeta ) :
  LoadBalancerImpl( comm, mol, mg, basis, std::make_shared<MolMeta>(molmeta) ) { }

LoadBalancerImpl::LoadBalancerImpl( MPI_Comm comm, const Molecule& mol, 
  const MolGrid& mg, const basis_type& basis ) :
  LoadBalancerImpl( comm, mol, mg, basis, std::make_shared<MolMeta>(mol) ) { }

LoadBalancerImpl::LoadBalancerImpl( const LoadBalancerImpl& ) = default;
LoadBalancerImpl::LoadBalancerImpl( LoadBalancerImpl&& ) noexcept = default;

LoadBalancerImpl::~LoadBalancerImpl() noexcept = default;

const std::vector<XCTask>& LoadBalancerImpl::get_tasks() const {
  if( not local_tasks_.size() ) throw std::runtime_error("No Tasks Created");
  return local_tasks_;
}

std::vector<XCTask>& LoadBalancerImpl::get_tasks() {
  if( not local_tasks_.size() ) local_tasks_ = create_local_tasks_();
  return local_tasks_;
}





}