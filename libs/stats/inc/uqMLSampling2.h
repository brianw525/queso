/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------
 *
 * Copyrightg (C) 2008 The PECOS Development Team
 *
 * Please see http://pecos.ices.utexas.edu for more information.
 *
 * This file is part of the QUESO Library (Quantification of Uncertainty
 * for Estimation, Simulation and Optimization).
 *
 * QUESO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUESO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUESO. If not, see <http://www.gnu.org/licenses/>.
 *
 *--------------------------------------------------------------------------
 *
 * $Id$
 *
 * Brief description of this file: 
 * 
 *--------------------------------------------------------------------------
 *-------------------------------------------------------------------------- */

#ifndef __UQ_MULTI_LEVEL_SAMPLING2_H__
#define __UQ_MULTI_LEVEL_SAMPLING2_H__

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Level0(
  const uqMLSamplingLevelOptionsClass& defaultLevelOptions,        // input
  unsigned int&                        unifiedRequestedNumSamples, // output
  uqSequenceOfVectorsClass<P_V,P_M>&   currChain,                  // output
  uqScalarSequenceClass<double>&       currLogLikelihoodValues,    // output
  uqScalarSequenceClass<double>&       currLogTargetValues)        // output
{
  char tmpSufix[256];

    sprintf(tmpSufix,"%d_",m_currLevel+LEVEL_REF_ID); // Yes, '+0'
    uqMLSamplingLevelOptionsClass currOptions(m_env,(m_options.m_prefix + tmpSufix).c_str());
    currOptions.scanOptionsValues(&defaultLevelOptions);

    if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
      *m_env.subDisplayFile() << "KEY In uqMLSampling<P_V,P_M>::generateSequence()"
                              << ": beginning level "              << m_currLevel+LEVEL_REF_ID
                              << ", currOptions.m_rawChainSize = " << currOptions.m_rawChainSize // Ok to use rawChainSize
                              << std::endl;
    }

    int iRC = UQ_OK_RC;
    struct timeval timevalLevel;
    iRC = gettimeofday(&timevalLevel, NULL);

    if (m_env.inter0Rank() >= 0) {
      unsigned int tmpSize = currOptions.m_rawChainSize;
      int mpiRC = MPI_Allreduce((void *) &tmpSize, (void *) &unifiedRequestedNumSamples, (int) 1, MPI_UNSIGNED, MPI_SUM, m_env.inter0Comm().Comm());
      UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                          m_env.fullRank(),
                          "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                          "failed MPI_Allreduce() for requested num samples in level 0");
    }
    else {
      unifiedRequestedNumSamples = currOptions.m_rawChainSize;
    }

    currChain.setName              (currOptions.m_prefix + "rawChain");
    currLogLikelihoodValues.setName(currOptions.m_prefix + "rawLogLikelihood");
    currLogTargetValues.setName    (currOptions.m_prefix + "rawLogTarget");

    currChain.resizeSequence              (currOptions.m_rawChainSize); // Ok to use rawChainSize
    currLogLikelihoodValues.resizeSequence(currOptions.m_rawChainSize); // Ok to use rawChainSize
    currLogTargetValues.resizeSequence    (currOptions.m_rawChainSize); // Ok to use rawChainSize

    P_V auxVec(m_vectorSpace.zeroVector());
    for (unsigned int i = 0; i < currChain.subSequenceSize(); ++i) {
      //std::cout << "In QUESO: before prior realizer with i = " << i << std::endl;
      m_priorRv.realizer().realization(auxVec);
      currChain.setPositionValues(i,auxVec);
      // KAUST: all nodes should call here
      currLogLikelihoodValues[i] = m_likelihoodFunction.lnValue(auxVec,NULL,NULL,NULL,NULL);  // likelihood is important
      currLogTargetValues[i]     = m_priorRv.pdf().lnValue(auxVec,NULL,NULL,NULL,NULL) + currLogLikelihoodValues[i];
      //std::cout << "In QUESO: currLogTargetValues[" << i << "] = " << currLogTargetValues[i] << std::endl;
    }

    if (m_env.inter0Rank() >= 0) { // KAUST
      if (currOptions.m_rawChainComputeStats) {
        std::ofstream* genericOfsVar = NULL;
        m_env.openOutputFile(currOptions.m_dataOutputFileName,
                             UQ_FILE_EXTENSION_FOR_MATLAB_FORMAT,
                             currOptions.m_dataOutputAllowedSet,
                             false,
                             genericOfsVar);

        currChain.computeStatistics(*currOptions.m_rawChainStatisticalOptions,
                                    genericOfsVar);

        //genericOfsVar->close();
        delete genericOfsVar;
      }

      if (currOptions.m_rawChainDataOutputFileName != UQ_MH_SG_FILENAME_FOR_NO_FILE) {
        currChain.unifiedWriteContents              (currOptions.m_rawChainDataOutputFileName);
        currLogLikelihoodValues.unifiedWriteContents(currOptions.m_rawChainDataOutputFileName);
        currLogTargetValues.unifiedWriteContents    (currOptions.m_rawChainDataOutputFileName);
      }

      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ": finished generating " << currChain.subSequenceSize()
                                << " chain positions"
                                << std::endl;

        //unsigned int numZeros = 0;
        //for (unsigned int i = 0; i < currTargetValues.subSequenceSize(); ++i) {
        //  *m_env.subDisplayFile() << "currTargetValues[" << i
        //                          << "] = " << currTargetValues[i]
        //                          << std::endl;
        //  if (currTargetValues[i] == 0.) numZeros++;
        //}
        //*m_env.subDisplayFile() << "Number of zeros in currTargetValues = " << numZeros
        //                        << std::endl;
      }
    } // KAUST

    UQ_FATAL_TEST_MACRO((currChain.subSequenceSize() != currOptions.m_rawChainSize), // Ok to use rawChainSize
                        m_env.fullRank(),
                        "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                        "currChain (first one) has been generated with invalid size");
    double levelRunTime = uqMiscGetEllapsedSeconds(&timevalLevel);

    if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
      *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                              << ": ending level " << m_currLevel+LEVEL_REF_ID
                              << " after " << levelRunTime << " seconds"
                              << std::endl;
    }

  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step1(
  const uqMLSamplingLevelOptionsClass* currOptions,                // input
  unsigned int&                        unifiedRequestedNumSamples) // output
{
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ": beginning step 1 of 9"
                                << std::endl;
      }

      unsigned int tmpSize = currOptions->m_rawChainSize;
      // This computed 'unifiedRequestedNumSamples' needs to be recomputed only at the last
      // level, when 'currOptions' is replaced by 'lastLevelOptions' (see step 3 of 9)
      int mpiRC = MPI_Allreduce((void *) &tmpSize, (void *) &unifiedRequestedNumSamples, (int) 1, MPI_UNSIGNED, MPI_SUM, m_env.inter0Comm().Comm());
      UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                          m_env.fullRank(),
                          "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                          "failed MPI_Allreduce() for requested num samples in step 1");

      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "KEY In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ", currOptions->m_rawChainSize = " << currOptions->m_rawChainSize // Ok to use rawChainSize
                                << std::endl;
      }

  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step2(
  const uqMLSamplingLevelOptionsClass* currOptions,             // input
  uqSequenceOfVectorsClass<P_V,P_M>&   currChain,               // input/output
  uqScalarSequenceClass<double>&       currLogLikelihoodValues, // input/output
  uqScalarSequenceClass<double>&       currLogTargetValues,     // input/output
  uqSequenceOfVectorsClass<P_V,P_M>&   prevChain,               // output
  uqScalarSequenceClass<double>&       prevLogLikelihoodValues, // output
  uqScalarSequenceClass<double>&       prevLogTargetValues,     // output
  unsigned int&                        indexOfFirstWeight,      // output
  unsigned int&                        indexOfLastWeight)       // output
{
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ": beginning step 2 of 9"
                                << std::endl;
      }

      prevChain = currChain;
      currChain.clear();
      currChain.setName(currOptions->m_prefix + "rawChain");

      prevLogLikelihoodValues = currLogLikelihoodValues; // likelihood is important
      prevLogTargetValues     = currLogTargetValues;

      currLogLikelihoodValues.clear();
      currLogLikelihoodValues.setName(currOptions->m_prefix + "rawLogLikelihood");

      currLogTargetValues.clear();
      currLogTargetValues.setName(currOptions->m_prefix + "rawLogTarget");

#if 0 // For debug only
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        P_V prevPosition(m_vectorSpace.zeroVector());
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ":"
                                << std::endl;
        for (unsigned int i = 0; i < prevChain.subSequenceSize(); ++i) {
          prevChain.getPositionValues(i,prevPosition);
          *m_env.subDisplayFile() << "  prevChain[" << i
                                  << "] = " << prevPosition
                                  << ", prevLogLikelihoodValues[" << i
                                  << "] = " << prevLogLikelihoodValues[i]
                                  << ", prevLogTargetValues[" << i
                                  << "] = " << prevLogTargetValues[i]
                                  << std::endl;
        }
      }
#endif

      unsigned int quantity1 = prevChain.unifiedSequenceSize();
      unsigned int quantity2 = currChain.unifiedSequenceSize();
      unsigned int quantity3 = prevLogLikelihoodValues.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
      unsigned int quantity4 = currLogLikelihoodValues.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
      unsigned int quantity5 = prevLogTargetValues.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
      unsigned int quantity6 = currLogTargetValues.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ": prevChain.unifiedSequenceSize() = " << quantity1
                                << ", currChain.unifiedSequenceSize() = " << quantity2
                                << ", prevLogLikelihoodValues.unifiedSequenceSize() = " << quantity3
                                << ", currLogLikelihoodValues.unifiedSequenceSize() = " << quantity4
                                << ", prevLogTargetValues.unifiedSequenceSize() = " << quantity5
                                << ", currLogTargetValues.unifiedSequenceSize() = " << quantity6
                                << std::endl;
      }

      UQ_FATAL_TEST_MACRO((prevChain.subSequenceSize() != prevLogLikelihoodValues.subSequenceSize()),
                          m_env.fullRank(),
                          "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                          "different sizes between previous chain and previous sequence of likelihood values");

      UQ_FATAL_TEST_MACRO((prevChain.subSequenceSize() != prevLogTargetValues.subSequenceSize()),
                          m_env.fullRank(),
                          "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                          "different sizes between previous chain and previous sequence of target values");

      // Set 'indexOfFirstWeight' and 'indexOfLastWeight' // KAUST
      indexOfFirstWeight = 0;
      indexOfLastWeight  = indexOfFirstWeight + prevChain.subSequenceSize()-1;
      {
        //std::cout << "m_env.inter0Comm().NumProc() = " << m_env.inter0Comm().NumProc() << std::endl;
        int r = m_env.inter0Rank();
        //std::cout << "r = " << r << std::endl;
        m_env.inter0Comm().Barrier();
        unsigned int auxUint = 0;
        if (r > 0) {
          MPI_Status status;
	  //std::cout << "Rank " << r << " is entering MPI_Recv()" << std::endl;
          int mpiRC = MPI_Recv((void*) &auxUint, 1, MPI_UNSIGNED, r-1, r-1, m_env.inter0Comm().Comm(), &status);
	  //std::cout << "Rank " << r << " received auxUint = " << auxUint << std::endl;
          UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                              m_env.fullRank(),
                              "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                              "failed MPI_Recv()");
          indexOfFirstWeight = auxUint;
          indexOfLastWeight = indexOfFirstWeight + prevChain.subSequenceSize()-1;
        }
        if (r < (m_env.inter0Comm().NumProc()-1)) {
          auxUint = indexOfLastWeight + 1;
	  //std::cout << "Rank " << r << " is sending auxUint = " << auxUint << std::endl;
          int mpiRC = MPI_Send((void*) &auxUint, 1, MPI_UNSIGNED, r+1, r, m_env.inter0Comm().Comm());
	  //std::cout << "Rank " << r << " sent auxUint = " << auxUint << std::endl;
          UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                              m_env.fullRank(),
                              "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                              "failed MPI_Send()");
        }
        m_env.inter0Comm().Barrier();
      }

  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step3(
  const uqMLSamplingLevelOptionsClass* currOptions,             // input
  const uqScalarSequenceClass<double>& prevLogLikelihoodValues, // input
  double                               prevExponent,            // input
  double&                              currExponent,            // output
  uqScalarSequenceClass<double>&       weightSequence)          // output
{
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ": beginning step 3 of 9"
                                << std::endl;
      }

      std::vector<double> exponents(2,0.);
      exponents[0] = prevExponent;
      exponents[1] = 1.;

      double nowExponent = 1.; // Try '1.' right away
      double nowEffectiveSizeRatio = 0.; // To be computed

      unsigned int nowAttempt = 0;
      bool testResult = false;
      double meanEffectiveSizeRatio = .5*(currOptions->m_minEffectiveSizeRatio + currOptions->m_maxEffectiveSizeRatio);
      uqScalarSequenceClass<double> omegaLnDiffSequence(m_env,prevLogLikelihoodValues.subSequenceSize(),"");

      double nowUnifiedEvidenceLnFactor = 0.;
      do {
        if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                  << ", level " << m_currLevel+LEVEL_REF_ID
                                  << ", step "  << m_currStep
                                  << ": entering loop for computing next exponent"
                                  << ", with nowAttempt = " << nowAttempt
                                  << std::endl;
        }

        if (nowAttempt > 0) {
          if (nowEffectiveSizeRatio > meanEffectiveSizeRatio) {
            exponents[0] = nowExponent;
          }
          else {
            exponents[1] = nowExponent;
          }
          nowExponent = .5*(exponents[0] + exponents[1]);
        }
        double auxExponent = nowExponent;
        if (prevExponent != 0.) {
          auxExponent /= prevExponent;
          auxExponent -= 1.;
        }
        double subWeightRatioSum     = 0.;
        double unifiedWeightRatioSum = 0.;

        for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
          omegaLnDiffSequence[i] = prevLogLikelihoodValues[i]*auxExponent; // likelihood is important
        }

        double unifiedOmegaLnMax = 0.;
        double unifiedOmegaLnMin = 0.;
        omegaLnDiffSequence.unifiedMinMax(m_vectorSpace.numOfProcsForStorage() == 1, // KAUST3
                                          0,
                                          omegaLnDiffSequence.subSequenceSize(),
                                          unifiedOmegaLnMin,
                                          unifiedOmegaLnMax);
        for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
          omegaLnDiffSequence[i] -= unifiedOmegaLnMax;
          weightSequence[i] = exp(omegaLnDiffSequence[i]);
          subWeightRatioSum += weightSequence[i];
        }
        int mpiRC = MPI_Allreduce((void *) &subWeightRatioSum, (void *) &unifiedWeightRatioSum, (int) 1, MPI_DOUBLE, MPI_SUM, m_env.inter0Comm().Comm());
        UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                            m_env.fullRank(),
                            "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                            "failed MPI_Allreduce() for weight ratio sum");

        nowUnifiedEvidenceLnFactor = log(unifiedWeightRatioSum) + unifiedOmegaLnMax - log(weightSequence.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1));

        double effectiveSampleSize = 0.;
        for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
          weightSequence[i] /= unifiedWeightRatioSum;
          effectiveSampleSize += weightSequence[i]*weightSequence[i];
          //if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          //  *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
          //                          << ", level "                 << m_currLevel+LEVEL_REF_ID
          //                          << ", step "                  << m_currStep
          //                          << ": i = "                   << i
          //                          << ", effectiveSampleSize = " << effectiveSampleSize
          //                          << std::endl;
          //}
        }

#if 0 // For debug only
        if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                  << ", level " << m_currLevel+LEVEL_REF_ID
                                  << ", step "  << m_currStep
                                  << ":"
                                  << std::endl;
        }
        for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
          if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
            *m_env.subDisplayFile() << "  weightSequence[" << i
                                    << "] = "              << weightSequence[i]
                                    << std::endl;
          }
        }
#endif

        double subQuantity = effectiveSampleSize;
        effectiveSampleSize = 0.;
        mpiRC = MPI_Allreduce((void *) &subQuantity, (void *) &effectiveSampleSize, (int) 1, MPI_DOUBLE, MPI_SUM, m_env.inter0Comm().Comm());
        UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                            m_env.fullRank(),
                            "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                            "failed MPI_Allreduce() for effective sample size");

        effectiveSampleSize = 1./effectiveSampleSize;
        nowEffectiveSizeRatio = effectiveSampleSize/((double) weightSequence.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1));
        UQ_FATAL_TEST_MACRO((nowEffectiveSizeRatio > (1.+1.e-8)),
                            m_env.fullRank(),
                            "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                            "effective sample size ratio cannot be > 1");
        //UQ_FATAL_TEST_MACRO((nowEffectiveSizeRatio < (1.-1.e-8)),
        //                    m_env.fullRank(),
        //                    "uqMLSamplingClass<P_V,P_M>::generateSequence()",
        //                    "effective sample size ratio cannot be < 1");

        //bool aux1 = (nowEffectiveSizeRatio == meanEffectiveSizeRatio);
        bool aux2 = (nowExponent == 1.                             )
                    &&
                    (nowEffectiveSizeRatio > meanEffectiveSizeRatio);
        bool aux3 = (nowEffectiveSizeRatio >= currOptions->m_minEffectiveSizeRatio)
                    &&
                    (nowEffectiveSizeRatio <= currOptions->m_maxEffectiveSizeRatio);
        testResult = aux2 || aux3;

        if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                  << ", level "                   << m_currLevel+LEVEL_REF_ID
                                  << ", step "                    << m_currStep
                                  << ": nowAttempt = "            << nowAttempt
                                  << ", prevExponent = "          << prevExponent
                                  << ", exponents[0] = "          << exponents[0]
                                  << ", nowExponent = "           << nowExponent
                                  << ", exponents[1] = "          << exponents[1]
                                  << ", effectiveSampleSize = "   << effectiveSampleSize
                                  << ", weightSequenceSize = "    << weightSequence.subSequenceSize()
                                  << ", minEffectiveSizeRatio = " << currOptions->m_minEffectiveSizeRatio
                                  << ", nowEffectiveSizeRatio = " << nowEffectiveSizeRatio
                                  << ", maxEffectiveSizeRatio = " << currOptions->m_maxEffectiveSizeRatio
                                  << std::endl;
        }
        nowAttempt++;

        // Make sure all nodes in 'inter0Comm' have the same value of 'nowExponent'
        uqMiscCheckForSameValueInAllNodes(nowExponent,
                                          0.,
                                          m_env.inter0Comm(),
                                          "uqMLSamplingClass<P_V,P_M>::generateSequence(), step 3, testResult");

        // Make sure all nodes in 'inter0Comm' have the same value of 'testResult'
        uqMiscCheckForSameValueInAllNodes(testResult,
                                          0.,
                                          m_env.inter0Comm(),
                                          "uqMLSamplingClass<P_V,P_M>::generateSequence(), step 3, testResult");
      } while (testResult == false);
      currExponent = nowExponent;
      m_logEvidenceFactors.push_back(nowUnifiedEvidenceLnFactor);

      unsigned int quantity1 = weightSequence.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level "                                  << m_currLevel+LEVEL_REF_ID
                                << ", step "                                   << m_currStep
                                << ": weightSequence.subSequenceSize() = "     << weightSequence.subSequenceSize()
                                << ", weightSequence.unifiedSequenceSize() = " << quantity1
                                << ", currExponent = "                         << currExponent
                                << ", effective ratio = "                      << nowEffectiveSizeRatio
                                << ", log(evidence factor) = "                 << m_logEvidenceFactors[m_logEvidenceFactors.size()-1]
                                << ", evidence factor = "                      << exp(m_logEvidenceFactors[m_logEvidenceFactors.size()-1])
                                << std::endl;

        //unsigned int numZeros = 0;
        //for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
        //  *m_env.subDisplayFile() << "weightSequence[" << i
        //                          << "] = " << weightSequence[i]
        //                         << std::endl;
        //  if (weightSequence[i] == 0.) numZeros++;
        //}
        //*m_env.subDisplayFile() << "Number of zeros in weightSequence = " << numZeros
        //                        << std::endl;
      }

      // Make sure all nodes in 'inter0Comm' have the same value of 'logEvidenceFactor'
      uqMiscCheckForSameValueInAllNodes(m_logEvidenceFactors[m_logEvidenceFactors.size()-1],
                                        1.e-16,
                                        m_env.inter0Comm(),
                                        "uqMLSamplingClass<P_V,P_M>::generateSequence(), step 3, logEvidenceFactor");
  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step4(
  const uqSequenceOfVectorsClass<P_V,P_M>& prevChain,        // input
  const uqScalarSequenceClass<double>&     weightSequence,   // input
  P_M&                                     unifiedCovMatrix) // output
{
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ": beginning step 4 of 9"
                                << std::endl;
      }

      P_V auxVec(m_vectorSpace.zeroVector());
      P_V weightedMeanVec(m_vectorSpace.zeroVector());
      for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
        prevChain.getPositionValues(i,auxVec);
        weightedMeanVec += weightSequence[i]*auxVec;
      }

      P_V diffVec(m_vectorSpace.zeroVector());
      P_M subCovMatrix(m_vectorSpace.zeroVector());
      for (unsigned int i = 0; i < weightSequence.subSequenceSize(); ++i) {
        prevChain.getPositionValues(i,auxVec);
        diffVec = auxVec - weightedMeanVec;
        subCovMatrix += weightSequence[i]*matrixProduct(diffVec,diffVec);
      }

      for (unsigned int i = 0; i < unifiedCovMatrix.numRowsLocal(); ++i) { // KAUST5
        for (unsigned int j = 0; j < unifiedCovMatrix.numCols(); ++j) {
          double localValue = subCovMatrix(i,j);
          double sumValue = 0.;
          if (m_env.inter0Rank() >= 0) {
            int mpiRC = MPI_Allreduce((void *) &localValue, (void *) &sumValue, (int) 1, MPI_DOUBLE, MPI_SUM, m_env.inter0Comm().Comm());
            UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                                m_env.fullRank(),
                                "uqMLSamplingClass<P_V,P_M>::generateSequence()",
                                "failed MPI_Allreduce() for cov matrix");
          }
          else {
            sumValue = localValue;
          }
          unifiedCovMatrix(i,j) = sumValue;
        }
      }

      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence()"
                                << ", level "              << m_currLevel+LEVEL_REF_ID
                                << ", step "               << m_currStep
                                << ": unifiedCovMatrix = " << unifiedCovMatrix
                                << std::endl;
      }

  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step5()
{
  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step6()
{
  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step7()
{
  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step8(
  const uqSequenceOfVectorsClass<P_V,P_M>& prevChain,                         // input
  unsigned int                             indexOfFirstWeight,                // input
  unsigned int                             indexOfLastWeight,                 // input
  const std::vector<double>&               unifiedWeightStdVectorAtProc0Only, // input
  const uqScalarSequenceClass<double>&     weightSequence,                    // input
  double                                   prevEta,                           // input
  const uqGenericVectorRVClass<P_V,P_M>&   currRv,                            // input
  uqMLSamplingLevelOptionsClass*           currOptions,                       // input (changed temporarily internally)
  P_M&                                     unifiedCovMatrix,                  // input/output
  double&                                  currEta)                           // output
{
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                << ", level " << m_currLevel+LEVEL_REF_ID
                                << ", step "  << m_currStep
                                << ": beginning step 8 of 9"
                                << std::endl;
      }

      double beforeEta           = prevEta;
      double beforeRejectionRate = 0.;               // To be updated
      bool   beforeRejectionRateIsBelowRange = true; // To be updated

      double nowEta           = prevEta;
      double nowRejectionRate = 0.;               // To be computed
      bool   nowRejectionRateIsBelowRange = true; // To be computed

      std::vector<double> etas(2,0.);
      etas[0] = beforeEta;
      etas[1] = 1.;

      std::vector<double> rejs(2,0.);
      rejs[0] = 0.; // To be computed
      rejs[1] = 0.; // To be computed

      unsigned int nowAttempt = 0;
      bool testResult = false;
      double meanRejectionRate = .5*(currOptions->m_minRejectionRate + currOptions->m_maxRejectionRate);
      bool useMiddlePointLogicForEta = false;
      P_M nowCovMatrix(unifiedCovMatrix);
#if 0 // KAUST, to check
      std::vector<double> unifiedWeightStdVectorAtProc0Only(0);
      weightSequence.getUnifiedContentsAtProc0Only(m_vectorSpace.numOfProcsForStorage() == 1,
                                                   unifiedWeightStdVectorAtProc0Only);
#endif
      do {
        if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                  << ", level " << m_currLevel+LEVEL_REF_ID
                                  << ", step "  << m_currStep
                                  << ": entering loop for assessing rejection rate"
                                  << ", with nowAttempt = "  << nowAttempt
                                  << ", nowRejectionRate = " << nowRejectionRate
                                  << std::endl;
        }
        nowCovMatrix = unifiedCovMatrix;

        if (nowRejectionRate < currOptions->m_minRejectionRate) {
          nowRejectionRateIsBelowRange = true;
        }
        else if (nowRejectionRate > currOptions->m_maxRejectionRate) {
          nowRejectionRateIsBelowRange = false;
        }
        else {
          UQ_FATAL_TEST_MACRO(true,
                              m_env.fullRank(),
                              "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                              "nowRejectionRate should be out of the requested range at this point of the logic");
        }

        if (m_env.inter0Rank() >= 0) { // KAUST
          if (nowAttempt > 0) {
            if (useMiddlePointLogicForEta == false) {
              if (nowAttempt == 1) {
                // Ok, keep useMiddlePointLogicForEta = false
              }
              else if ((beforeRejectionRateIsBelowRange == true) &&
                       (nowRejectionRateIsBelowRange    == true)) {
                // Ok
              }
              else if ((beforeRejectionRateIsBelowRange == false) &&
                       (nowRejectionRateIsBelowRange    == false)) {
                // Ok
              }
              else if ((beforeRejectionRateIsBelowRange == true ) &&
                       (nowRejectionRateIsBelowRange    == false)) {
                useMiddlePointLogicForEta = true;

                // This is the first time the middle point logic will be used below
                etas[0] = std::min(beforeEta,nowEta);
                etas[1] = std::max(beforeEta,nowEta);

                if (etas[0] == beforeEta) {
                  rejs[0] = beforeRejectionRate;
                  rejs[1] = nowRejectionRate;
                }
                else {
                  rejs[0] = nowRejectionRate;
                  rejs[1] = beforeRejectionRate;
                }
              }
              else if ((beforeRejectionRateIsBelowRange == false) &&
                       (nowRejectionRateIsBelowRange    == true )) {
                useMiddlePointLogicForEta = true;

                // This is the first time the middle point logic will be used below
                etas[0] = std::min(beforeEta,nowEta);
                etas[1] = std::max(beforeEta,nowEta);
              }
              else {
                UQ_FATAL_TEST_MACRO(true,
                                    m_env.fullRank(),
                                    "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                                    "before and now range flags are inconsistent");
              }
            } // if (useMiddlePointLogicForEta == false)

            beforeEta                       = nowEta;
            beforeRejectionRate             = nowRejectionRate;
            beforeRejectionRateIsBelowRange = nowRejectionRateIsBelowRange;
            if (useMiddlePointLogicForEta == false) {
              if (beforeRejectionRateIsBelowRange) nowEta *= 4.;
              else                                 nowEta /= 4.;
              if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
                *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                        << ", level " << m_currLevel+LEVEL_REF_ID
                                        << ", step "  << m_currStep
                                        << ": in loop for assessing rejection rate"
                                        << ", with nowAttempt = "  << nowAttempt
                                        << ", useMiddlePointLogicForEta = false"
                                        << ", nowEta just updated to value (to be tested) " << nowEta
                                        << std::endl;
              }
            }
            else {
              if (nowRejectionRate > meanRejectionRate) {
                if (rejs[0] > meanRejectionRate) {
                  etas[0] = nowEta;
                  etas[1] = etas[1];
                }
                else {
                  etas[0] = etas[0];
                  etas[1] = nowEta;
                }
              }
              else {
                if (rejs[0] < meanRejectionRate) {
                  etas[0] = nowEta;
                  etas[1] = etas[1];
                }
                else {
                  etas[0] = etas[0];
                  etas[1] = nowEta;
                }
              }
              nowEta = .5*(etas[0] + etas[1]);
              if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
                *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                        << ", level " << m_currLevel+LEVEL_REF_ID
                                        << ", step "  << m_currStep
                                        << ": in loop for assessing rejection rate"
                                        << ", with nowAttempt = " << nowAttempt
                                        << ", useMiddlePointLogicForEta = true"
                                        << ", nowEta just updated to value (to be tested) " << nowEta
                                        << ", etas[0] = " << etas[0]
                                        << ", etas[1] = " << etas[1]
                                        << std::endl;
              }
            }
          } // if (nowAttempt > 0)
        } // if (m_env.inter0Rank() >= 0) // KAUST

        nowCovMatrix *= nowEta;

        unsigned int originalSubNumSamples = 1 + (unsigned int) ( (1.-meanRejectionRate)/meanRejectionRate/currOptions->m_covRejectionRate/currOptions->m_covRejectionRate );

        if (m_env.inter0Rank() >= 0) { // KAUST
          if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
            *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                    << ", level " << m_currLevel+LEVEL_REF_ID
                                    << ", step "  << m_currStep
                                    << ": in loop for assessing rejection rate"
                                    << ", about to sample "     << originalSubNumSamples << " indexes"
                                    << ", meanRejectionRate = " << meanRejectionRate
                                    << ", covRejectionRate = "  << currOptions->m_covRejectionRate
                                    << std::endl;
          }
        } // KAUST

        std::vector<unsigned int> nowUnifiedIndexCountersAtProc0Only(0); // It will be resized by 'sampleIndexesAtProc0()' below
        if (m_env.inter0Rank() >= 0) { // KAUST
          unsigned int tmpUnifiedNumSamples = originalSubNumSamples*m_env.inter0Comm().NumProc();
          sampleIndexesAtProc0(tmpUnifiedNumSamples,                // input
                               unifiedWeightStdVectorAtProc0Only,   // input
                               nowUnifiedIndexCountersAtProc0Only); // output

          unsigned int auxUnifiedSize = weightSequence.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
          if (m_env.inter0Rank() == 0) {
            UQ_FATAL_TEST_MACRO(nowUnifiedIndexCountersAtProc0Only.size() != auxUnifiedSize,
                                m_env.fullRank(),
                                "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                                "wrong output from sampleIndexesAtProc0() in step 8");
          }

          if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
            *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                    << ", level " << m_currLevel+LEVEL_REF_ID
                                    << ", step "  << m_currStep
                                    << ": in loop for assessing rejection rate"
                                    << ", about to distribute sampled assessment indexes"
                                    << std::endl;
          }
        } // KAUST

        uqBalancedLinkedChainsPerNodeStruct<P_V> nowBalLinkControl;
        uqUnbalancedLinkedChainsPerNodeStruct    nowUnbLinkControl; // KAUST

        unsigned int Np = 0;
        if (m_env.inter0Rank() == 0) { // Yes, '== 0'
          Np = (unsigned int) m_env.inter0Comm().NumProc();
        }
        std::vector<uqExchangeInfoStruct> exchangeStdVec(0);

        // All processors should call this routine in order to have the same decision value
        bool useBalancedChains = decideOnBalancedChains(currOptions,                        // input
                                                        indexOfFirstWeight,                 // input
                                                        indexOfLastWeight,                  // input
                                                        nowUnifiedIndexCountersAtProc0Only, // input
                                                        exchangeStdVec);                    // output

        if (m_env.inter0Rank() >= 0) { // KAUST
          if (useBalancedChains) {
            prepareBalLinkedChains(currOptions,                        // input
                                   prevChain,                          // input
                                   exchangeStdVec,                     // input/output
                                   nowBalLinkControl);                 // output
          }
          else {
            prepareUnbLinkedChains(indexOfFirstWeight,                 // input
                                   indexOfLastWeight,                  // input
                                   nowUnifiedIndexCountersAtProc0Only, // input
                                   nowUnbLinkControl);                 // output
          }
        } // KAUST

        if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                  << ", level " << m_currLevel+LEVEL_REF_ID
                                  << ", step "  << m_currStep
                                  << ": in loop for assessing rejection rate"
                                  << ", about to generate assessment chain"
                                  << std::endl;
        }

        uqSequenceOfVectorsClass<P_V,P_M> nowChain(m_vectorSpace,
                                                   0,
                                                   m_options.m_prefix+"now_chain");
        double       nowRunTime    = 0.;
        unsigned int nowRejections = 0;

        // KAUST: all nodes should call here
        bool         savedTotallyMute           = currOptions->m_totallyMute; // HERE - ENHANCEMENT
        unsigned int savedRawChainSize          = currOptions->m_rawChainSize; // Ok to use rawChainSize
        bool         savedRawChainComputeStats  = currOptions->m_rawChainComputeStats;
        bool         savedFilteredChainGenerate = currOptions->m_filteredChainGenerate;
        unsigned int savedDrMaxNumExtraStages   = currOptions->m_drMaxNumExtraStages;
        unsigned int savedAmAdaptInterval       = currOptions->m_amAdaptInterval;

        currOptions->m_totallyMute           = true;
        currOptions->m_rawChainSize          = 0; // will be set inside generateXYZLinkedChains()
        currOptions->m_rawChainComputeStats  = false;
        currOptions->m_filteredChainGenerate = false;
        currOptions->m_drMaxNumExtraStages   = 0;
        currOptions->m_amAdaptInterval       = 0;

        // KAUST: all nodes in 'subComm' should call here, important
        if (useBalancedChains) {
          generateBalLinkedChains(*currOptions,       // input, only m_rawChainSize changes
                                  nowCovMatrix,       // input
                                  currRv,             // input
                                  nowBalLinkControl,  // input // Round Rock
                                  nowChain,           // output 
                                  nowRunTime,         // output
                                  nowRejections,      // output
                                  NULL,               // output
                                  NULL);              // output
        }
        else {
          generateUnbLinkedChains(*currOptions,       // input, only m_rawChainSize changes
                                  nowCovMatrix,       // input
                                  currRv,             // input
                                  nowUnbLinkControl,  // input // Round Rock
                                  indexOfFirstWeight, // input // Round Rock
                                  prevChain,          // input // Round Rock
                                  nowChain,           // output 
                                  nowRunTime,         // output
                                  nowRejections,      // output
                                  NULL,               // output
                                  NULL);              // output
        }

        // KAUST: all nodes should call here
        currOptions->m_totallyMute           = savedTotallyMute;
        currOptions->m_rawChainSize          = savedRawChainSize;
        currOptions->m_rawChainComputeStats  = savedRawChainComputeStats;
        currOptions->m_filteredChainGenerate = savedFilteredChainGenerate; // FIX ME
        currOptions->m_drMaxNumExtraStages   = savedDrMaxNumExtraStages;
        currOptions->m_amAdaptInterval       = savedAmAdaptInterval;

        for (unsigned int i = 0; i < nowBalLinkControl.balLinkedChains.size(); ++i) {
          UQ_FATAL_TEST_MACRO(nowBalLinkControl.balLinkedChains[i].initialPosition == NULL,
                              m_env.fullRank(),
                              "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                              "Initial position pointer in step 8 should not be NULL");
          delete nowBalLinkControl.balLinkedChains[i].initialPosition;
          nowBalLinkControl.balLinkedChains[i].initialPosition = NULL;
        }
        nowBalLinkControl.balLinkedChains.clear();

        if (m_env.inter0Rank() >= 0) { // KAUST
          // If only one cov matrix is used, then the rejection should be assessed among all inter0Comm nodes // KAUST3
          unsigned int nowUnifiedRejections = 0;
          int mpiRC = MPI_Allreduce((void *) &nowRejections, (void *) &nowUnifiedRejections, (int) 1, MPI_UNSIGNED, MPI_SUM, m_env.inter0Comm().Comm());
          UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                              m_env.fullRank(),
                              "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                              "failed MPI_Allreduce() for now rejections");

#if 0 // Round Rock 2009 12 29
          unsigned int tmpUnifiedNumSamples = 0;
          mpiRC = MPI_Allreduce((void *) &tmpSubNumSamples, (void *) &tmpUnifiedNumSamples, (int) 1, MPI_UNSIGNED, MPI_SUM, m_env.inter0Comm().Comm());
          UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                              m_env.fullRank(),
                              "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                              "failed MPI_Allreduce() for num samples in step 8");
#endif

          unsigned int tmpUnifiedNumSamples = originalSubNumSamples*m_env.inter0Comm().NumProc();
          nowRejectionRate = ((double) nowUnifiedRejections) / ((double) tmpUnifiedNumSamples);

          //bool aux1 = (nowRejectionRate == meanRejectionRate);
          bool aux2 = (nowRejectionRate >= currOptions->m_minRejectionRate)
                      &&
                      (nowRejectionRate <= currOptions->m_maxRejectionRate);
          testResult = aux2;

          // Make sure all nodes in 'inter0Comm' have the same value of 'testResult'
          uqMiscCheckForSameValueInAllNodes(testResult,
                                            0.,
                                            m_env.inter0Comm(),
                                            "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8(), step 8, testResult");
        } // if (m_env.inter0Rank() >= 0) { // KAUST

        // KAUST: all nodes in 'subComm' should have the same 'testResult'
        unsigned int tmpUint = (unsigned int) testResult;
        int mpiRC = MPI_Bcast((void *) &tmpUint, (int) 1, MPI_UNSIGNED, 0, m_env.subComm().Comm()); // Yes, 'subComm', important
        UQ_FATAL_TEST_MACRO(mpiRC != MPI_SUCCESS,
                            m_env.fullRank(),
                            "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8()",
                            "failed MPI_Bcast() for testResult");
        testResult = (bool) tmpUint;

        if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
          *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                  << ", level "              << m_currLevel+LEVEL_REF_ID
                                  << ", step "               << m_currStep
                                  << ": in loop for assessing rejection rate"
                                  << ", nowAttempt = "       << nowAttempt
                                  << ", beforeEta = "        << beforeEta
                                  << ", etas[0] = "          << etas[0]
                                  << ", nowEta = "           << nowEta
                                  << ", etas[1] = "          << etas[1]
                                  << ", minRejectionRate = " << currOptions->m_minRejectionRate
                                  << ", nowRejectionRate = " << nowRejectionRate
                                  << ", maxRejectionRate = " << currOptions->m_maxRejectionRate
                                  << std::endl;
        }
        nowAttempt++;

        if (m_env.inter0Rank() >= 0) { // KAUST
          // Make sure all nodes in 'inter0Comm' have the same value of 'nowEta'
          uqMiscCheckForSameValueInAllNodes(nowEta,
                                            1.e-16,
                                            m_env.inter0Comm(),
                                            "uqMLSamplingClass<P_V,P_M>::generateSequence_Step8(), step 8, testResult");
        }
      } while (testResult == false);
      currEta = nowEta;
      if (currEta != 1.) {
        unifiedCovMatrix *= currEta;
      }

      unsigned int quantity1 = weightSequence.unifiedSequenceSize(m_vectorSpace.numOfProcsForStorage() == 1);
      if ((m_env.subDisplayFile()) && (m_env.displayVerbosity() >= 0)) {
        *m_env.subDisplayFile() << "In uqMLSampling<P_V,P_M>::generateSequence_Step8()"
                                << ", level "                                  << m_currLevel+LEVEL_REF_ID
                                << ", step "                                   << m_currStep
                                << ": weightSequence.subSequenceSize() = "     << weightSequence.subSequenceSize()
                                << ", weightSequence.unifiedSequenceSize() = " << quantity1
                                << ", currEta = "                              << currEta
                                << ", assessed rejection rate = "              << nowRejectionRate
                                << std::endl;
      }

  return;
}

template <class P_V,class P_M>
void
uqMLSamplingClass<P_V,P_M>::generateSequence_Step9()
{
  return;
}
#endif // __UQ_MULTI_LEVEL_SAMPLING2_H__


