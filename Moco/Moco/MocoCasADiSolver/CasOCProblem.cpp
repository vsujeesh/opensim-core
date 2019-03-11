/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoCasOCProblem.cpp                                         *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2018 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "CasOCProblem.h"

#include "../MocoUtilities.h"
#include "CasOCTranscription.h"
#include "CasOCTrapezoidal.h"

// Shhh...we shouldn't depend on these but MocoIterate has a handy resample()
// function.
#include "../MocoIterate.h"
#include "MocoCasADiBridge.h"
extern template class OpenSim::MocoCasADiMultibodySystem<false>;
extern template class OpenSim::MocoCasADiMultibodySystem<true>;
extern template class OpenSim::MocoCasADiMultibodySystemImplicit<false>;
extern template class OpenSim::MocoCasADiMultibodySystemImplicit<true>;

using OpenSim::Exception;
using OpenSim::format;

namespace CasOC {

Iterate Iterate::resample(const casadi::DM& newTimes) const {
    auto mocoIt = OpenSim::convertToMocoIterate(*this);
    auto simtkNewTimes = OpenSim::convertToSimTKVector(newTimes);
    mocoIt.resample(simtkNewTimes);
    return OpenSim::convertToCasOCIterate(mocoIt);
}

} // namespace CasOC