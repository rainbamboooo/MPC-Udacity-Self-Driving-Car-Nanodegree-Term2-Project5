#include "MPC.h"
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp> 
#include "Eigen-3.3/Eigen/Core"

using CppAD::AD;

// TODO: Set the timestep length and duration
size_t N = 25;
double dt = 0.05;

// This value assumes the model presented in the classroom is used.
//
// It was obtained by measuring the radius formed by running the vehicle in the
// simulator around in a circle with a constant steering angle and velocity on a
// flat terrain.
//
// Lf was tuned until the the radius formed by the simulating the model
// presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
const double Lf = 2.67;

const int num_states = 6;
const int num_actuations = 2;

const int x_start = 0;
const int y_start = x_start + N;
const int psi_start = y_start + N;
const int v_start = psi_start + N;
const int cte_start = v_start + N;
const int epsi_start = cte_start + N;
const int delta_start = epsi_start + N;
const int a_start = delta_start + N -1;

const double ref_cte = 0;
const double ref_epsi = 0;
const double ref_v = 60;

class FG_eval {
 public:
  // Fitted polynomial coefficients
  Eigen::VectorXd coeffs;
  FG_eval(Eigen::VectorXd coeffs) { this->coeffs = coeffs; }

  typedef CPPAD_TESTVECTOR(AD<double>) ADvector;
  void operator()(ADvector& fg, const ADvector& vars) {
    // TODO: implement MPC
    // `fg` a vector of the cost constraints, `vars` is a vector of variable values (state & actuators)
    // NOTE: You'll probably go back and forth between this function and
    // the Solver function below.
	
	// set weights
	const double w_cte = 200;
	const double w_epsi = 100;
	const double w_dv = 10;
	const double w_ddelta = 1000;
	const double w_da = 1000;

	fg[0] = 0;
	// Minimize cte and epsi
	// Make the velocity close to the reference velocity
	for (int t = 0; t < N; t++){
		fg[0] += w_cte * CppAD::pow(vars[cte_start + t], 2);
		fg[0] += w_epsi * CppAD::pow(vars[epsi_start + t], 2);    
		fg[0] += w_dv * CppAD::pow(vars[v_start + t] - ref_v, 2);
	}

    // Minimize jerk
    for (int t = 0; t < N - 2; t++){
		fg[0] += w_ddelta * CppAD::pow(vars[delta_start + t + 1] - vars[delta_start + t], 2);
		fg[0] += w_da * CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);
    }

    // Minimize acceleration magnitude, turning up the scalar will make the driving more conservative.
    for (int t = 0; t < N - 1; t++){
		fg[0] += 3.0*(0.20 * CppAD::pow(CppAD::pow(vars[v_start+t], 2)/Lf*vars[delta_start + t], 2) + CppAD::pow(vars[v_start + t + 1] - vars[v_start + t], 2));
    }

	// initialize the state
	fg[x_start    + 1] = vars[x_start];
	fg[y_start    + 1] = vars[y_start];
	fg[psi_start  + 1] = vars[psi_start];
	fg[v_start    + 1] = vars[v_start];
	fg[cte_start  + 1] = vars[cte_start];
	fg[epsi_start + 1] = vars[epsi_start];
	for (int t = 1; t < N; t++) {
		AD<double> x1 = vars[x_start + t];
		AD<double> y1 = vars[y_start + t];
		AD<double> psi1 = vars[psi_start + t];
		AD<double> v1 = vars[v_start + t];
		AD<double> cte1 = vars[cte_start + t];
		AD<double> epsi1 = vars[epsi_start + t];

		AD<double> x0 = vars[x_start + t - 1];
		AD<double> y0 = vars[y_start + t - 1];
		AD<double> psi0 = vars[psi_start + t - 1];
		AD<double> v0 = vars[v_start + t - 1];
		AD<double> cte0 = vars[cte_start + t - 1];
		AD<double> epsi0 = vars[epsi_start + t - 1];

		AD<double> a0 = vars[a_start + t - 1];
		AD<double> delta0 = vars[delta_start + t - 1];
		
		AD<double> f = coeffs[3] * CppAD::pow(x0, 3) + coeffs[2] * CppAD::pow(x0, 2) + coeffs[1] * x0 + coeffs[0];
		AD<double> psi_desired = CppAD::atan(3.0*coeffs[3] * CppAD::pow(x0, 2) + 2.0*coeffs[2] * x0 + coeffs[1]);
      
		// Global Kinematic Model
		fg[1 + x_start + t] = x1 - (x0 + v0 * CppAD::cos(psi0) * dt);
		fg[1 + y_start + t] = y1 - (y0 + v0 * CppAD::sin(psi0) * dt);
		fg[1 + psi_start + t] = psi1 - (psi0 + v0 * delta0/Lf * dt);
		fg[1 + v_start + t] = v1 - (v0 + a0 * dt);

		fg[1 + cte_start + t] = cte1 - ((f - y0) + v0 * CppAD::sin(epsi0) * dt);
		fg[1 + epsi_start + t] = epsi1 - ((psi0 - psi_desired) + v0 * delta0 / Lf * dt);
	}
  }
};

//
// MPC class definition implementation.
//
MPC::MPC() {}
MPC::~MPC() {}

vector<double> MPC::Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs) {
  bool ok = true;
  size_t i;
  typedef CPPAD_TESTVECTOR(double) Dvector;

  // TODO: Set the number of model variables (includes both states and inputs).
  // For example: If the state is a 4 element vector, the actuators is a 2
  // element vector and there are 10 timesteps. The number of variables is:
  //
  // 4 * 10 + 2 * 9
  size_t n_vars = 6 * N + 2 * (N - 1);
  // TODO: Set the number of constraints
  size_t n_constraints = 6 * N;

  // Initial value of the independent variables.
  // SHOULD BE 0 besides initial state.

  Dvector vars(n_vars);
  for (unsigned int i = 0; i < n_vars; i++) {
    vars[i] = 0;
  }

  vars[x_start] = state[0];
  vars[y_start] = state[1];
  vars[psi_start] = state[2];
  vars[v_start] = state[3];
  vars[cte_start] = state[4];
  vars[epsi_start] = state[5];

  Dvector vars_lowerbound(n_vars);
  Dvector vars_upperbound(n_vars);
  // TODO: Set lower and upper limits for variables.

  // Lower and upper limits for the constraints
  // Should be 0 besides initial state.
  Dvector constraints_lowerbound(n_constraints);
  Dvector constraints_upperbound(n_constraints);

  for (int i = 0; i < delta_start; i++){
      vars_lowerbound[i] = -1.0e19;
      vars_upperbound[i] = 1.0e19;
  }

  for (int i = delta_start; i < a_start; i++){
      vars_lowerbound[i] = -0.436332;
      vars_upperbound[i] = 0.436332;
  }

  for (unsigned int i = a_start; i < n_vars; i++){
      vars_lowerbound[i] = -1.0;
      vars_upperbound[i] = 1.0;
  }


  for (unsigned int i = 0; i < n_constraints; i++) {
    constraints_lowerbound[i] = 0;
    constraints_upperbound[i] = 0;
  }

  constraints_lowerbound[x_start] = state[0];
  constraints_lowerbound[y_start] = state[1];
  constraints_lowerbound[psi_start] = state[2];
  constraints_lowerbound[v_start] = state[3];
  constraints_lowerbound[cte_start] = state[4];
  constraints_lowerbound[epsi_start] = state[5];

  constraints_upperbound[x_start] = state[0];
  constraints_upperbound[y_start] = state[1];
  constraints_upperbound[psi_start] = state[2];
  constraints_upperbound[v_start] = state[3];
  constraints_upperbound[cte_start] = state[4];
  constraints_upperbound[epsi_start] = state[5];


  // object that computes objective and constraints
  FG_eval fg_eval(coeffs);

  //
  // NOTE: You don't have to worry about these options
  //
  // options for IPOPT solver
  std::string options;
  // Uncomment this if you'd like more print information
  options += "Integer print_level  0\n";
  // NOTE: Setting sparse to true allows the solver to take advantage
  // of sparse routines, this makes the computation MUCH FASTER. If you
  // can uncomment 1 of these and see if it makes a difference or not but
  // if you uncomment both the computation time should go up in orders of
  // magnitude.
  options += "Sparse  true        forward\n";
  options += "Sparse  true        reverse\n";
  // NOTE: Currently the solver has a maximum time limit of 0.5 seconds.
  // Change this as you see fit.
  options += "Numeric max_cpu_time          0.5\n";

  // place to return solution
  CppAD::ipopt::solve_result<Dvector> solution;

  // solve the problem
  CppAD::ipopt::solve<Dvector, FG_eval>(
      options, vars, vars_lowerbound, vars_upperbound, constraints_lowerbound,
      constraints_upperbound, fg_eval, solution);

  // Check some of the solution values
  ok &= solution.status == CppAD::ipopt::solve_result<Dvector>::success;

  // Cost
  auto cost = solution.obj_value;
  std::cout << "Cost " << cost << std::endl;

  std::vector<double> result = {solution.x[delta_start], solution.x[a_start]};
  for(int i=0; i < N; i++){
    result.push_back(solution.x[i+x_start]);
  }

  for(int i=0; i < N; i++){
    result.push_back(solution.x[i+y_start]);
  }
  // TODO: Return the first actuator values. The variables can be accessed with
  // `solution.x[i]`.
  //
  // {...} is shorthand for creating a vector, so auto x1 = {1.0,2.0}
  // creates a 2 element double vector.
  return result;
}
