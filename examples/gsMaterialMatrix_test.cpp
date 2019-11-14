/** @file gsExprIntegral_test.cpp

    @brief Testing integral computation using the expression evaluator

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): A. Mantzaflaris
*/

#include <gismo.h>

#include <gsAssembler/gsExprEvaluator.h>

using namespace gismo;

/*
    gsIntegrantZ has as input
        -
        -
        -

*/
template<class T>
class gsIntegrantZ : public gismo::gsFunction<T>
{
    protected:
        const typename gsFunction<T>::Ptr _fun;
        mutable gsMatrix<T> tmp;
        gsMatrix<T> _surfPts;
    public:
        /// Shared pointer for gsIntegrantZ
        typedef memory::shared_ptr< gsIntegrantZ > Ptr;

        /// Unique pointer for gsIntegrantZ
        typedef memory::unique_ptr< gsIntegrantZ > uPtr;

    // copy constructor
    explicit gsIntegrantZ(const gsIntegrantZ &other) : _fun(other._fun), _surfPts(other._surfPts) {}

    gsIntegrantZ(const gsFunction<T> & fun) : _fun(fun.clone()) { }

    GISMO_CLONE_FUNCTION(gsIntegrantZ)

    void setPoint(const gsMatrix<T>& surfPts) { _surfPts = surfPts; }

    gsMatrix<T> point() { return _surfPts; }

    short_t domainDim() const {return 1;}

    short_t targetDim() const {return _fun->targetDim();}

    void eval_into(const gsMatrix<T>& u, gsMatrix<T>& result) const
    {
        GISMO_ASSERT(u.rows()==1,
            "The number of rows for the 1D coordinate is not 1 but " <<u.rows()<<"!"  );
        GISMO_ASSERT(_fun->domainDim()==_surfPts.rows() + 1,
            "The domain dimensions do not match! fun.domainDim() != surfPts.rows() + 1! (" <<_fun->domainDim()<<"!="<<_surfPts.rows() + 1<<" )"  );
        GISMO_ASSERT(_surfPts.cols()==1,
            "Multiple ("<<_surfPts.cols()<<") parametric points given, accepts only 1... " <<"!"     );

        index_t m = _surfPts.rows();
        index_t N = u.cols();

        tmp.resize(m + 1,N);
        tmp.topRows(m)=_surfPts.replicate(1,N);
        tmp.bottomRows(1) = u;

        _fun->eval_into(tmp,result);
    }

    std::ostream &print(std::ostream &os) const
      { os << "gsIntegrantZ ( " << _fun << " )"; return os; };
};

template <class T>
class gsIntegrateZ : public gismo::gsFunction<T>
{
    protected:
        const typename gsFunction<T>::Ptr _fun;//, _t;
        mutable gsMatrix<T> tmp;
        gsMatrix<T> _surfPts;
        T _t;
    public:
        /// Shared pointer for gsIntegrateZ
        typedef memory::shared_ptr< gsIntegrateZ > Ptr;

        /// Unique pointer for gsIntegrateZ
        typedef memory::unique_ptr< gsIntegrateZ > uPtr;

    // copy constructor
    explicit gsIntegrateZ(const gsIntegrateZ &other)
    : _fun(other._fun), _t(other._t), _surfPts(other._surfPts) {}

    gsIntegrateZ(const gsFunction<T> & fun, T thickness)
    : _fun(fun.clone()), _t(thickness)
    {
        // gsConstantFunction _t(thickness, 3);
    }

    // gsIntegrateZ(const gsFunction<T> & fun, gsFunction<T> & thickFun)
    // : _fun(fun.clone()), _t(thickFun.clone()) { }

    GISMO_CLONE_FUNCTION(gsIntegrateZ)

    short_t domainDim() const {return 1;}

    short_t targetDim() const {return _fun->targetDim();}

    void setPoint(const gsMatrix<T>& surfPts) { _surfPts = surfPts; }

    // u are z-coordinates only!
    void eval_into(const gsMatrix<T>& u, gsMatrix<T>& result) const
    {
        // // Compute the thickness
        // _tmp.points = u;
        // _t.eval_into(_tmp.values[0], thickMat);

        result.resize(_fun->targetDim(),u.cols());

        // Define integrator for the z direction
        gsExprEvaluator<real_t> ev;

        // Define integration interval
        int k = 1; // interior knots
        int p = 1; // B-spline order

        // Make 1D domain with a basis
        gsKnotVector<> KV(-_t/2.0, _t/2.0, k, p+1);
        gsMultiBasis<> basis;
        basis.addBasis(gsBSplineBasis<>::make(KV));

        // Set integration elements along basis
        ev.setIntegrationElements(basis);

        // Define integrant variables
        typedef gsExprEvaluator<real_t>::variable    variable;
        variable    integrant = ev.getVariable(*_fun, 1);

        for (index_t i=0; i!=_fun->targetDim(); ++i)
            for (index_t j = 0; j != u.cols(); ++j)
            {
                //thickness integral for all components i.
                ev.integral(integrant.tr()[i]);
                result(i,j) = ev.value();
            }
    }

    std::ostream &print(std::ostream &os) const
      { os << "gsIntegrateZ ( " << _fun << " )"; return os; };
};

template <class T>
class gsIntegrate : public gismo::gsFunction<T>
{
    protected:
        const typename gsFunction<T>::Ptr _fun, _t;
        mutable gsMatrix<T> tmp, thickMat;
        gsMatrix<T> _surfPts;
        // T _t;
    public:
        /// Shared pointer for gsIntegrate
        typedef memory::shared_ptr< gsIntegrate > Ptr;

        /// Unique pointer for gsIntegrate
        typedef memory::unique_ptr< gsIntegrate > uPtr;

    // copy constructor
    explicit gsIntegrate(const gsIntegrate &other)
    : _fun(other._fun), _t(other._t), _surfPts(other._surfPts) {}

    // gsIntegrate(const gsFunction<T> & fun, T thickness)
    // : _fun(fun.clone()), _t(thickness)
    // {
    //     gsConstantFunction _t(thickness, 3);
    // }

    gsIntegrate(const gsFunction<T> & fun, gsFunction<T> & thickFun)
    : _fun(fun.clone()), _t(thickFun.clone()) { }

    GISMO_CLONE_FUNCTION(gsIntegrate)

    short_t domainDim() const {return 2;}

    short_t targetDim() const {return _fun->targetDim();}

    // u are xy-coordinates only; domainDim=2
    void eval_into(const gsMatrix<T>& u, gsMatrix<T>& result) const
    {
        // Compute the thickness
        _t->eval_into(u, thickMat);

        result.resize(_fun->targetDim(),u.cols());

        // Define integrator for the z direction
        gsExprEvaluator<real_t> ev;

        // Define integrant variables
        typedef gsExprEvaluator<real_t>::variable    variable;
        gsIntegrantZ integrant(*_fun);

        T tHalf;
        for (index_t i=0; i!=_fun->targetDim(); ++i)
            for (index_t j = 0; j != u.cols(); ++j)
            {
                // this part is quite sloppy since a multi-basis is created every iteration..
                tHalf = thickMat(0,j)/2.0;

                gsKnotVector<> KV(-tHalf, tHalf, 2, 2);
                gsMultiBasis<> basis;
                basis.addBasis(gsBSplineBasis<>::make(KV));

                // Set integration elements along basis
                ev.setIntegrationElements(basis);

                variable intfun = ev.getVariable(integrant, 1);

                // set new integration point
                integrant.setPoint(u.col(j));

                //thickness integral for all components i.
                // ev.eval(intfun.coord(i));
                ev.integral(intfun.tr()[i]);
                result(i,j) = ev.value();
                gsDebugVar(ev.value());
            }
    }

    std::ostream &print(std::ostream &os) const
      { os << "gsIntegrateZ ( " << _fun << " )"; return os; };
};


int main(int argc, char *argv[])
{
    //std::string fn("surfaces/sphere1.xml"); // todo: test
    std::string fn("planar/two_squares.xml");
    //std::string fn("planar/quarter_annulus_2p.xml");

    gsCmdLine cmd("Testing expression evaluator.");
    cmd.addString("g","geometry","File containing Geometry (.xml, .axl, .txt)", fn);
    try { cmd.getValues(argc,argv); } catch (int rv) { return rv; }

    gsMultiPatch<> mp;
    gsReadFile<>(fn, mp);
    mp.computeTopology();
    gsMultiBasis<> b(mp);
    b.uniformRefine(1);
    //b.degreeElevate();
    //b.basis(0).component(0).uniformRefine();

    // Construct a tensor-product point grid
    const gsMatrix<> param = mp.patch(0).parameterRange();
    gsGridIterator<real_t,CUBE> grid(param, 12);

    // Initiate the expression evaluator
    gsExprEvaluator<real_t> ev;

    // Set the parameter mesh as the integration mesh
    ev.setIntegrationElements(b);


    /*
        test gsIntegrantZ function
    */

    gsVector<> pt1D(1); pt1D.setConstant(0.25);
    gsVector<> pt2D(2); pt2D.setConstant(0.25);
    gsVector<> pt3D(3); pt3D.setConstant(0.25);

    gsMatrix<> points(2,11);
    points<<0,1,2,3,4,5,6,7,8,9,10,
            0,1,2,3,4,5,6,7,8,9,10;

    gsFunctionExpr<> fun("1*x","2*y","x*y*z^2",3);

    gsMatrix<> result;
    fun.eval_into(pt3D,result);
    gsInfo<<"Evaluation of the following function on point (x,y) = ("<<pt3D.at(0)<<","<<pt3D.at(1)<<") and z coordinate "<<pt3D.at(2)<<"\n";
    gsInfo<<fun<<"\n";
    gsInfo<<"result = "<<result.transpose()<<"\n";

    gsInfo<<"Evaluation of the following function on point (x,y) = ("<<pt2D.at(0)<<","<<pt2D.at(1)<<") and z coordinate "<<pt1D.at(0)<<"\n";
    gsInfo<<fun<<"\n";
    gsIntegrantZ fun2(fun);
    fun2.setPoint(pt2D); // if changes to be applied
    fun2.eval_into(pt1D,result);
    gsInfo<<"result = "<<result.transpose()<<"\n";

    pt2D.setConstant(0.1);
    gsInfo<<"Evaluation of the following function on point (x,y) = ("<<pt2D.at(0)<<","<<pt2D.at(1)<<") and z coordinate "<<pt1D.at(0)<<"\n";
    gsInfo<<fun<<"\n";
    fun2.setPoint(pt2D); // if changes to be applied
    fun2.eval_into(pt1D,result);
    gsInfo<<"result = "<<result.transpose()<<"\n";

    /*
        test gsIntegrateZ function
    */
    gsFunctionExpr<> fun3("1","x","x^2","x^3","x^4","x^5","x^6","x^7","x^8",1);

    real_t bound = 1.0;
    gsIntegrateZ<real_t> integrator(fun3,bound);
    integrator.eval_into(pt1D,result);

    gsInfo<<"Integration of the following function from "<<-bound/2.0<<" to "<<bound/2.0<<": \n";
    gsInfo<<fun3<<"\n";
    gsInfo<<"Result: "<<result.transpose()<<"\n";

    /*
        test gsIntegrateZ function
    */
    bound = 1.0;
    gsIntegrateZ integrator2(fun2,bound);
    integrator2.eval_into(pt1D,result);

    gsInfo<<"Integration of the third component of the following function from "<<-bound/2.0<<" to "<<bound/2.0<<" on point (x,y) = ("<<pt2D.at(0)<<","<<pt2D.at(1)<<") \n";
    gsInfo<<fun<<"\n";
    gsInfo<<"Result: "<<result.transpose()<<"\n";


    /*
        test gsIntegrate function
    */
    gsConstantFunction<> thickFun(bound,2);
    gsIntegrate integrate(fun,thickFun);
    integrate.eval_into(points,result);

    gsInfo<<"Integration of the third component of the following function from "<<-bound/2.0<<" to "<<bound/2.0<<"\n";
    gsInfo<<fun<<"\n";
    gsInfo<<"on points (x,y) = \n";
    gsInfo<<points.transpose()<<"\n";
    gsInfo<<"Result: \n"<<result.transpose()<<"\n";

/*

    // Define integrant variables
    typedef gsExprEvaluator<real_t>::element     element;
    typedef gsExprEvaluator<real_t>::geometryMap geometryMap;
    typedef gsExprEvaluator<real_t>::variable    variable;
    element     e = ev.getElement();
    geometryMap G = ev.getMap(mp);
    variable    u = ev.getVariable(a_, G);
    variable    v = ev.getVariable(b_);

    //------------- Evaluation on a point grid

    // Construct a tensor-product point grid
    const gsMatrix<> param = mp.patch(0).parameterRange();
    gsGridIterator<real_t,CUBE> grid(param, 12);
    gsInfo<< "* Jacobian determinant values on tensor-product grid:\n";
    ev.eval( jac(G), grid );
    gsInfo<< "  Result: "<< ev.allValues().transpose() <<"\n";

    gsInfo<< "* Derivative values on tensor-product grid:\n";
    ev.eval( grad(u) * jac(G), grid );
    gsInfo<< "  Result: "<< ev.allValues().transpose() <<"\n";

    //------------- Maximum and minimum value

    gsInfo<< "* The maximum value of [ meas(G) ]:\n";
    ev.max( meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The minimum value of [ meas(G) ]:\n";
    ev.min( meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    //------------- Geometric quantities

    gsInfo<< "* The area of the domain [ meas(G) ]:\n";
    ev.integral( meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The area of the domain, assuming codim=0 [ jac(G).det() ] (!) :\n";
    ev.integral( jac(G).det() );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The boundary area (eg. perimeter) of the domain [ nv(G).norm() ]:\n";
    // Note: meas(G) is different than nv(G).norm()
    ev.integralBdr( nv(G).norm() );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The area of the parameter domain [ 1 ] :\n";
    ev.integral( 1 );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    //------------- L2 Norm

    gsInfo<< "* The squared L2 norm [ u.sqr() * meas(G) ]:\n";
    ev.integral( u.sqr() * meas(G) );

    gsInfo<< "  Result: "<< ev.value() <<"\n";
    ev.calcSqrt();
    gsInfo<< "  sqrt  : "<< ev.value() <<"\n";

    gsInfo<< "* The squared L2 distance [ (u-v).sqNorm() * meas(G) ]:\n";
    ev.integral( (u-v).sqNorm() * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The squared L2 norm ||u-v+2*v*u|| [ (u-v+2*v*u).sqNorm() * meas(G) ]:\n";
    ev.integral( (u-v+2*v*u).sqNorm() * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The squared L2 distance on the boundary [ u.sqNorm() * nv(G).norm() ]:\n";
    ev.integralBdr( u.sqNorm() * nv(G).norm() );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    // gsInfo<< "* The cubed L3 distance [  ]:\n";
    // ev.integral( (u-v).abs().pow(3)*(u-v).abs().pow(3) * meas(G) );
    // gsInfo<< "  Result: "<< ev.value() <<"\n";

    //------------- H1 Norm

    gsInfo<< "* The squared H1 seminorm [ igrad(u,G).sqNorm() * meas(G) ]:\n";
    ev.integral( igrad(u,G).sqNorm() * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The squared H1 norm [ (igrad(u,G).sqNorm()+u.sqNorm() ) * meas(G) ]:\n";
    ev.integral( (igrad(u,G).sqNorm() + u.sqNorm() ) * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The squared H1 norm per element :\n";
    ev.integralElWise( (igrad(u,G).sqNorm() + u.sqNorm() ) * meas(G) );
    gsInfo<< "  Result (elwise sum): "<< ev.allValues().sum() <<"\n";
    gsInfo<< "  Result (global)    : "<< ev.value() <<"\n";
    ev.calcSqrt();
    gsInfo<< "  sqrt (elwise): "<< ev.allValues().transpose() <<"\n";
    gsInfo<< "  sqrt (global): "<< ev.value() <<"\n";

    //------------- H2 Norm

    gsInfo<< "* The squared H2 seminorm [ ihess(u,G).sqNorm() * meas(G) ]:\n";
    ev.integral( ihess(u,G).sqNorm() * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* The squared H2 norm [ ihess(u,G).sqNorm()+igrad(u,G).sqNorm()+u.sqNorm() ) * meas(G) ]:\n";
    ev.integral( ( ihess(u,G).sqNorm() + igrad(u,G).sqNorm() + u.sqNorm() ) * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    //------------- Error estimates

    gsInfo<< "* Poisson residual [ (ilapl(u,G) + v).sqr() * meas(G) ]:\n";
    // u is the trial solution
    // v is the rhs
    ev.integral((ilapl(u,G) + v).sqNorm() * meas(G) );
    gsInfo<< "  Result: "<< ev.value() <<"\n";

    gsInfo<< "* Poisson residual estimator [ e.diam().sqr()*(ilapl(u,G)+v).sqr()*meas(G) ]:\n";
    ev.integralElWise( e.diam().sqr() * (ilapl(u,G) + v).sqr() * meas(G) );
    gsInfo<< "  Result: "<< ev.allValues().transpose() <<"\n";
    gsInfo<< "  Global: "<< ev.value() <<"\n"; //== ev.allValues().sum()
    // todo: add boundary contributions and interface contributions
*/
    return EXIT_SUCCESS;
}