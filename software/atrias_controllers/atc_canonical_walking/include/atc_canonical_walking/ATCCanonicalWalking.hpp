#ifndef ATCCanonicalWalking_HPP
#define ATCCanonicalWalking_HPP

/**
 * @file ATCCanonicalWalking.hpp
 * @author Ryan Van Why
 * @brief This controller is to test the tracking of canonical walking
 */


// Top-level controllers are components, so we need to include this.
#include <rtt/Component.hpp>

// Include the ATC class
#include <atrias_control_lib/ATC.hpp>

// Our logging data type.
#include "atc_canonical_walking/controller_log_data.h"
// The type transmitted from the GUI to the controller
#include "atc_canonical_walking/controller_input.h"
// The type transmitted from the controller to the GUI
#include "atc_canonical_walking/controller_status.h"


// Our subcontroller types
#include <asc_common_toolkit/ASCCommonToolkit.hpp>
#include <asc_pd/ASCPD.hpp>
#include <asc_rate_limit/ASCRateLimit.hpp>
#define N_DOFS 5
#define N_OUTPUTS 4
#define N_MOTORS 4
#define N_STATES 10
#define N_PARAMS 5


// Namespaces we're using
using namespace std;

// Our namespaces
namespace atrias {
  namespace controller {
    
    static const double invT[4][4] = {
      {1  ,   0,-1/2,   0},
      {1  ,   0, 1/2,   0},
      {0  ,   1,   0,-1/2},
      {0  ,   1,   0, 1/2}};
    static const double a_opt[4][5]={
      {0.29315756,-3.47513300,-0.12892990,0.61400634,3.27548479}, 	// StanceLegAngleParam
      {-0.46569548,3.05272503,-0.48594434,3.30148543,3.54487616}, 	// NonStanceLegAngleParam
      {-0.00689978,-7.58620951,0.00406186,-2.62897489,0.26955308},	// StanceKneeParam
      {-0.89695408,6.13951996,-0.07070758,0.48012954,0.88758479}};	// NonStanceKneeParam
    static const double x_opt[4] = {3.08386532926800,   //rLeg.halfA.motorAngle
			     3.07449603435955,   //rLeg.halfB.motorAngle
			     3.43731571117661,   //lLeg.halfA.motorAngle
			     3.69996901140680};    //lLeg.halfB.motorAngle
    
    class ATCCanonicalWalking : public ATC<
      atc_canonical_walking::controller_log_data_,
      atc_canonical_walking::controller_input_,
      atc_canonical_walking::controller_status_>
    {
    public:
      /** 
       * @brief The constructor for this controller.
       * @param name The name of this component.
       * Every top-level controller will have this name parameter,
       * just like current controllers.
       */
      ATCCanonicalWalking(string name);
      
    private:
      /** 
       * @brief This is the main function for the top-level controller.
       */
      void controller();
      
      /** 
       * @brief Those are top level controllers.
       */ 
      void updateController();
      void hipController();
      void standingController();
      void stoppingController();
      void walkingController();

      // Include subcontrollers and variables here
      
      

      // PD controllers for each motor
      ASCPD pdLA; // Left  A
      ASCPD pdLB; // Left  B
      ASCPD pdRA; // Right A
      ASCPD pdRB; // Right B

      // Rate limiters for each motor
      ASCRateLimit rateLimLA; // Left  A
      ASCRateLimit rateLimLB; // Left  B
      ASCRateLimit rateLimRA; // Right A
      ASCRateLimit rateLimRB; // Right B
      
      /**
       * @define Transformation Matrix and Its Inverse Matrix
       *  y2 = T * states;
       *  states = invT * y2;
       *
       * double T[4][4] = {
       *       {1/2, 1/2,   0,   0},
       *       {0  ,   0, 1/2, 1/2},
       *       {-1 ,   1,   0,   0},
       *       {0  ,   0,  -1,   1}};
       */
      //double invT[N_OUTPUTS][N_OUTPUTS];
      //static const double invT, a_opt, x_opt;

      // define varibles
      double theta_limit1;  // Initial phase (or parameterized time)
      double theta_limit2;  // Final phase (or parameterized time)

      double param_mat[N_OUTPUTS][N_PARAMS];  // The parameter matrix of Canonical Walking Function.

      /**
       * @define y2d (y2dDot) Outputs: sLegAngle, NsLegAngle, sKnee, NsKnee
       */
      double y2d[N_OUTPUTS];    // The desired outputs
      double y2dDot[N_OUTPUTS]; // The desired time derivative of outputs

      /**
       * @define robot states: {qxT, q1, q2, q1L, q2L, dqxT, dq1, dq2, dq1L, dq2L}
       * **       q1, q2 are the stance leg motor anlges;
       * **       q1L, q2L are the non-stance leg motor angles;
       * **       qxT is torso angle.
       */
      double xa[N_STATES];      // The current robot states in the old configuration.  
      double xd[N_STATES];      // The desired robot states in the old configuration.
      
      double pos_initial[N_MOTORS]; // The initial position for walking
      
      /**
       * @define desired motor angles: {qlATgt, qlBTgt, qrATgt, qrBTgt}
       */
      double qTgt[N_MOTORS];    // The desired motor position in the current configuration.
      double dqTgt[N_MOTORS];   // The desired motor velocity in the current configuration.
     
      enum StanceLeg {LEFT_LEG, RIGHT_LEG};
      StanceLeg sleg, stance_prev;
      enum CtrlState {STANDING = 0, WALKING = 1, STOPPING = 2};
      CtrlState controllerState;

      // The maximum rate of motion
      double rate;
      
      bool    isInitialized;
      double tau_prev;
      int    cnt;
      int    timer;
      


      // Define subfunctions
      /**
       * @brief This is to initialize the walking parameters.
       */
      void init_param();
      
      /**
       * @brief This is to set the initial position for walking.
       */
      void init_pos();
      
      /**
       * @brief This is to compute the inverse kinematics of the system.
       */
      void phi_inverse_mat();

      /**
       * @brief This is to compute the phase (parameterized time) 'tau'
       */
      double compute_tau();
      
      /**
       * @brief This is to compute the time derivative of 'tau'
       */
      double compute_dtau();

      /**
       * @brief This is to compute the desired output 'y2d'
       * @param tau The parameterized time
       */
      void compute_y2d(double tau);

      /**
       * @brief This is to compute the desired time derivative (velocity) of 'y2d'
       * @param tau The parameterized time
       * @param dtau The derivative of parameterized time
       */
      void compute_y2dDot(double tau, double dtau);

      /**
       * @brief This is to convert the states from the current coordinate configuration to the old (Dr Grizzle's) configuration.
       */
      void convert2torso();

      /**
       * @brief This is to convert the states from the old (Dr. Grizzle's)  coordinate configuration to the current configuration.
       */
      void convert2bodypitch();

      

    };
    
  }
}

#endif // ATCCanonicalWalking_HPP
