#pragma once

#include "Eigen\Dense"

//#include "Dynamics.h"
//#include "Kinematics.h"

#define epis 1e-5

using namespace Eigen;


class Jacobian
{
public:
	Jacobian( int dof);
	~Jacobian();

	void calJ ( const VectorXd& q);
	MatrixXd getJ (){return Jmat;}
	MatrixXd getJd( const VectorXd& q, const VectorXd& qd); // Derivative of geometric jacobian w.r.t time
	MatrixXd DLSJacobInverse(VectorXd q);  //Singular value decomposition for Jacobian Inverse
	Vector3d *z, *w, *pd, *Oi;    //w- angualr velocity(omega)

	MatrixXd Jmat,Jmat_inv;  // Jacobian Matrix.

	Dynamics* dyn; //only to let Jacobion get "get_A(i)" result.

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

private:
	int dof;
	//MatrixXd Jmat,Jmat_inv;  // Jacobian Matrix.
	MatrixXd Jmatd; // derivative of Jmat
	MatrixXd Jmat_prev;

	Matrix3d *R;
	Matrix4d *T;
	//Vector3d *z, *w, *pd, *Oi;

	Matrix4d T_flange;
	Matrix3d R_flange;
	Matrix4d tempMat;
	Vector3d tempVec;
};


Jacobian::Jacobian(int dof)
{
	this->dof = dof;
	Jmat  = MatrixXd(6, dof);
	Jmat_inv = MatrixXd(6, dof);
	Jmatd = MatrixXd(6, dof);
	Jmat_prev = MatrixXd::Zero(6,dof);  //MatrixXd(6, dof);

	R  = new Matrix3d[dof + 1];
	T  = new Matrix4d[dof + 1];
	z  = new Vector3d[dof + 1];
	w  = new Vector3d[dof + 1];
	pd = new Vector3d[dof + 1];
	Oi = new Vector3d[dof + 1];

	R[0] = Matrix3d::Identity();
	T[0] = Matrix4d::Identity();
	w[0] = Vector3d::Zero();
	z[0] = Vector3d::Zero();
	pd[0]= Vector3d::Zero();
}
Jacobian::~Jacobian()
{
	delete[] R;
	delete[] T;
	delete[] z;
	delete[] w;
	delete[] pd;
	delete[] Oi;
}

void Jacobian::calJ(const VectorXd &q){
	
	// jacobian is given as J = [Jvi Jwi].transpose
	// Jvi = Zi-1 x (On-Oi-1), n = dof 
	// Jwi = Zi-1 = Ri-1*k, k=[0,0,1]^T

	Matrix4d tempMat;
	
	for(int i=1; i<= dof; i++){
		
		tempMat = dyn->get_A(i);

		R[i]    = R[i-1]*tempMat.block<3,3>(0,0);
		T[i]    = T[i-1]*tempMat;

		z[i]  = R[i].rightCols(1);
		Oi[i] = T[i].block<3,1>(0,3);
	}	
			
	for(int i = 1;i <= dof; i++){	
		Vector3d tempVec = z[i].cross( Oi[dof] - Oi[i] );
		Jmat.block<3,1>(0,i-1) = tempVec;
		Jmat.block<3,1>(3,i-1) = z[i];
	}

	for(int i = 0; i<6; i++){
		for(int j = 0;j < dof;j++){
			if(abs(Jmat(i,j)) < TINY_VALUE )
				Jmat(i,j) = 0.0;
		}
	}
}


//derivative of the jacobian dJ(q)/dq
MatrixXd Jacobian::getJd( const VectorXd& q, const VectorXd& qd)
{
	Vector3d temp1,temp3;
	Matrix4d temp;
      
	//Orientation-joint i to TCP, speed joint-i to TCP
	Vector3d O_endeffect2Jointi, pd_endeffector2JointiSpeed;
	
	/*Matrix4d T_flange;
	T_flange<< 1,0,0,0,0,-1,0,0,0,0,-1,0,0,0,0,1;
	Matrix3d R_flange;
	R_flange<< 1,0,0,0,-1,0,0,0,-1;*/

	for(int i= 1;i <=dof; i++){

		temp = dyn->get_A(i);

	/*	if(i==dof){
			
			T[i]   = T[i-1]*temp*T_flange;
			R[i]   = R[i-1]*temp.block<3,3>(0,0)*R_flange;
		}
		
		else{*/
		T[i]   *= /*T[i-1]**/temp;
		R[i]   *= /*R[i-1]**/temp.block<3,3>(0,0);
		//}
		z[i]   = R[i].rightCols(1);
		w[i]   = w[i-1] + qd(i-1)*z[i];
		
		Oi[i] = T[i].block<3,1>(0,3);
		pd[i] = pd[i-1] + R[i-1] * w[i-1].cross(Oi[i]-Oi[i-1]);
	}	
		
	for(int i=1; i<=dof; i++){

		//w[i] = w[i-1] + qd(i-1)*z[i];

		O_endeffect2Jointi = Oi[dof] - Oi[i];
		pd_endeffector2JointiSpeed = pd[dof] - pd[i];
				
		temp1 = (R[i-1] * w[i-1]).cross(z[i]);
		temp3 = temp1.cross(O_endeffectToJointi) + z[i].cross(pd_endeffectorJointiSpeed);
		
		Jmatd.block<3,1>(0,i-1) = temp3;
		Jmatd.block<3,1>(3,i-1) = temp1;
	}

	for(int i=0; i<6; i++){
		for(int j =0; j<dof; j++){

			if(abs(Jmatd(i,j)) < TINY_VALUE) 
				Jmatd(i,j) = 0.0;
		}
	}

	return Jmatd;
}
// inverse of jacobian for non-square matrix avoid numerical drifts
MatrixXd Jacobian::DLSJacobInverse(VectorXd q){

	//getJ(q);

	double k = 0.2;

	MatrixXd temp = MatrixXd::Identity(6,6);
	temp = (Jmat*Jmat.transpose() + k*k*temp).inverse();

	temp = Jmat.transpose()*temp;

	return temp;
}
