//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// QUESO - a library to support the Quantification of Uncertainty
// for Estimation, Simulation and Optimization
//
// Copyright (C) 2008,2009,2010,2011,2012,2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-

#include <queso/ConcatenatedJointPdf.h>
#include <queso/GslVector.h>
#include <queso/GslMatrix.h>

namespace QUESO {

// Constructor -------------------------------------
template<class V,class M>
ConcatenatedJointPdf<V,M>::ConcatenatedJointPdf(
  const char*                     prefix,
  const BaseJointPdf<V,M>& density1,
  const BaseJointPdf<V,M>& density2,
  const VectorSet   <V,M>& concatenatedDomain)
  :
  BaseJointPdf<V,M>(((std::string)(prefix)+"concat").c_str(),concatenatedDomain),
  m_densities             (2,(const BaseJointPdf<V,M>*) NULL)
{
  m_densities[0] = &density1;
  m_densities[1] = &density2;

  unsigned int size1 = m_densities[0]->domainSet().vectorSpace().dimLocal();
  unsigned int size2 = m_densities[1]->domainSet().vectorSpace().dimLocal();
  unsigned int size  = concatenatedDomain.vectorSpace().dimLocal();

  UQ_FATAL_TEST_MACRO((size1+size2) != size,
                      m_env.worldRank(),
                      "ConcatenatedJointPdf<V,M>::constructor(1)",
                      "incompatible dimensions");
}
// Constructor -------------------------------------
template<class V,class M>
ConcatenatedJointPdf<V,M>::ConcatenatedJointPdf(
  const char*                                          prefix,
  const std::vector<const BaseJointPdf<V,M>* >& densities,
  const VectorSet<V,M>&                         concatenatedDomain)
  :
  BaseJointPdf<V,M>(((std::string)(prefix)+"concat").c_str(),concatenatedDomain),
  m_densities             (densities.size(),(const BaseJointPdf<V,M>*) NULL)
{
  unsigned int sumSizes = 0;
  for (unsigned i = 0; i < m_densities.size(); ++i) {
    m_densities[i] = densities[i];
    sumSizes += m_densities[i]->domainSet().vectorSpace().dimLocal();
  }

  unsigned int size  = concatenatedDomain.vectorSpace().dimLocal();

  UQ_FATAL_TEST_MACRO(sumSizes != size,
                      m_env.worldRank(),
                      "ConcatenatedJointPdf<V,M>::constructor(2)",
                      "incompatible dimensions");
}
// Destructor --------------------------------------
template<class V,class M>
ConcatenatedJointPdf<V,M>::~ConcatenatedJointPdf()
{
}
// Math methods-------------------------------------
template<class V,class M>
void
ConcatenatedJointPdf<V,M>::setNormalizationStyle(unsigned int value) const
{
  for (unsigned i = 0; i < m_densities.size(); ++i) {
    m_densities[i]->setNormalizationStyle(value);
  }
  return;
}
//--------------------------------------------------
template<class V, class M>
double
ConcatenatedJointPdf<V,M>::actualValue(
  const V& domainVector,
  const V* domainDirection,
        V* gradVector,
        M* hessianMatrix,
        V* hessianEffect) const
{
  if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 54)) {
    *m_env.subDisplayFile() << "Entering ConcatenatedJointPdf<V,M>::actualValue()"
                            << ": domainVector = " << domainVector
                            << std::endl;
  }

  UQ_FATAL_TEST_MACRO(domainVector.sizeLocal() != this->m_domainSet.vectorSpace().dimLocal(),
                      m_env.worldRank(),
                      "ConcatenatedJointPdf<V,M>::actualValue()",
                      "invalid input");

  UQ_FATAL_TEST_MACRO((domainDirection || gradVector || hessianMatrix || hessianEffect),
                      m_env.worldRank(),
                      "ConcatenatedJointPdf<V,M>::actualValue()",
                      "incomplete code for gradVector, hessianMatrix and hessianEffect calculations");

  std::vector<V*> vecs(m_densities.size(),(V*) NULL);
  std::vector<double> values(m_densities.size(),0.);
  double returnValue = 1.;
  unsigned int cummulativeSize = 0;
  for (unsigned int i = 0; i < vecs.size(); ++i) {
    vecs[i] = new V(m_densities[i]->domainSet().vectorSpace().zeroVector());
    domainVector.cwExtract(cummulativeSize,*(vecs[i]));
    values[i] = m_densities[i]->actualValue(*(vecs[i]),NULL,NULL,NULL,NULL);
    returnValue *= values[i];
    if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 99)) {
      *m_env.subDisplayFile() << "In ConcatenatedJointPdf<V,M>::actualValue()"
                              << ", *(vecs[" << i << "]) = "       << *(vecs[i])
                              << ": values[" << i << "] = "        << values[i]
                              << ", temporary cumulative value = " << returnValue
                              << std::endl;
    }
    cummulativeSize += vecs[i]->sizeLocal();
    delete vecs[i];
  }
  //returnValue *= exp(m_logOfNormalizationFactor); // No need, because each PDF should be already normalized [PDF-11]

  if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 54)) {
    *m_env.subDisplayFile() << "Leaving ConcatenatedJointPdf<V,M>::actualValue()"
                            << ": domainVector = " << domainVector
                            << ", returnValue = "  << returnValue
                            << std::endl;
  }

  return returnValue;
}
//--------------------------------------------------
template<class V, class M>
double
ConcatenatedJointPdf<V,M>::lnValue(
  const V& domainVector,
  const V* domainDirection,
        V* gradVector,
        M* hessianMatrix,
        V* hessianEffect) const
{
  if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 54)) {
    *m_env.subDisplayFile() << "Entering ConcatenatedJointPdf<V,M>::lnValue()"
                            << ": domainVector = " << domainVector
                            << std::endl;
  }

  UQ_FATAL_TEST_MACRO((domainDirection || gradVector || hessianMatrix || hessianEffect),
                      m_env.worldRank(),
                      "ConcatenatedJointPdf<V,M>::lnValue()",
                      "incomplete code for gradVector, hessianMatrix and hessianEffect calculations");

  std::vector<V*> vecs(m_densities.size(),(V*) NULL);
  std::vector<double> values(m_densities.size(),0.);
  double returnValue = 0.;
  unsigned int cummulativeSize = 0;
  for (unsigned int i = 0; i < vecs.size(); ++i) {
    vecs[i] = new V(m_densities[i]->domainSet().vectorSpace().zeroVector());
    domainVector.cwExtract(cummulativeSize,*(vecs[i]));
    values[i] = m_densities[i]->lnValue(*(vecs[i]),NULL,NULL,NULL,NULL);
    returnValue += values[i];
    if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 99)) {  // gpmsa
      *m_env.subDisplayFile() << "In ConcatenatedJointPdf<V,M>::lnValue()"
                              << ", *(vecs[" << i << "]) = "       << *(vecs[i])
                              << ": values[" << i << "] = "        << values[i]
                              << ", temporary cumulative value = " << returnValue
                              << std::endl;
    }
    cummulativeSize += vecs[i]->sizeLocal();
    delete vecs[i];
  }
  //returnValue += m_logOfNormalizationFactor; // No need, because each PDF should be already normalized [PDF-11]

  if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 54)) {
    *m_env.subDisplayFile() << "Leaving ConcatenatedJointPdf<V,M>::lnValue()"
                            << ": domainVector = " << domainVector
                            << ", returnValue = "  << returnValue
                            << std::endl;
  }

  return returnValue;
}
//--------------------------------------------------
template<class V, class M>
double
ConcatenatedJointPdf<V,M>::computeLogOfNormalizationFactor(unsigned int numSamples, bool updateFactorInternally) const
{
  double value = 0.;

  if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 2)) {
    *m_env.subDisplayFile() << "Entering ConcatenatedJointPdf<V,M>::computeLogOfNormalizationFactor()"
                            << std::endl;
  }
  double volume = m_domainSet.volume();
  if (((boost::math::isnan)(volume)) ||
      (volume == -INFINITY         ) ||
      (volume ==  INFINITY         ) ||
      (volume <= 0.                )) {
    // Do nothing
  }
  else {
    for (unsigned int i = 0; i < m_densities.size(); ++i) {
      m_densities[i]->computeLogOfNormalizationFactor(numSamples, updateFactorInternally);
    }
  }
  if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 2)) {
    *m_env.subDisplayFile() << "Leaving ConcatenatedJointPdf<V,M>::computeLogOfNormalizationFactor()"
                            << ", m_logOfNormalizationFactor = " << m_logOfNormalizationFactor
                            << std::endl;
  }

  return value;
}

}  // End namespace QUESO

template class QUESO::ConcatenatedJointPdf<QUESO::GslVector, QUESO::GslMatrix>;
