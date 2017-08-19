Model Predictive Control(MPC)
================================

Definition
----------

Model predictive control (MPC) is an advanced method of process control that has been in use in the process industries in chemical plants and oil refineries since the 1980s. In recent years it has also been used in power system balancing models. Model predictive controllers rely on dynamic models of the process, most often linear empirical models obtained by system identification. The main advantage of MPC is the fact that it allows the current timeslot to be optimized, while keeping future timeslots in account. This is achieved by optimizing a finite time-horizon, but only implementing the current timeslot. MPC has the ability to anticipate future events and can take control actions accordingly. PID and LQR controllers do not have this predictive ability. MPC is nearly universally implemented as a digital control, although there is research into achieving faster response times with specially designed analog circuitry. (From Wikipedia)

Model Detail
------------

### State
There are six states, which are ***x, y, psi, v, cte,*** and ***espi***.

### Actuation
There are two actuations, which are ***delta*** and ***a***.

### Update
![](https://github.com/rainbamboooo/MPC-Udacity-Self-Driving-Car-Nanodegree-Term2-project5/raw/master/1.png)

### Timestep Length and Elapsed Duration
Timestep length ***N = 25***, elapsed duration ***dt = 0.05s***.

###Polynomial Fitting and MPC Preprocessing
I transformed the map coordination to the car coordination. Then I did the polynomial fitting.

###Latency

    	  double latency = 0.1;
		  px += v * std::cos(psi) * latency;
		  py += v * std::sin(psi) * latency;
		  psi -= v * delta/2.67 * latency;
		  v += a * latency;
