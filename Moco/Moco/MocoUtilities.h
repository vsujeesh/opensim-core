#ifndef MOCO_MOCOUTILITIES_H
#define MOCO_MOCOUTILITIES_H
/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoUtilities.h                                              *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2017 Stanford University and the Authors                     *
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

#include "osimMocoDLL.h"
#include "MocoIterate.h"

#include <set>
#include <stack>
#include <regex>

#include <OpenSim/Common/GCVSplineSet.h>
#include <OpenSim/Common/PiecewiseLinearFunction.h>
#include <OpenSim/Common/Storage.h>
#include <Simulation/Model/Model.h>
#include <Simulation/StatesTrajectory.h>
#include <Common/Reporter.h>

namespace OpenSim {

class StatesTrajectory;
class Model;
class MocoIterate;
class MocoProblem;

/// Since Moco does not require C++14 (which contains std::make_unique()),
/// here is an implementation of make_unique().
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/// Get a string with the current date and time formatted using the ISO standard
/// extended datetime format (%Y-%m-%dT%X))
OSIMMOCO_API std::string getFormattedDateTime();

/// Determine if `string` starts with the substring `start`.
/// https://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
inline bool startsWith(const std::string& string, const std::string& start) {
    if (string.length() >= start.length()) {
        return string.compare(0, start.length(), start) == 0;
    }
    return false;
}

/// Determine if `string` ends with the substring `ending`.
/// https://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
inline bool endsWith(const std::string& string, const std::string& ending) {
    if (string.length() >= ending.length()) {
        return string.compare(string.length() - ending.length(),
                       ending.length(), ending) == 0;
    }
    return false;
}

/// @name Filling in a string with variables.
/// @{

#ifndef SWIG
/// Return type for make_printable()
template <typename T>
struct make_printable_return {
    typedef T type;
};
/// Convert to types that can be printed with sprintf() (vsnprintf()).
/// The generic template does not alter the type.
template <typename T>
inline typename make_printable_return<T>::type make_printable(const T& x) {
    return x;
}

/// Specialization for std::string.
template <>
struct make_printable_return<std::string> {
    typedef const char* type;
};
/// Specialization for std::string.
template <>
inline typename make_printable_return<std::string>::type make_printable(
        const std::string& x) {
    return x.c_str();
}

/// Format a char array using (C interface; mainly for internal use).
OSIMMOCO_API std::string format_c(const char*, ...);

/// Format a string in the style of sprintf. For example, the code
/// `format("%s %d and %d yields %d", "adding", 2, 2, 4)` will produce
/// "adding 2 and 2 yields 4".
template <typename... Types>
std::string format(const std::string& formatString, Types... args) {
    return format_c(formatString.c_str(), make_printable(args)...);
}

/// Print a formatted string to std::cout. A newline is not included, but the
/// stream is flushed.
template <typename... Types>
void printMessage(const std::string& formatString, Types... args) {
    std::cout << format(formatString, args...);
    std::cout.flush();
}

#endif // SWIG

/// @}

/// Create a SimTK::Vector with the provided length whose elements are
/// linearly spaced between start and end.
OSIMMOCO_API
SimTK::Vector createVectorLinspace(int length, double start, double end);

#ifndef SWIG
/// Create a SimTK::Vector using modern C++ syntax.
OSIMMOCO_API
SimTK::Vector createVector(std::initializer_list<SimTK::Real> elements);
#endif

/// Linearly interpolate y(x) at new values of x. The optional 'ignoreNaNs'
/// argument will ignore any NaN values contained in the input vectors and
/// create the interpolant from the non-NaN values only. Note that this option
/// does not necessarily prevent NaN values from being returned in 'newX', which
/// will have NaN for any values of newX outside of the range of x.
OSIMMOCO_API
SimTK::Vector interpolate(const SimTK::Vector& x, const SimTK::Vector& y,
        const SimTK::Vector& newX, const bool ignoreNaNs = false);

#ifndef SWIG
template <typename FunctionType>
std::unique_ptr<FunctionSet> createFunctionSet(const TimeSeriesTable& table) {
    auto set = make_unique<FunctionSet>();
    const auto& time = table.getIndependentColumn();
    const auto numRows = (int)table.getNumRows();
    for (int icol = 0; icol < (int)table.getNumColumns(); ++icol) {
        const double* y =
                table.getDependentColumnAtIndex(icol).getContiguousScalarData();
        set->adoptAndAppend(new FunctionType(numRows, time.data(), y));
    }
    return set;
}

template <>
inline std::unique_ptr<FunctionSet> createFunctionSet<GCVSpline>(
        const TimeSeriesTable& table) {
    const auto& time = table.getIndependentColumn();
    return std::unique_ptr<GCVSplineSet>(new GCVSplineSet(table,
            std::vector<std::string>{}, std::min((int)time.size() - 1, 5)));
}
#endif // SWIG

/// Resample (interpolate) the table at the provided times. In general, a
/// 5th-order GCVSpline is used as the interpolant; a lower order is used if the
/// table has too few points for a 5th-order spline. Alternatively, you can
/// provide a different function type as a template argument (e.g.,
/// PiecewiseLinearFunction).
/// @throws Exception if new times are
/// not within existing initial and final times, if the new times are
/// decreasing, or if getNumTimes() < 2.
template <typename TimeVector, typename FunctionType = GCVSpline>
TimeSeriesTable resample(const TimeSeriesTable& in, const TimeVector& newTime) {

    const auto& time = in.getIndependentColumn();

    OPENSIM_THROW_IF(newTime.size() < 2, Exception,
            "Cannot resample if number of times is 0 or 1.");
    OPENSIM_THROW_IF(newTime[0] < time[0], Exception,
            format("New initial time (%f) cannot be less than existing "
                   "initial time (%f)",
                    newTime[0], time[0]));
    OPENSIM_THROW_IF(newTime[newTime.size() - 1] > time[time.size() - 1],
            Exception,
            format("New final time (%f) cannot be greater than existing final "
                   "time (%f)",
                    newTime[newTime.size() - 1], time[time.size() - 1]));
    for (int itime = 1; itime < (int)newTime.size(); ++itime) {
        OPENSIM_THROW_IF(newTime[itime] < newTime[itime - 1], Exception,
                format("New times must be non-decreasing, but "
                       "time[%i] < time[%i] (%f < %f).",
                        itime, itime - 1, newTime[itime], newTime[itime - 1]));
    }

    // Copy over metadata.
    TimeSeriesTable out = in;
    for (int irow = (int)out.getNumRows() - 1; irow >= 0; --irow) {
        out.removeRowAtIndex(irow);
    }

    std::unique_ptr<FunctionSet> functions =
            createFunctionSet<FunctionType>(in);
    SimTK::Vector curTime(1);
    SimTK::RowVector row(functions->getSize());
    for (int itime = 0; itime < (int)newTime.size(); ++itime) {
        curTime[0] = newTime[itime];
        for (int icol = 0; icol < functions->getSize(); ++icol) {
            row(icol) = functions->get(icol).calcValue(curTime);
        }
        // Not efficient!
        out.appendRow(curTime[0], row);
    }
    return out;
}

/// Create a Storage from a TimeSeriesTable. Metadata from the
/// TimeSeriesTable is *not* copied to the Storage.
/// You should use TimeSeriesTable if possible, as support for Storage may be
/// reduced in future versions of OpenSim. However, Storage supports some
/// operations not supported by TimeSeriesTable (e.g., filtering, resampling).
// TODO move to the Storage class.
OSIMMOCO_API Storage convertTableToStorage(const TimeSeriesTable&);

/// Lowpass filter the data in a TimeSeriesTable at a provided cutoff frequency.
/// The table is converted to a Storage object to use the lowpassIIR() method
/// to filter, and then converted back to TimeSeriesTable.
OSIMMOCO_API TimeSeriesTable filterLowpass(
        const TimeSeriesTable& table, double cutoffFreq, bool padData = false);

/// Read in a table of type TimeSeriesTable_<T> from file, where T is the type
/// of the elements contained in the table's columns. The `filepath` argument
/// should refer to a STO or CSV file (or other file types for which there is a
/// FileAdapter). This function assumes that only one table is contained in the
/// file, and will throw an exception otherwise.
template <typename T>
TimeSeriesTable_<T> readTableFromFileT(const std::string& filepath) {
    auto tablesFromFile = FileAdapter::readFile(filepath);
    // There should only be one table.
    OPENSIM_THROW_IF(tablesFromFile.size() != 1, Exception,
            format("Expected file '%s' to contain 1 table, but "
                   "it contains %i tables.",
                    filepath, tablesFromFile.size()));
    // Get the first table.
    auto* firstTable = dynamic_cast<TimeSeriesTable_<T>*>(
            tablesFromFile.begin()->second.get());
    OPENSIM_THROW_IF(!firstTable, Exception,
            "Expected file to contain a TimeSeriesTable_<T> where T is "
            "the type specified in the template argument, but it contains a "
            "different type of table.");

    return *firstTable;
}

/// Read in a TimeSeriesTable from file containing scalar elements. The
/// `filepath` argument should refer to a STO or CSV file (or other file types
/// for which there is a FileAdapter). This function assumes that only one table
/// is contained in the file, and will throw an exception otherwise.
OSIMMOCO_API inline TimeSeriesTable readTableFromFile(
        const std::string& filepath) {
    return readTableFromFileT<double>(filepath);
}

/// Write a single TimeSeriesTable to a file, using the FileAdapter associated
/// with the provided file extension.
OSIMMOCO_API void writeTableToFile(const TimeSeriesTable&, const std::string&);

/// Play back a motion (from the Storage) in the simbody-visuailzer. The Storage
/// should contain all generalized coordinates. The visualizer window allows the
/// user to control playback speed.
/// This function blocks until the user exits the simbody-visualizer window.
// TODO handle degrees.
OSIMMOCO_API void visualize(Model, Storage);

/// This function is the same as visualize(Model, Storage), except that
/// the states are provided in a TimeSeriesTable.
OSIMMOCO_API void visualize(Model, TimeSeriesTable);

/// Calculate the requested outputs using the model in the problem and the
/// states and controls in the MocoIterate.
/// The output paths can be regular expressions. For example,
/// ".*activation" gives the activation of all muscles.
/// Constraints are not enforced but prescribed motion (e.g.,
/// PositionMotion) is.
/// The output paths must correspond to outputs that match the type provided in
/// the template argument, otherwise they are not included in the report.
/// @note Parameters in the MocoIterate are **not** applied to the model.
template <typename T>
TimeSeriesTable_<T> analyze(Model model, const MocoIterate& iterate,
        std::vector<std::string> outputPaths) {

    // Initialize the system so we can access the outputs.
    model.initSystem();
    // Create the reporter object to which we'll add the output data to create
    // the report.
    auto* reporter = new TableReporter_<T>();
    // Loop through all the outputs for all components in the model, and if
    // the output path matches one provided in the argument and the output type
    // agrees with the template argument type, add it to the report.
    for (const auto& comp : model.getComponentList()) {
        for (const auto& outputName : comp.getOutputNames()) {
            const auto& output = comp.getOutput(outputName);
            auto thisOutputPath = output.getPathName();
            for (const auto& outputPathArg : outputPaths) {
                if (std::regex_match(thisOutputPath, 
                        std::regex(outputPathArg))) {
                    // Make sure the output type agrees with the template.
                    if (dynamic_cast<const Output<T>*>(&output)) {
                        reporter->addToReport(output);
                    } else {
                        std::cout << format("Warning: ignoring output %s of "
                                            "type %s.", output.getPathName(), 
                                            output.getTypeName())
                                  << std::endl;
                    }
                }
            }
       
        }
    }
    model.addComponent(reporter);
    model.initSystem();

    // Get states trajectory.
    Storage storage = iterate.exportToStatesStorage();
    auto statesTraj = StatesTrajectory::createFromStatesStorage(model, storage);

    // Loop through the states trajectory to create the report.
    for (int i = 0; i < (int)statesTraj.getSize(); ++i) {
        // Get the current state.
        auto state = statesTraj[i];

        // Enforce any SimTK::Motion's included in the model.
        model.getSystem().prescribe(state);

        // Create a SimTK::Vector of the control values for the current state.
        SimTK::RowVector controlsRow =
            iterate.getControlsTrajectory().row(i);
        SimTK::Vector controls(controlsRow.size(),
            controlsRow.getContiguousScalarData(), true);

        // Set the controls on the state object.
        model.realizeVelocity(state);
        model.setControls(state, controls);

        // Generate report results for the current state.
        model.realizeReport(state);
    }

    return reporter->getTable();
}

/// Given a MocoIterate and the associated OpenSim model, return the model with
/// a prescribed controller appended that will compute the control values from
/// the MocoSolution. This can be useful when computing state-dependent model
/// quantities that require realization to the Dynamics stage or later.
/// The function used to fit the controls can either be GCVSpline or
/// PiecewiseLinearFunction.
OSIMMOCO_API void prescribeControlsToModel(const MocoIterate& iterate,
        Model& model, std::string functionType = "GCVSpline");

/// Use the controls and initial state in the provided iterate to simulate the
/// model using an ODE time stepping integrator (OpenSim::Manager), and return
/// the resulting states and controls. We return a MocoIterate (rather than a
/// StatesTrajectory) to facilitate comparing optimal control solutions with
/// time stepping. Use integratorAccuracy to override the default setting.
OSIMMOCO_API MocoIterate simulateIterateWithTimeStepping(
        const MocoIterate& iterate, Model model,
        double integratorAccuracy = -1);

/// The map provides the index of each state variable in
/// SimTK::State::getY() from its each state variable path string.
/// Empty slots in Y (e.g., for quaternions) are ignored.
OSIMMOCO_API
std::vector<std::string> createStateVariableNamesInSystemOrder(
        const Model& model);

#ifndef SWIG
/// Same as above, but you can obtain a map from the returned state variable
/// names to the index in SimTK::State::getY() that accounts for empty slots
/// in Y.
OSIMMOCO_API
std::vector<std::string> createStateVariableNamesInSystemOrder(
        const Model& model, std::unordered_map<int, int>& yIndexMap);

/// The map provides the index of each state variable in
/// SimTK::State::getY() from its state variable path string.
OSIMMOCO_API
std::unordered_map<std::string, int> createSystemYIndexMap(const Model& model);
#endif

/// Create a vector of control names based on the actuators in the model for
/// which appliesForce == True. For actuators with one control (e.g.
/// ScalarActuator) the control name is simply the actuator name. For actuators
/// with multiple controls, each control name is the actuator name appended by
/// the control index (e.g. "/actuator_0"); modelControlIndices has length equal
/// to the number of controls associated with actuators that apply a force
/// (appliesForce == True). Its elements are the indices of the controls in the
/// Model::updControls() that are associated with actuators that apply a force.
OSIMMOCO_API
std::vector<std::string> createControlNamesFromModel(
        const Model& model, std::vector<int>& modelControlIndices);
//// Same as above, but when there is no mapping to the modelControlIndices.
OSIMMOCO_API
std::vector<std::string> createControlNamesFromModel(const Model& model);
/// The map provides the index of each control variable in the SimTK::Vector
/// return by OpenSim::Model::getControls() from its control name.
OSIMMOCO_API
std::unordered_map<std::string, int> createSystemControlIndexMap(
        const Model& model);

/// Throws an exception if the order of the controls in the model is not the
/// same as the order of the actuators in the model.
OSIMMOCO_API void checkOrderSystemControls(const Model& model);

/// Throws an exception if the same label appears twice in the list of labels.
/// The argument copies the provided labels since we need to sort them to check
/// for redundancies.
OSIMMOCO_API void checkRedundantLabels(std::vector<std::string> labels);

/// Throw an exception if the property's value is not in the provided set.
/// We assume that `p` is a single-value property.
template <typename T>
void checkPropertyInSet(
        const Object& obj, const Property<T>& p, const std::set<T>& set) {
    const auto& value = p.getValue();
    if (set.find(value) == set.end()) {
        std::stringstream msg;
        msg << "Property '" << p.getName() << "' (in ";
        if (!obj.getName().empty()) {
            msg << "object '" << obj.getName() << "' of type ";
        }
        msg << obj.getConcreteClassName() << ") has invalid value " << value
            << "; expected one of the following:";
        std::string separator(" ");
        for (const auto& s : set) {
            msg << separator << " " << s;
            if (separator.empty()) separator = ", ";
        }
        msg << ".";
        OPENSIM_THROW(Exception, msg.str());
    }
}

/// Throw an exception if the property's value is not positive.
/// We assume that `p` is a single-value property.
template <typename T>
void checkPropertyIsPositive(const Object& obj, const Property<T>& p) {
    const auto& value = p.getValue();
    if (value <= 0) {
        std::stringstream msg;
        msg << "Property '" << p.getName() << "' (in ";
        if (!obj.getName().empty()) {
            msg << "object '" << obj.getName() << "' of type ";
        }
        msg << obj.getConcreteClassName() << ") must be positive, but is "
            << value << ".";
        OPENSIM_THROW(Exception, msg.str());
    }
}

/// Throw an exception if the property's value is neither in the provided
/// range nor in the provided set.
/// We assume that `p` is a single-value property.
template <typename T>
void checkPropertyInRangeOrSet(const Object& obj, const Property<T>& p,
        const T& lower, const T& upper, const std::set<T>& set) {
    const auto& value = p.getValue();
    if ((value < lower || value > upper) && set.find(value) == set.end()) {
        std::stringstream msg;
        msg << "Property '" << p.getName() << "' (in ";
        if (!obj.getName().empty()) {
            msg << "object '" << obj.getName() << "' of type ";
        }
        msg << obj.getConcreteClassName() << ") has invalid value " << value
            << "; expected value to be in range " << lower << "," << upper
            << ", or one of the following:";
        std::string separator("");
        for (const auto& s : set) {
            msg << " " << separator << s;
            if (separator.empty()) separator = ",";
        }
        msg << ".";
        OPENSIM_THROW(Exception, msg.str());
    }
}

/// Record and report elapsed real time ("clock" or "wall" time) in seconds.
class Stopwatch {
public:
    /// This stores the start time as the current time.
    Stopwatch() { reset(); }
    /// Reset the start time to the current time.
    void reset() { m_startTime = SimTK::realTimeInNs(); }
    /// Return the amount of time that has elapsed since this object was
    /// constructed or since reset() has been called.
    double getElapsedTime() const {
        return SimTK::realTime() - SimTK::nsToSec(m_startTime);
    }
    /// Get elapsed time in nanoseconds. See SimTK::realTimeInNs() for more
    /// information.
    long long getElapsedTimeInNs() const {
        return SimTK::realTimeInNs() - m_startTime;
    }
    /// This provides the elapsed time as a formatted string (using format()).
    std::string getElapsedTimeFormatted() const {
        return formatNs(getElapsedTimeInNs());
    }
    /// Format the provided elapsed time in nanoseconds into a string.
    /// The time may be converted into seconds, milliseconds, or microseconds.
    /// Additionally, if the time is greater or equal to 60 seconds, the time in
    /// hours and/or minutes is also added to the string.
    /// Usually, you can call getElapsedTimeFormatted() instead of calling this
    /// function directly. If you call this function directly, use
    /// getElapsedTimeInNs() to get a time in nanoseconds (rather than
    /// getElapsedTime()).
    static std::string formatNs(const long long& nanoseconds) {
        std::stringstream ss;
        double seconds = SimTK::nsToSec(nanoseconds);
        int secRounded = (int)std::round(seconds);
        if (seconds > 1)
            ss << secRounded << " second(s)";
        else if (nanoseconds >= 1000000)
            ss << nanoseconds / 1000000 << " millisecond(s)";
        else if (nanoseconds >= 1000)
            ss << nanoseconds / 1000 << " microsecond(s)";
        else
            ss << nanoseconds << " nanosecond(s)";
        int minutes = secRounded / 60;
        int hours = minutes / 60;
        if (minutes || hours) {
            ss << " (";
            if (hours) {
                ss << hours << " hour(s), ";
                ss << minutes % 60 << " minute(s), ";
                ss << secRounded % 60 << " second(s)";
            } else {
                ss << minutes % 60 << " minute(s), ";
                ss << secRounded % 60 << " second(s)";
            }
            ss << ")";
        }
        return ss.str();
    }

private:
    long long m_startTime;
};

/// This obtains the value of the OPENSIM_MOCO_PARALLEL environment variable.
/// The value has the following meanings:
/// - 0: run in series (not parallel).
/// - 1: run in parallel using all cores.
/// - greater than 1: run in parallel with this number of threads.
/// If the environment variable is not set, this function returns -1.
///
/// This variable does not indicate which calculations are parallelized
/// or how the parallelization is achieved. Moco may even ignore or override
/// the setting from the environment variable. See documentation elsewhere
/// (e.g., from a specific MocoSolver) for more information.
OSIMMOCO_API int getMocoParallelEnvironmentVariable();

/// This class lets you store objects of a single type for reuse by multiple
/// threads, ensuring threadsafe access to each of those objects.
// TODO: Find a way to always give the same thread the same object.
template <typename T>
class ThreadsafeJar {
public:
    /// Request an object for your exclusive use on your thread. This function
    /// blocks the thread until an object is available. Make sure to return
    /// (leave()) the object when you're done!
    std::unique_ptr<T> take() {
        // Only one thread can lock the mutex at a time, so only one thread
        // at a time can be in any of the functions of this class.
        std::unique_lock<std::mutex> lock(m_mutex);
        // Block this thread until the condition variable is woken up
        // (by a notify_...()) and the lambda function returns true.
        m_inventoryMonitor.wait(lock, [this] { return m_entries.size() > 0; });
        std::unique_ptr<T> top = std::move(m_entries.top());
        m_entries.pop();
        return top;
    }
    /// Add or return an object so that another thread can use it. You will need
    /// to std::move() the entry, ensuring that you will no longer have access
    /// to the entry in your code (the pointer will now be null).
    void leave(std::unique_ptr<T> entry) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_entries.push(std::move(entry));
        lock.unlock();
        m_inventoryMonitor.notify_one();
    }
    /// Obtain the number of entries that can be taken.
    int size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (int)m_entries.size();
    }

private:
    std::stack<std::unique_ptr<T>> m_entries;
    mutable std::mutex m_mutex;
    std::condition_variable m_inventoryMonitor;
};

} // namespace OpenSim

#endif // MOCO_MOCOUTILITIES_H