#ifndef LAB_H_
#define LAB_H_

int umfpack_matrix_solver (int n,
         int Ap[],
         int Aip[],
         double Ax[],
         double b[],
         double x[]);

#endif
