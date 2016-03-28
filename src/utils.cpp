

#include "utils.h"

double threshold(double num) 
{
  return num > 0 ? num : 0;
}

// computes cumulative sum of vector x
VectorXd cumsum(const VectorXd& x) {
  const int n(x.size());
  VectorXd cmsm(n);
  //cmsm = std::partial_sum(x.data(), x.data() + x.size(), cmsm.data(), std::plus<double>());
  cmsm(0) = x(0);
  
  for (int i = 1; i < n; i++) {
    cmsm(i) = cmsm(i-1) + x(i);
  }
  return (cmsm);
}

// computes reverse cumulative sum of vector x
VectorXd cumsumrev(const VectorXd& x) {
  const int n(x.size());
  VectorXd cmsm(n);
  //std::reverse(x.data(), x.data() + x.size());
  //cmsm = std::partial_sum(x.data(), x.data() + x.size(), cmsm.data(), std::plus<double>());
  cmsm(0) = x(n-1);
  //double tmpsum = 0;
  
  for (int i = 1; i < n; i++) {
    //tmpsum += cmsm(i-1);
    cmsm(i) = cmsm(i-1) + x(n-i-1);
  }
  std::reverse(cmsm.data(), cmsm.data() + cmsm.size());
  return (cmsm);
}

VectorXd sliced_matvecprod(const MatrixXd& A, const VectorXd& b, const std::vector<int>& idx)
{
  const int nn(A.rows());
  const int rr(idx.size());
  VectorXd retvec(nn);
  retvec.setZero();
  
  
  for (int cl = 0; cl < rr; ++cl)
  {
    for (int r = 0; r < nn; ++r)
    {
      retvec(r) += A(r, idx[cl] - 1) * b( idx[cl] - 1 );
    }
  }
  return(retvec);
}

// computes X[,idx]'y
VectorXd sliced_crossprod(const MatrixXd& X, const VectorXd& y, const VectorXi& idx)
{
  const int rr(idx.size());
  VectorXd retvec(rr);
  
  for (int cl = 0; cl < rr; ++cl)
  {
    retvec(cl) = X.col(idx(cl)).dot(y);
  }
  return(retvec);
}

// computes X[,idx]'y
void sliced_crossprod_inplace(VectorXd &res, const MatrixXd& X, const VectorXd& y, const std::vector<int>& idx)
{
  const int rr(idx.size());
  //VectorXd retvec(rr);
  res.setZero();
  
  for (int cl = 0; cl < rr; ++cl)
  {
    res(idx[cl]) = X.col(idx[cl]).dot(y);
  }
  
}

void soft_threshold(SparseVector &res, const VectorXd &vec, const double &penalty)
{
  int v_size = vec.size();
  res.setZero();
  res.reserve(v_size);
  
  const double *ptr = vec.data();
  for(int i = 0; i < v_size; i++)
  {
    if(ptr[i] > penalty)
      res.insertBack(i) = ptr[i] - penalty;
    else if(ptr[i] < -penalty)
      res.insertBack(i) = ptr[i] + penalty;
  }
}

void soft_threshold(VectorXd &res, const VectorXd &vec, const double &penalty)
{
  int v_size = vec.size();
  res.setZero();
  
  const double *ptr = vec.data();
  for(int i = 0; i < v_size; i++)
  {
    if(ptr[i] > penalty)
      res(i) = ptr[i] - penalty;
    else if(ptr[i] < -penalty)
      res(i) = ptr[i] + penalty;
  }
  
}

void update_active_set(VectorXd &u, std::vector<int> &active, std::vector<int> &inactive,
                       double &lambdak, double &lambdakminus1, const int &penalty)
{
  for(std::vector<int>::iterator it = inactive.begin(); it != inactive.end(); ) {
    // the sequential strong rule
    // https://statweb.stanford.edu/~tibs/ftp/strong.pdf
    //std::cout << "var idx: " << *it << std::endl;
    if (std::abs(u(*it)) >= 2 *lambdak - lambdakminus1){
      active.push_back(*it);
      it = inactive.erase(it);
    } else {
      ++it;
    }
  }
}

void initiate_active_set(VectorXd &u, std::vector<int> &active, std::vector<int> &inactive,
                         double &lambdak, double &lambdamax, const int &nvars, const int &penalty)
{
  for (int cl = 0; cl < nvars; ++cl) {
    // the basic strong rule
    if (std::abs(u(cl)) >= 2 * lambdak - lambdamax){
      active.push_back(cl);
    } else {
      inactive.push_back(cl);
    }
  }
}

void block_soft_threshold(SparseVector &res, const VectorXd &vec, const double &penalty,
                                 const int &ngroups, VectorXi &unique_grps, VectorXi &grps)
{
  int v_size = vec.size();
  res.setZero();
  res.reserve(v_size);
  
  for (int g = 0; g < ngroups; ++g) 
  {
    double thresh_factor;
    std::vector<int> gr_idx;
    for (int v = 0; v < v_size; ++v) 
    {
      if (grps(v) == unique_grps(g)) 
      {
        gr_idx.push_back(v);
      }
    }
    if (unique_grps(g) == 0) 
    {
      thresh_factor = 1;
    } else 
    {
      double ds_norm = 0;
      for (std::vector<int>::size_type v = 0; v < gr_idx.size(); ++v)
      {
        int c_idx = gr_idx[v];
        ds_norm += pow(vec(c_idx), 2);
      }
      ds_norm = sqrt(ds_norm);
      double grp_wts = sqrt(gr_idx.size());
      thresh_factor = std::max(0.0, 1 - penalty * grp_wts / (ds_norm) );
    }
    if (thresh_factor != 0.0)
    {
      for (std::vector<int>::size_type v = 0; v < gr_idx.size(); ++v)
      {
        int c_idx = gr_idx[v];
        res.insertBack(c_idx) = vec(c_idx) * thresh_factor;
      }
    }
  }
}


void block_soft_threshold(VectorXd &res, const VectorXd &vec, const double &penalty,
                          const int &ngroups, VectorXi &unique_grps, VectorXi &grps)
{
  int v_size = vec.size();
  res.setZero();
  
  for (int g = 0; g < ngroups; ++g) 
  {
    double thresh_factor;
    std::vector<int> gr_idx;
    for (int v = 0; v < v_size; ++v) 
    {
      if (grps(v) == unique_grps(g)) 
      {
        gr_idx.push_back(v);
      }
    }
    if (unique_grps(g) == 0) 
    {
      thresh_factor = 1;
    } else 
    {
      double ds_norm = 0;
      for (std::vector<int>::size_type v = 0; v < gr_idx.size(); ++v)
      {
        int c_idx = gr_idx[v];
        ds_norm += pow(vec(c_idx), 2);
      }
      ds_norm = sqrt(ds_norm);
      double grp_wts = sqrt(gr_idx.size());
      thresh_factor = std::max(0.0, 1 - penalty * grp_wts / (ds_norm) );
    }
    if (thresh_factor != 0.0)
    {
      for (std::vector<int>::size_type v = 0; v < gr_idx.size(); ++v)
      {
        int c_idx = gr_idx[v];
        res(c_idx) = vec(c_idx) * thresh_factor;
      }
    }
  }
}


/*
static void block_soft_threshold(SparseVector &res, const VectorXd &vec, const double &penalty,
                                 const int &ngroups, const MapVeci &unique_grps, const MapVeci &grps)
{
  int v_size = vec.size();
  res.setZero();
  res.reserve(v_size);
  
  for (int g = 0; g < ngroups; ++g) 
  {
    double thresh_factor;
    std::vector<int> gr_idx;
    for (int v = 0; v < v_size; ++v) 
    {
      if (grps(v) == unique_grps(g)) 
      {
        gr_idx.push_back(v);
      }
    }
    if (unique_grps(g) == 0) 
    {
      thresh_factor = 1;
    } else 
    {
      double ds_norm = 0;
      for (std::vector<int>::size_type v = 0; v < gr_idx.size(); ++v)
      {
        int c_idx = gr_idx[v];
        ds_norm += pow(vec(c_idx), 2);
      }
      ds_norm = sqrt(ds_norm);
      double grp_wts = sqrt(gr_idx.size());
      thresh_factor = std::max(0.0, 1 - penalty * grp_wts / (ds_norm) );
    }
    if (thresh_factor != 0.0)
    {
      for (std::vector<int>::size_type v = 0; v < gr_idx.size(); ++v)
      {
        int c_idx = gr_idx[v];
        res.insertBack(c_idx) = vec(c_idx) * thresh_factor;
      }
    }
  }
}
 */

//computes X'WX where W is diagonal (input w as vector)
MatrixXd XtWX(const MatrixXd& xx, const MatrixXd& ww) {
  const int n(xx.cols());
  MatrixXd AtWA(MatrixXd(n, n).setZero().
    selfadjointView<Lower>().rankUpdate(xx.adjoint() * ww.asDiagonal()));
  return (AtWA);
}

//computes X'X
MatrixXd XtX(const MatrixXd& xx) {
  const int n(xx.cols());
  MatrixXd AtA(MatrixXd(n, n).setZero().
    selfadjointView<Lower>().rankUpdate(xx.adjoint()));
  return (AtA);
}

bool stopRule(const VectorXd& cur, const VectorXd& prev, const double& tolerance) {
  for (unsigned i = 0; i < cur.rows(); i++) {
    if ( (std::abs(cur(i)) > 1e-13 && std::abs(prev(i)) <= 1e-13) || 
         (std::abs(cur(i)) <= 1e-13 && std::abs(prev(i)) > 1e-13) ) {
      return 0;
    }
    if (std::abs(cur(i)) > 1e-13 && std::abs(prev(i)) > 1e-13 && 
        std::abs( (cur(i) - prev(i)) / prev(i)) > tolerance) {
  	  return 0;
    }
  }
  return 1;
}

bool stopRule(const SparseVector& cur, const SparseVector& prev, const double& tolerance) {
  
  
  SparseVector diff = cur - prev;
    
  for(SparseVector::InnerIterator iter(diff); iter; ++iter)
  {
    double prevval = prev.coeff(iter.index());
    double curval  = cur.coeff(iter.index());
    
    if ( (curval != 0 && prevval == 0) || (curval == 0 && prevval != 0) ) {
      return 0;
    }
    
    if (prevval != 0 && curval != 0 && 
        std::abs(iter.value() / prevval) > tolerance)
    {
      return 0;
    }
  }
  return 1;
}

bool stopRuleMat(const MatrixXd& cur, const MatrixXd& prev, const double& tolerance) {
  for (unsigned j = 0; j < cur.cols(); j++) {
    for (unsigned i = 0; i < cur.rows(); i++) {
      if ( (cur(i, j) != 0 && prev(i, j) == 0) 
        || (cur(i, j) == 0 && prev(i, j) != 0) ) {
        return 0;
      }
      if (cur(i, j) != 0 && prev(i, j) != 0 && std::abs( (cur(i, j) - prev(i, j)) / prev(i, j)) > tolerance) {
    	  return 0;
      }
    }
  }
  return 1;
}


//computes X'WX where W is diagonal (input w as vector)
/*SparseMatrix<double> XtWX_sparse(const SparseMatrix<double>& xx, const MatrixXd& ww) {
  const int n(xx.cols());
  SparseMatrix<double> AtWA(n, n);
  AtWA = AtWA.selfadjointView<Lower>().rankUpdate(xx.adjoint() * ww.asDiagonal());
  return (AtWA);
}*/
