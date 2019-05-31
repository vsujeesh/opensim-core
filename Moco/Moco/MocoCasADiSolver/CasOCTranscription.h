#ifndef MOCO_CASOCTRANSCRIPTION_H
#define MOCO_CASOCTRANSCRIPTION_H
/* -------------------------------------------------------------------------- *
 * OpenSim Moco: CasOCTranscription.h                                         *
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

#include "CasOCSolver.h"

namespace CasOC {

/// This is the base class for transcription schemes that convert a
/// CasOC::Problem into a general nonlinear programming problem. If you are
/// creating a new derived class, make sure to override all virtual functions
/// and obey the settings that the user specified in the CasOC::Solver.
class Transcription {
public:
    Transcription(const Solver& solver, const Problem& problem)
            : m_solver(solver), m_problem(problem) {}
    virtual ~Transcription() = default;
    Iterate createInitialGuessFromBounds() const;
    /// Use the provided random number generator to generate an iterate.
    /// Random::Uniform is used if a generator is not provided. The generator
    /// should produce numbers with [-1, 1].
    Iterate createRandomIterateWithinBounds(
            const SimTK::Random* = nullptr) const;
    template <typename T>
    T createTimes(const T& initialTime, const T& finalTime) const {
        return (finalTime - initialTime) * m_grid + initialTime;
    }
    casadi::DM createQuadratureCoefficients() const {
        return createQuadratureCoefficientsImpl();
    }
    casadi::DM createKinematicConstraintIndices() const {
        casadi::DM kinConIndices = createKinematicConstraintIndicesImpl();
        const auto shape = kinConIndices.size();
        OPENSIM_THROW_IF(shape.first != 1 || shape.second != m_numGridPoints,
                OpenSim::Exception,
                OpenSim::format(
                        "createKinematicConstraintIndicesImpl() must return a "
                        "row vector of shape length [1, %i], but a matrix of "
                        "shape [%i, %i] was returned.",
                        m_numGridPoints, shape.first, shape.second));
        OPENSIM_THROW_IF(!SimTK::isNumericallyEqual(
                                 casadi::DM::sum2(kinConIndices).scalar(),
                                 m_numMeshPoints),
                OpenSim::Exception, "Internal error.");

        return kinConIndices;
    }

    Solution solve(const Iterate& guessOrig);

protected:
    /// This must be called in the constructor of derived classes so that
    /// overridden virtual methods are accessible to the base class. This
    /// implementation allows initialization to occur during construction,
    /// avoiding an extra call on the instantiated object.
    void createVariablesAndSetBounds(
            const casadi::DM& grid, int numDefectsPerGridPoint);

    /// We assume all functions depend on time and parameters.
    /// "inputs" is prepended by time and postpended (?) by parameters.
    casadi::MXVector evalOnTrajectory(const casadi::Function& pointFunction,
            const std::vector<Var>& inputs,
            const casadi::Matrix<casadi_int>& timeIndices) const;

    template <typename TRow, typename TColumn>
    void setVariableBounds(Var var, const TRow& rowIndices,
            const TColumn& columnIndices, const Bounds& bounds) {
        if (bounds.isSet()) {
            const auto& lower = bounds.lower;
            m_lowerBounds[var](rowIndices, columnIndices) = lower;
            const auto& upper = bounds.upper;
            m_upperBounds[var](rowIndices, columnIndices) = upper;
        } else {
            const auto inf = std::numeric_limits<double>::infinity();
            m_lowerBounds[var](rowIndices, columnIndices) = -inf;
            m_upperBounds[var](rowIndices, columnIndices) = inf;
        }
    }

    template <typename T>
    struct Constraints {
        T defects;
        T residuals;
        T kinematic;
        std::vector<T> path;
    };
    void printConstraintValues(const Iterate& it,
            const Constraints<casadi::DM>& constraints) const;

    const Solver& m_solver;
    const Problem& m_problem;
    int m_numGridPoints = -1;
    int m_numMeshPoints = -1;
    int m_numMeshIntervals = -1;
    int m_numPointsIgnoringConstraints = -1;
    int m_numDefectsPerGridPoint = -1;
    int m_numResiduals = -1;
    int m_numConstraints = -1;
    casadi::DM m_grid;
    casadi::MX m_times;
    casadi::MX m_duration;

private:
    VariablesMX m_vars;
    casadi::MX m_paramsTrajGrid;
    casadi::MX m_paramsTraj;
    casadi::MX m_paramsTrajIgnoringConstraints;
    VariablesDM m_lowerBounds;
    VariablesDM m_upperBounds;

    casadi::DM m_kinematicConstraintIndices;
    casadi::Matrix<casadi_int> m_gridIndices;
    casadi::Matrix<casadi_int> m_daeIndices;
    casadi::Matrix<casadi_int> m_daeIndicesIgnoringConstraints;

    casadi::MX m_xdot; // State derivatives.

    casadi::MX m_objective;
    Constraints<casadi::MX> m_constraints;
    Constraints<casadi::DM> m_constraintsLowerBounds;
    Constraints<casadi::DM> m_constraintsUpperBounds;

private:
    /// Override this function in your derived class to compute a vector of
    /// quadrature coeffecients (of length m_numGridPoints) required to set the
    /// the integral cost within transcribe().
    virtual casadi::DM createQuadratureCoefficientsImpl() const = 0;
    /// Override this function to specify the indicies in the grid where any
    /// existing kinematic constraints are to be enforced.
    /// @note The returned vector must be a row vector of length m_numGridPoints
    /// with nonzero values at the indices where kinematic constraints are
    /// enforced.
    virtual casadi::DM createKinematicConstraintIndicesImpl() const = 0;
    /// Override this function in your derived class set the defect, kinematic,
    /// and path constraint errors required for your transcription scheme.
    virtual void calcDefectsImpl(const casadi::MX& x, const casadi::MX& xdot,
            casadi::MX& defects) = 0;

    void transcribe();
    void setObjective();
    void calcDefects() {
        calcDefectsImpl(m_vars.at(states), m_xdot, m_constraints.defects);
    }

    /// Use this function to ensure you iterate through variables in the same
    /// order.
    template <typename T>
    static std::vector<Var> getSortedVarKeys(const Variables<T>& vars) {
        std::vector<Var> keys;
        for (const auto& kv : vars) { keys.push_back(kv.first); }
        std::sort(keys.begin(), keys.end());
        return keys;
    }
    /// Convert the map of variables into a column vector, for passing onto
    /// nlpsol(), etc.
    template <typename T>
    static T flattenVariables(const CasOC::Variables<T>& vars) {
        std::vector<T> stdvec;
        for (const auto& key : getSortedVarKeys(vars)) {
            stdvec.push_back(vars.at(key));
        }
        return T::veccat(stdvec);
    }
    /// Convert the 'x' column vector into separate variables.
    CasOC::VariablesDM expandVariables(const casadi::DM& x) const {
        CasOC::VariablesDM out;
        using casadi::Slice;
        casadi_int offset = 0;
        for (const auto& key : getSortedVarKeys(m_vars)) {
            const auto& value = m_vars.at(key);
            // Convert a portion of the column vector into a matrix.
            out[key] = casadi::DM::reshape(
                    x(Slice(offset, offset + value.numel())), value.rows(),
                    value.columns());
            offset += value.numel();
        }
        return out;
    }

    /// Flatten the constraints into a row vector, keeping constraints
    /// grouped together by time. Organizing the sparsity of the Jacobian
    /// this way might have benefits for sparse linear algebra.
    template <typename T>
    T flattenConstraints(const Constraints<T>& constraints) const {
        T flat = T(casadi::Sparsity::dense(m_numConstraints, 1));

        int iflat = 0;
        auto copyColumn = [&flat, &iflat](const T& matrix, int columnIndex) {
            using casadi::Slice;
            if (matrix.rows()) {
                flat(Slice(iflat, iflat + matrix.rows())) =
                        matrix(Slice(), columnIndex);
                iflat += matrix.rows();
            }
        };

        // Trapezidal sparsity pattern for mesh intervals 0, 1 and 2:
        //                   0    1    2    3
        //    path_0         x
        //    kinematic_0    x
        //    residual_0     x
        //    defect_0       x    x
        //    kinematic_1         x
        //    path_1              x
        //    residual_1          x
        //    defect_1            x    x
        //    kinematic_2              x
        //    path_2                   x
        //    residual_2               x
        //    kinematic_3                   x
        //    path_3                        x
        //    residual_3                    x

        // Hermite-Simpson sparsity pattern for mesh intervals 0, 1 and 2:
        //                   0    0.5    1    1.5    2    2.5    3
        //    path_0         x
        //    kinematic_0    x
        //    residual_0     x
        //    residual_0.5         x
        //    defect_0       x     x     x
        //    kinematic_1                x
        //    path_1                     x
        //    residual_1                 x
        //    residual_1.5                     x
        //    defect_1                   x     x     x
        //    kinematic_2                            x
        //    path_2                                 x
        //    residual_2                             x
        //    residual_2.5                                 x
        //    defect_2                               x     x     x
        //    kinematic_3                                        x
        //    path_3                                             x
        //    residual_3                                         x
        //                   0    0.5    1    1.5    2    2.5    3

        int igrid = 0;
        for (int imesh = 0; imesh < m_numMeshPoints; ++imesh) {
            copyColumn(constraints.kinematic, imesh);
            for (const auto& path : constraints.path) {
                copyColumn(path, imesh);
            }
            if (imesh < m_numMeshIntervals) {
                while (m_grid(igrid).scalar() < m_solver.getMesh()[imesh + 1]) {
                    copyColumn(constraints.residuals, igrid);
                    ++igrid;
                }
                copyColumn(constraints.defects, imesh);
            }
        }
        // The loop above does not handle the residual at the final grid point.
        copyColumn(constraints.residuals, m_numGridPoints - 1);

        OPENSIM_THROW_IF(iflat != m_numConstraints, OpenSim::Exception,
                "Internal error.");
        return flat;
    }

    // Expand constraints that have been flattened into a Constraints struct.
    template <typename T>
    Constraints<T> expandConstraints(const T& flat) const {
        using casadi::Sparsity;

        // Allocate memory.
        auto init = [](int numRows, int numColumns) {
            return T(casadi::Sparsity::dense(numRows, numColumns));
        };
        Constraints<T> out;
        out.defects = init(m_numDefectsPerGridPoint, m_numMeshPoints - 1);
        out.residuals = init(m_numResiduals, m_numGridPoints);
        out.kinematic = init(m_problem.getNumKinematicConstraintEquations(),
                m_numMeshPoints);
        out.path.resize(m_problem.getPathConstraintInfos().size());
        for (int ipc = 0; ipc < (int)m_constraints.path.size(); ++ipc) {
            const auto& info = m_problem.getPathConstraintInfos()[ipc];
            out.path[ipc] = init(info.size(), m_numMeshPoints);
        }

        int iflat = 0;
        auto copyColumn = [&flat, &iflat](T& matrix, int columnIndex) {
            using casadi::Slice;
            if (matrix.rows()) {
                matrix(Slice(), columnIndex) =
                        flat(Slice(iflat, iflat + matrix.rows()));
                iflat += matrix.rows();
            }
        };

        int igrid = 0;
        for (int imesh = 0; imesh < m_numMeshPoints; ++imesh) {
            copyColumn(out.kinematic, imesh);
            for (auto& path : out.path) {
                copyColumn(path, imesh);
            }
            if (imesh < m_numMeshIntervals) {
                while (m_grid(igrid).scalar() < m_solver.getMesh()[imesh + 1]) {
                    copyColumn(out.residuals, igrid);
                    ++igrid;
                }
                copyColumn(out.defects, imesh);
            }
        }
        // The loop above does not handle the residual at the final grid point.
        copyColumn(out.residuals, m_numGridPoints - 1);

        OPENSIM_THROW_IF(iflat != m_numConstraints, OpenSim::Exception,
                "Internal error.");
        return out;
    }
};

} // namespace CasOC

#endif // MOCO_CASOCTRANSCRIPTION_H
