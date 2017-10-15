#ifndef TROPTER_IPOPTSOLVER_H
#define TROPTER_IPOPTSOLVER_H
// ----------------------------------------------------------------------------
// tropter: IPOPTSolver.h
// ----------------------------------------------------------------------------
// Copyright (c) 2017 tropter authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain a
// copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#include "OptimizationSolver.h"

namespace tropter {

/// @ingroup optimization
class IPOPTSolver : public OptimizationSolver {
public:
    // TODO this means the IPOPTSolver *would* get access to the Problem,
    // and we don't want that.
    IPOPTSolver(const AbstractOptimizationProblem& problem)
            : OptimizationSolver(problem) {}
    // TODO explain what happens if initial guess is omitted.
    // TODO cannot use temporary.
protected:
    double optimize_impl(Eigen::VectorXd& variables) const override;
private:
    class TNLP;
};


} // namespace tropter

#endif // TROPTER_IPOPTSOLVER_H