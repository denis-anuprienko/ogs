/**
 * \file
 * \author Norihiro Watanabe
 * \author Wenqing Wang
 * \date   2013-05-15, 2014-02
 * \brief  Interface tests of global matrix classes
 *
 * \copyright
 * Copyright (c) 2013, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include <gtest/gtest.h>

#include "MathLib/LinAlg/Dense/DenseMatrix.h"
#include "MathLib/LinAlg/Dense/GlobalDenseMatrix.h"
#include "MathLib/LinAlg/FinalizeMatrixAssembly.h"

#ifdef USE_LIS
#include "MathLib/LinAlg/Lis/LisMatrix.h"
#endif

#ifdef USE_PETSC
#include "MathLib/LinAlg/PETSc/PETScMatrix.h"
#endif

namespace
{

template <class T_MATRIX>
void checkGlobalMatrixInterface(T_MATRIX &m)
{
    ASSERT_EQ(10u, m.getNRows());
    ASSERT_EQ(10u, m.getNCols());
    ASSERT_EQ(0u,  m.getRangeBegin());
    ASSERT_EQ(10u, m.getRangeEnd());

    m.setValue(0, 0, 1.0);
    m.add(0, 0, 1.0);
    m.setZero();

    MathLib::DenseMatrix<double> local_m(2, 2, 1.0);
    std::vector<std::size_t> vec_pos(2);
    vec_pos[0] = 1;
    vec_pos[1] = 3;
    m.add(vec_pos, vec_pos, local_m);

    ASSERT_TRUE(finalizeMatrixAssembly(m));
}

#ifdef USE_PETSC // or MPI
template <class T_MATRIX, class T_VECTOR>
void checkGlobalMatrixInterfaceMPI(T_MATRIX &m, T_VECTOR &v)
{
    int msize;
    MPI_Comm_size(PETSC_COMM_WORLD, &msize);
    int mrank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &mrank);

    ASSERT_EQ(3u, msize);
    ASSERT_EQ(6u, m.getNRows());
    ASSERT_EQ(m.getRangeEnd()-m.getRangeBegin(), m.getNLocalRows());

    int gathered_cols;
    int local_cols = m.getNLocalColumns();
    MPI_Allreduce(&local_cols, &gathered_cols, 1, MPI_INT, MPI_SUM, PETSC_COMM_WORLD);
    ASSERT_EQ(6u, gathered_cols);

    // Add entries
    MathLib::DenseMatrix<double> loc_m(2, 2);
    loc_m(0, 0) = 1.;
    loc_m(0, 1) = 2.;
    loc_m(1, 0) = 3.;
    loc_m(1, 1) = 4.;

    std::vector<int> row_pos(2);
    std::vector<int> col_pos(2);
    row_pos[0] = 2 * mrank;
    row_pos[1] = 2 * mrank + 1;
    col_pos[0] = row_pos[0];
    col_pos[1] = row_pos[1];

    m.add(row_pos, col_pos, loc_m);

    MathLib::finalizeMatrixAssembly(m);

    // Multiply by a vector
    v = 1.;
    const bool deep_copy = false;
    T_VECTOR y(v, deep_copy);
    m.multi(v, y);

    ASSERT_EQ(sqrt(3*(3*3 + 7*7)), y.getNorm());

    // set a value
    m.set(2 * mrank, 2 * mrank, 5.0);
    MathLib::finalizeMatrixAssembly(m);
    // add a value
    m.add(2 * mrank+1, 2 * mrank+1, 5.0);
    MathLib::finalizeMatrixAssembly(m);
    m.multi(v, y);

    ASSERT_EQ(sqrt((3*7*7 + 3*12*12)), y.getNorm());
}
#endif // end of: ifdef USE_PETSC // or MPI

} // end namespace

TEST(Math, CheckInterface_GlobalDenseMatrix)
{
    MathLib::GlobalDenseMatrix<double> m(10, 10);
    checkGlobalMatrixInterface(m);
}

#ifdef USE_LIS
TEST(Math, CheckInterface_LisMatrix)
{
    MathLib::LisMatrix m(10);
    checkGlobalMatrixInterface(m);
}
#endif

#ifdef USE_PETSC // or MPI
TEST(Math, CheckInterface_PETScMatrix_Local_Size)
{
    MathLib::PETScMatrixOption opt;
    opt.d_nz = 2;
    opt.o_nz = 0;
    opt.is_global_size = false;
    opt.n_local_cols = 2;
    MathLib::PETScMatrix A(2, opt);

    const bool is_gloabal_size = false;
    MathLib::PETScVector x(2, is_gloabal_size);

    checkGlobalMatrixInterfaceMPI(A, x);
}

TEST(Math, CheckInterface_PETScMatrix_Global_Size)
{
    MathLib::PETScMatrixOption opt;
    opt.d_nz = 2;
    opt.o_nz = 0;
    MathLib::PETScMatrix A(6, opt);

    MathLib::PETScVector x(6);

    checkGlobalMatrixInterfaceMPI(A, x);
}

#endif // end of: ifdef USE_PETSC // or MPI

