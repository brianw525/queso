/* uq/libs/mcmc/inc/uqCalibProblem.h
 *
 * Copyright (C) 2008 The PECOS Team, http://queso.ices.utexas.edu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __UQ_CALIB_PROBLEM_H__
#define __UQ_CALIB_PROBLEM_H__

#include <uqParamSpace.h>

#include <uqProposalDensity.h>   // For substep 3
#include <uqProposalGenerator.h> // For substep 3

#include <uqDefaultPrior.h>
#include <uqMarkovChainSG1.h>
#include <uqVectorRV.h>

template <class P_V,class P_M>
class uqCalibProblemClass
{
public:
  uqCalibProblemClass(const uqEnvironmentClass&               env,
                      const char*                             prefix,
                      const uqVectorRVClass        <P_V,P_M>& priorRv,
                      const uqProbDensity_BaseClass<P_V,P_M>& likelihoodFunction,
                            uqVectorRVClass        <P_V,P_M>& postRv);
 ~uqCalibProblemClass();

        void solveWithBayesMarkovChain(void* transitionKernel);

        void print                    (std::ostream& os) const;

private:
        void defineMyOptions          (po::options_description& optionsDesc);
        void getMyOptionValues        (po::options_description& optionsDesc);

  const uqEnvironmentClass&                          m_env;
        std::string                                  m_prefix;

        po::options_description*                     m_optionsDesc;
        std::string                                  m_option_help;

  const uqVectorRVClass                   <P_V,P_M>& m_priorRv;
  const uqProbDensity_BaseClass           <P_V,P_M>& m_likelihoodFunction;
        uqVectorRVClass                   <P_V,P_M>& m_postRv;

  const uqProbDensity_BaseClass           <P_V,P_M>* m_priorParamDensity;
        bool                                         m_userPriorDensityIsNull;
        uqDefault_M2lPriorRoutine_DataType<P_V,P_M>  m_m2lPriorRoutine_Data;
        P_V*                                         m_paramPriorMus;
        P_V*                                         m_paramPriorSigmas;
        P_M*                                         m_proposalCovMatrix;
  const uqProposalDensity_BaseClass       <P_V,P_M>* m_proposalDensity;
  const uqProposalGenerator_BaseClass     <P_V,P_M>* m_proposalGenerator;

        uqMarkovChainSGClass              <P_V,P_M>* m_mcSeqGenerator;
        uqProbDensity_BaseClass           <P_V,P_M>* m_solutionProbDensity;
        uqRealizer_BaseClass              <P_V,P_M>* m_solutionRealizer;
};

template<class P_V,class P_M>
std::ostream& operator<<(std::ostream& os, const uqCalibProblemClass<P_V,P_M>& obj);

template <class P_V,class P_M>
uqCalibProblemClass<P_V,P_M>::uqCalibProblemClass(
  const uqEnvironmentClass&               env,
  const char*                             prefix,
  const uqVectorRVClass        <P_V,P_M>& priorRv,
  const uqProbDensity_BaseClass<P_V,P_M>& likelihoodFunction,
        uqVectorRVClass        <P_V,P_M>& postRv)
  :
  m_env                   (env),
  m_prefix                ((std::string)(prefix) + "cal_"),
  m_optionsDesc           (new po::options_description("UQ Calibration Problem")),
  m_option_help           (m_prefix + "help"  ),
  m_priorRv               (priorRv),
  m_likelihoodFunction    (likelihoodFunction),
  m_postRv                (postRv),
  m_priorParamDensity     (NULL),
  m_userPriorDensityIsNull(true),
  m_paramPriorMus         (NULL),
  m_paramPriorSigmas      (NULL),
  m_proposalCovMatrix     (NULL),
  m_proposalDensity       (NULL),
  m_proposalGenerator     (NULL),
  m_mcSeqGenerator        (NULL),
  m_solutionProbDensity   (NULL),
  m_solutionRealizer      (NULL)
{
  if (m_env.rank() == 0) std::cout << "Entering uqCalibProblemClass<P_V,P_M>::constructor()"
                                   << ": prefix = "              << m_prefix
                                   << std::endl;

  defineMyOptions                (*m_optionsDesc);
  m_env.scanInputFileForMyOptions(*m_optionsDesc);
  getMyOptionValues              (*m_optionsDesc);

  if (m_env.rank() == 0) std::cout << "In uqCalibProblemClass<P_V,P_M>::constructor()"
                                   << ": after getting values of options, state of object is:"
                                   << "\n" << *this
                                   << std::endl;

  if (m_env.rank() == 0) std::cout << "Leaving uqCalibProblemClass<P_V,P_M>::constructor()"
                                   << ": prefix = "              << m_prefix
                                   << std::endl;

  return;
}

template <class P_V,class P_M>
uqCalibProblemClass<P_V,P_M>::~uqCalibProblemClass()
{
  if (m_solutionRealizer   ) delete m_solutionRealizer;
  if (m_solutionProbDensity) delete m_solutionProbDensity;
  if (m_mcSeqGenerator     ) delete m_mcSeqGenerator;

  if (m_userPriorDensityIsNull) { 
    delete m_priorParamDensity;
    delete m_paramPriorSigmas;
    delete m_paramPriorMus;
  }

  if (m_optionsDesc) delete m_optionsDesc;
}

template<class P_V,class P_M>
void
uqCalibProblemClass<P_V,P_M>::defineMyOptions(
  po::options_description& optionsDesc)
{
  optionsDesc.add_options()
    (m_option_help.c_str(), "produce help message for calibration problem")
  ;

  return;
}

template<class P_V,class P_M>
void
  uqCalibProblemClass<P_V,P_M>::getMyOptionValues(
  po::options_description& optionsDesc)
{
  if (m_env.allOptionsMap().count(m_option_help.c_str())) {
    std::cout << optionsDesc
              << std::endl;
  }

  return;
}

template <class P_V,class P_M>
void
uqCalibProblemClass<P_V,P_M>::solveWithBayesMarkovChain(void* transitionKernel)
{
  if (m_solutionRealizer   ) delete m_solutionRealizer;
  if (m_solutionProbDensity) delete m_solutionProbDensity;

  if (m_userPriorDensityIsNull) {
    m_paramPriorMus    = new P_V(m_paramSpace->priorMuValues   ());
    m_paramPriorSigmas = new P_V(m_paramSpace->priorSigmaValues());
    m_m2lPriorRoutine_Data.paramPriorMus    = m_paramPriorMus;
    m_m2lPriorRoutine_Data.paramPriorSigmas = m_paramPriorSigmas;

    m_priorParamDensity = new uqRoutineProbDensity_Class<P_V,P_M>(uqDefault_M2lPriorRoutine<P_V,P_M>, // use default prior() routine
                                                                  (void *) &m_m2lPriorRoutine_Data,
                                                                  true); // the routine computes [-2.*ln(Likelihood)]
  }

  // Bayesian step
  m_solutionProbDensity = new uqBayesianProbDensity_Class<P_V,P_M>(m_priorRv.probDensity(),
                                                                  &m_likelihoodFunction);
  m_postRv.setProbDensity(m_solutionProbDensity);

  // Markov Chain step
  if (m_mcSeqGenerator == NULL) {
    m_mcSeqGenerator = new uqMarkovChainSGClass<P_V,P_M>(m_env,
                                                         m_prefix.c_str(),
                                                         m_postRv,
                                                         m_proposalCovMatrix,
                                                         m_proposalDensity,
                                                         m_proposalGenerator);
  }

  m_mcSeqGenerator->generateSequence();
  m_solutionRealizer = new uqRealizer_BaseClass<P_V,P_M>(&(m_mcSeqGenerator->chain()));
  m_postRv.setRealizer(m_solutionRealizer);

  return;
}

template <class P_V,class P_M>
void
uqCalibProblemClass<P_V,P_M>::print(std::ostream& os) const
{
  os << "\n" << "No options for this class";
  os << std::endl;
}

template<class P_V,class P_M>
std::ostream& operator<<(std::ostream& os, const uqCalibProblemClass<P_V,P_M>& obj)
{
  obj.print(os);

  return os;
}
#endif // __UQ_CALIB_PROBLEM_H__