%newobject *::clone;

/* To recognize SimTK::RowVector in header files (TODO: move to simbody.i) */
typedef SimTK::RowVector_<double> RowVector;

%include <OpenSim/Moco/osimMocoDLL.h>

%include <OpenSim/Moco/About.h>

%include <OpenSim/Moco/Common/TableProcessor.h>


namespace OpenSim {
    %ignore ModelProcessor::setModel(std::unique_ptr<Model>);
}

%extend OpenSim::ModelProcessor {
    void setModel(Model* model) {
        $self->setModel(std::unique_ptr<Model>(model));
    }
};
%include <OpenSim/Moco/ModelProcessor.h>

namespace OpenSim {
    %ignore MocoGoal::IntegrandInput;
    %ignore MocoGoal::calcIntegrand;
    %ignore MocoGoal::GoalInput;
    %ignore MocoGoal::calcGoal;
}
%include <OpenSim/Moco/MocoGoal/MocoGoal.h>
%template(SetMocoWeight) OpenSim::Set<OpenSim::MocoWeight, OpenSim::Object>;
%include <OpenSim/Moco/MocoWeightSet.h>
%include <OpenSim/Moco/MocoGoal/MocoStateTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoMarkerTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoMarkerFinalGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoContactTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoControlGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoControlTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoInitialActivationGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoJointReactionGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoSumSquaredStateGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoOrientationTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoTranslationTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoAccelerationTrackingGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoOutputGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoPeriodicityGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoInitialForceEquilibriumGoal.h>
%include <OpenSim/Moco/MocoGoal/MocoInitialVelocityEquilibriumDGFGoal.h>


// %template(MocoBoundsVector) std::vector<OpenSim::MocoBounds>;

%include <OpenSim/Moco/MocoBounds.h>
%include <OpenSim/Moco/MocoVariableInfo.h>

// %template(MocoVariableInfoVector) std::vector<OpenSim::MocoVariableInfo>;

%ignore OpenSim::MocoMultibodyConstraint::getKinematicLevels;
%ignore OpenSim::MocoConstraintInfo::getBounds;
%ignore OpenSim::MocoConstraintInfo::setBounds;
%ignore OpenSim::MocoProblemRep::getMultiplierInfos;

%include <OpenSim/Moco/MocoConstraint.h>

%include <OpenSim/Moco/MocoControlBoundConstraint.h>
%include <OpenSim/Moco/MocoFrameDistanceConstraint.h>

// unique_ptr
// ----------
// https://stackoverflow.com/questions/27693812/how-to-handle-unique-ptrs-with-swig
namespace std {
%feature("novaluewrapper") unique_ptr;
template <typename Type>
struct unique_ptr {
    typedef Type* pointer;
    explicit unique_ptr( pointer Ptr );
    unique_ptr (unique_ptr&& Right);
    template<class Type2, Class Del2> unique_ptr( unique_ptr<Type2, Del2>&& Right );
    unique_ptr( const unique_ptr& Right) = delete;
    pointer operator-> () const;
    pointer release ();
    void reset (pointer __p=pointer());
    void swap (unique_ptr &__u);
    pointer get () const;
    operator bool () const;
    ~unique_ptr();
};
}

// https://github.com/swig/swig/blob/master/Lib/python/std_auto_ptr.i
#if SWIGPYTHON
%define moco_unique_ptr(TYPE)
    %template() std::unique_ptr<TYPE>;
    %newobject std::unique_ptr<TYPE>::release;
    %typemap(out) std::unique_ptr<TYPE> %{
        %set_output(SWIG_NewPointerObj($1.release(), $descriptor(TYPE *), SWIG_POINTER_OWN | %newpointer_flags));
    %}
%enddef
#endif

// https://github.com/swig/swig/blob/master/Lib/java/std_auto_ptr.i
#if SWIGJAVA
%define moco_unique_ptr(TYPE)
    %template() std::unique_ptr<TYPE>;
    %typemap(jni) std::unique_ptr<TYPE> "jlong"
    %typemap(jtype) std::unique_ptr<TYPE> "long"
    %typemap(jstype) std::unique_ptr<TYPE> "$typemap(jstype, TYPE)"
    %typemap(out) std::unique_ptr<TYPE> %{
        jlong lpp = 0;
        *(TYPE**) &lpp = $1.release();
        $result = lpp;
    %}
    %typemap(javaout) std::unique_ptr<TYPE> {
        long cPtr = $jnicall;
        return (cPtr == 0) ? null : new $typemap(jstype, TYPE)(cPtr, true);
    }
%enddef
#endif

moco_unique_ptr(OpenSim::MocoProblemRep);

%include <OpenSim/Moco/MocoProblemRep.h>

// MocoProblemRep() is not copyable, but by default, SWIG tries to make a copy
// when wrapping createRep().
%rename(createRep) OpenSim::MocoProblem::createRepHeap;

namespace OpenSim {
    %ignore ModelProcessor::setModel(std::unique_ptr<Model>);
    %ignore MocoPhase::setModel(Model);
    %ignore MocoPhase::setModel(std::unique_ptr<Model>);
    %ignore MocoProblem::setModel(Model);
    %ignore MocoProblem::setModel(std::unique_ptr<Model>);
}

%extend OpenSim::ModelProcessor {
    void setModel(Model* model) {
        $self->setModel(std::unique_ptr<Model>(model));
    }
};

%extend OpenSim::MocoPhase {
    void setModel(Model* model) {
        $self->setModel(std::unique_ptr<Model>(model));
    }
    void addParameter(MocoParameter* ptr) {
        $self->addParameter(std::unique_ptr<MocoParameter>(ptr));
    }
    void addGoal(MocoGoal* ptr) {
        $self->addGoal(std::unique_ptr<MocoGoal>(ptr));
    }
    void addPathConstraint(MocoPathConstraint* ptr) {
        $self->addPathConstraint(std::unique_ptr<MocoPathConstraint>(ptr));
    }
}

%extend OpenSim::MocoProblem {
    void setModel(Model* model) {
        $self->setModel(std::unique_ptr<Model>(model));
    }
    void addParameter(MocoParameter* ptr) {
        $self->addParameter(std::unique_ptr<MocoParameter>(ptr));
    }
    void addGoal(MocoGoal* ptr) {
        $self->addGoal(std::unique_ptr<MocoGoal>(ptr));
    }
    void addPathConstraint(MocoPathConstraint* ptr) {
        $self->addPathConstraint(std::unique_ptr<MocoPathConstraint>(ptr));
    }
}

%include <OpenSim/Moco/MocoProblem.h>
%include <OpenSim/Moco/MocoParameter.h>

// Workaround for SWIG not supporting inherited constructors.
%define EXPOSE_BOUNDS_CONSTRUCTORS_HELPER(NAME)
%extend OpenSim::NAME {
    NAME() { return new NAME(); }
    NAME(double value) { return new NAME(value); }
    NAME(double lower, double upper) { return new NAME(lower, upper); }
};
%enddef
EXPOSE_BOUNDS_CONSTRUCTORS_HELPER(MocoInitialBounds);
EXPOSE_BOUNDS_CONSTRUCTORS_HELPER(MocoFinalBounds);


// SWIG does not support initializer_list, but we can use Java arrays to
// achieve similar syntax in MATLAB.
%ignore OpenSim::MocoTrajectory::setTime(std::initializer_list<double>);
%ignore OpenSim::MocoTrajectory::setState(const std::string&,
        std::initializer_list<double>);
%ignore OpenSim::MocoTrajectory::setControl(const std::string&,
        std::initializer_list<double>);
%ignore OpenSim::MocoTrajectory::setMultiplier(const std::string&,
        std::initializer_list<double>);
%ignore OpenSim::MocoTrajectory::setDerivative(const std::string&,
        std::initializer_list<double>);

%include <OpenSim/Moco/MocoTrajectory.h>

%include <OpenSim/Moco/MocoSolver.h>
%include <OpenSim/Moco/MocoDirectCollocationSolver.h>


namespace OpenSim {
    %ignore MocoTropterSolver::MocoTropterSolver(const MocoProblem&);
}
%include <OpenSim/Moco/MocoTropterSolver.h>
%include <OpenSim/Moco/MocoCasADiSolver/MocoCasADiSolver.h>
%include <OpenSim/Moco/MocoStudy.h>
%include <OpenSim/Moco/MocoStudyFactory.h>

%include <OpenSim/Moco/MocoTool.h>
%include <OpenSim/Moco/MocoInverse.h>
%include <OpenSim/Moco/MocoTrack.h>

%include <OpenSim/Moco/Components/DeGrooteFregly2016Muscle.h>
moco_unique_ptr(OpenSim::PositionMotion);
%include <OpenSim/Moco/Components/PositionMotion.h>

%include <OpenSim/Moco/MocoUtilities.h>
%template(analyze) OpenSim::analyze<double>;
%template(analyzeVec3) OpenSim::analyze<SimTK::Vec3>;
%template(analyzeSpatialVec) OpenSim::analyze<SimTK::SpatialVec>;

%include <OpenSim/Moco/Components/MultivariatePolynomialFunction.h>

%include <OpenSim/Moco/ModelOperators.h>