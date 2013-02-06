/*! \file controller_component.cpp
 *  \author Mikhail Jones
 *  \brief Orocos Component code for the atc_single_leg_hopping controller.
 */

#include <atc_single_leg_hopping/controller_component.h>

// Initialize variables (IS THIS THE BEST PLACE FOR THIS??)
bool isStance = false;

namespace atrias {
namespace controller {

ATCSingleLegHopping::ATCSingleLegHopping(std::string name):
    RTT::TaskContext(name),
    logPort(name + "_log"),
    guiDataOut("gui_data_out"),
    guiDataIn("gui_data_in")
{
    this->provides("atc")
        ->addOperation("runController", &ATCSingleLegHopping::runController, this, ClientThread)
        .doc("Get robot_state from RTOps and return controller output.");

    // Add subcontrollers properties.
    this->addProperty("pd0Name", pd0Name)
        .doc("Leg PD subcontroller.");
    this->addProperty("pd1Name", pd1Name)
        .doc("Hip PD subcontroller.");

    // For the GUI
    addEventPort(guiDataIn);
    addPort(guiDataOut);
    pubTimer = new GuiPublishTimer(20);

    // Logging
    // Create a port
    addPort(logPort);
    // Buffer port so we capture all data.
    ConnPolicy policy = RTT::ConnPolicy::buffer(100000);
    // Transport type = ROS
    policy.transport = 3;
    // ROS topic name
    policy.name_id = "/" + name + "_log";
    // Construct the stream between the port and ROS topic
    logPort.createStream(policy);

    log(Info) << "[ATCMT] Constructed!" << endlog();
}

atrias_msgs::controller_output ATCSingleLegHopping::runController(atrias_msgs::robot_state rs) {

    // Do nothing unless told otherwise
    co.lLeg.motorCurrentA   = 0.0;
    co.lLeg.motorCurrentB   = 0.0;
    co.lLeg.motorCurrentHip = 0.0;
    co.rLeg.motorCurrentA   = 0.0;
    co.rLeg.motorCurrentB   = 0.0;
    co.rLeg.motorCurrentHip = 0.0;

    // Only run the controller when we're enabled
    if ((rtOps::RtOpsState)rs.rtOpsState != rtOps::RtOpsState::ENABLED)
        return co;

    // Define leg link lengths (thigh and shin).
    l1 = 0.5;
    l2 = 0.5;

    // Gravity
    g = -9.81;




    // begin control code //

    // Check if we are in flight or stance phase
    if (rs.position.zPosition < guiIn.slip_leg || guiIn.debug1) {
        isStance = true;
    } else {
        isStance = false;
    }

    // Stance phase control =======================================================================
    if (isStance) {

        // Define desired force at end-effector.
        if (guiIn.constant_force) {
            // Constant force
            Fy = guiIn.fy;
            Fz = guiIn.fz;

        } else if (guiIn.slip_force) {
            // Time step
            delta = 0.001;

            // SLIP model 4th orer Runge-Kutta numerical solution
            rNew = r + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/3.0 + delta*dr/6.0 + delta*(dr + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass)/2.0)/3.0 + delta*(dr + delta*(-g*sin(-q - delta*(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)/2.0) + (r + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)*pow(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)*(2.0*dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr), 2) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)/guiIn.slip_mass))/6.0;

            drNew = dr + delta*(g*sin(q + delta*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)*(2.0*dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr))) + (r + delta*(dr + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass)/2.0))*pow(dq - delta*(g*cos(-q - delta*(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)/2.0) + (2.0*dr + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass))*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)*(2.0*dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr)))/(r + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0), 2) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*(dr + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass)/2.0))/guiIn.slip_mass)/6.0 + delta*(-g*sin(-q - delta*(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)/2.0) + (r + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)*pow(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)*(2.0*dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr), 2) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)/guiIn.slip_mass)/3.0 + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/6.0 + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass)/3.0;

            qNew = q + delta*dq/6.0 + delta*(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)/3.0 + delta*(dq - delta*(g*cos(-q - delta*(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)/2.0) + (2.0*dr + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass))*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)*(2.0*dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr)))/(r + delta*(dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0))/6.0 + delta*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*(2.0*dq*dr + g*cos(q))/r/2.0)*(2.0*dr + delta*(r*dq*dq + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr))/3.0;

            dqNew = dq - delta*(g*cos(-q - delta*(dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)/2.0) + ((2.0*dr) + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass))*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)*((2.0*dr) + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr)))/(3.0*r + 3.0/2.0*delta*(dr + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)) - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)*((2.0*dr) + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(3.0*r + 3.0/2.0*delta*dr) - delta*(g*cos(q + delta*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)*((2.0*dr) + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr))) + ((2.0*dr) + 2.0*delta*(-g*sin(-q - delta*(dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)/2.0) + (r + delta*(dr + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)*pow(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)*((2.0*dr) + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr), 2) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*(dr + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)/guiIn.slip_mass))*(dq - delta*(g*cos(-q - delta*(dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)/2.0) + ((2.0*dr) + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass))*(dq - delta*(g*cos(q + delta*dq/2.0) + (dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0)*((2.0*dr) + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)))/(2.0*r + delta*dr)))/(r + delta*(dr + delta*(r*(dq*dq) + g*sin(q) + guiIn.slip_spring*(guiIn.slip_leg - r)/guiIn.slip_mass)/2.0)/2.0)))/(6.0*r + 6.0*delta*(dr + delta*(g*sin(q + delta*dq/2.0) + pow(dq - delta*((2.0*dq*dr) + g*cos(q))/r/2.0, 2)*(r + delta*dr/2.0) - guiIn.slip_spring*(r - guiIn.slip_leg + delta*dr/2.0)/guiIn.slip_mass)/2.0)) - delta*((2.0*dq*dr) + g*cos(q))/r/6.0;

            // Check if we are in flight or stance phase
            if (r > guiIn.slip_leg) {
                Fy = 0.0;
                Fz = 0.0;
            } else {
                // Replace last time step with next time step
                r = rNew;
                dr = drNew;
                q = qNew;
                dq = dqNew;

                // Calculate force components
                Fy = guiIn.slip_spring*(guiIn.slip_leg - r)*cos(q);
                Fz = guiIn.slip_spring*(guiIn.slip_leg - r)*sin(q);
            }

        } else {
            Fy = 0.0;
            Fz = 0.0;

        }

        if (guiIn.debug2) {
            printf("Fy: %f\n", Fy);
            printf("Fz: %f\n", Fz);
        }

        // Jacobian matrix with global joint angles.
        J11 = l1*sin(rs.lLeg.halfA.motorAngle);
        J12 = -l2*sin(rs.lLeg.halfB.motorAngle);
        J21 = l1*cos(rs.lLeg.halfA.motorAngle);
        J22 = -l2*cos(rs.lLeg.halfB.motorAngle);

        // Compute required spring torque from desired end-effector force.
        // This assumes leg angles are global, if not we need to add in body pitch
        tauA = (Fz*J11 + Fy*J21);
        tauB = (Fz*J12 + Fy*J22);

        if (guiIn.debug3) {
            printf("tauA: %f\n", tauA);
            printf("tauB: %f\n", tauB);
        }

        // Compute required spring deflection.
        desDeflA = tauA/guiIn.robot_spring;
        desDeflB = tauB/guiIn.robot_spring;

        // Compute current spring deflection.
        deflA = rs.lLeg.halfA.motorAngle - rs.lLeg.halfA.legAngle;
        deflB = rs.lLeg.halfB.motorAngle - rs.lLeg.halfB.legAngle;

        // Compute required motor position.
        leftMotorAngle.A = rs.lLeg.halfA.legAngle + desDeflA;
        leftMotorAngle.B = rs.lLeg.halfB.legAngle + desDeflB;

        // Compute leg length from motor positions.
        legAng = (rs.lLeg.halfA.legAngle + rs.lLeg.halfB.legAngle)/2.0;
        legLen = cos(rs.lLeg.halfA.legAngle - legAng);

        // Make right leg mirror the left leg but with 85% of the length.
        rightMotorAngle = legToMotorPos(legAng, legLen*0.85);

        // Set hip gains
        P1.set(guiIn.flight_hip_P_gain);
        D1.set(guiIn.flight_hip_D_gain);

        // Set flight leg gains and currents
        P0.set(guiIn.flight_leg_P_gain);
        D0.set(guiIn.flight_leg_D_gain);
        co.rLeg.motorCurrentA = pd0Controller(rightMotorAngle.A, rs.rLeg.halfA.motorAngle, 0.0, rs.rLeg.halfA.motorVelocity);
        co.rLeg.motorCurrentB = pd0Controller(rightMotorAngle.B, rs.rLeg.halfB.motorAngle, 0.0, rs.rLeg.halfB.motorVelocity); 

        // Set stance leg gains and currents
        P0.set(guiIn.stance_leg_P_gain);
        D0.set(guiIn.stance_leg_D_gain);
        co.lLeg.motorCurrentA = pd0Controller(leftMotorAngle.A, rs.lLeg.halfA.motorAngle, 0.0, rs.lLeg.halfA.motorVelocity);
        co.lLeg.motorCurrentB = pd0Controller(leftMotorAngle.B, rs.lLeg.halfB.motorAngle, 0.0, rs.lLeg.halfB.motorVelocity);

    // Flight phase control =======================================================================
    } else {
        
        // Reset initial conditions --------------------------------------------------------------- (TEMP)
        r = guiIn.slip_leg;
        dr = -sqrt(2.0*9.81*guiIn.slip_height);
        q = M_PI/2.0;
        dq = 0.0;

        if (guiIn.debug4) {
            printf("r: %f\n", r);
            printf("dr: %f\n", dr);
            printf("q: %f\n", q);
            printf("dq: %f\n", dq);
        }

        // Leg control
        leftMotorAngle = legToMotorPos(M_PI/2.0, guiIn.slip_leg);
        rightMotorAngle = legToMotorPos(M_PI/2.0, guiIn.slip_leg*0.85);

        // Set hip gains
        P1.set(guiIn.flight_hip_P_gain);
        D1.set(guiIn.flight_hip_D_gain);

        // Set flight leg gains and currents
        P0.set(guiIn.flight_leg_P_gain);
        D0.set(guiIn.flight_leg_D_gain);
        co.rLeg.motorCurrentA = pd0Controller(rightMotorAngle.A, rs.rLeg.halfA.motorAngle, 0.0, rs.rLeg.halfA.motorVelocity);
        co.rLeg.motorCurrentB = pd0Controller(rightMotorAngle.B, rs.rLeg.halfB.motorAngle, 0.0, rs.rLeg.halfB.motorVelocity); 
        co.lLeg.motorCurrentA = pd0Controller(leftMotorAngle.A, rs.lLeg.halfA.motorAngle, 0.0, rs.lLeg.halfA.motorVelocity);
        co.lLeg.motorCurrentB = pd0Controller(leftMotorAngle.B, rs.lLeg.halfB.motorAngle, 0.0, rs.lLeg.halfB.motorVelocity);

    }

    // Hip Control -------------------------------------------------------------------------------- (TEMP)
    if (guiIn.constant_hip) {
        // Constant angle        
        leftHipAngle = guiIn.hip_angle;
        rightHipAngle = guiIn.hip_angle;

        // Set motor currents
        co.lLeg.motorCurrentHip = pd1Controller(leftHipAngle, rs.lLeg.hip.legBodyAngle, 0.0, rs.lLeg.hip.legBodyVelocity);     
        co.rLeg.motorCurrentHip = pd1Controller(rightHipAngle, rs.rLeg.hip.legBodyAngle, 0.0, rs.rLeg.hip.legBodyVelocity);

    } else if (guiIn.advanced_hip){
        // Constant y end-effector position
        leftHipAngle = guiIn.left_hip_target;
        rightHipAngle = guiIn.right_hip_target;
        //hip0Controller((rs.lLeg.halfA.legAngle + rs.lLeg.halfB.legAngle) / 2.0, rs.position.boomAngle, 0)
        //hip1Controller((rs.rLeg.halfA.legAngle + rs.rLeg.halfB.legAngle) / 2.0, rs.position.boomAngle, 1)

        // Set motor currents
        co.lLeg.motorCurrentHip = pd1Controller(leftHipAngle, rs.lLeg.hip.legBodyAngle, 0.0, rs.lLeg.hip.legBodyVelocity);     
        co.rLeg.motorCurrentHip = pd1Controller(rightHipAngle, rs.rLeg.hip.legBodyAngle, 0.0, rs.rLeg.hip.legBodyVelocity);

    } else {
        // Do nothing
    }

    // end control code //





    // Command a run state
    co.command = medulla_state_run;

    // Let the GUI know the controller run state
    guiOut.isEnabled = ((rtOps::RtOpsState) rs.rtOpsState == rtOps::RtOpsState::ENABLED);

    // Send data to the GUI
    if (pubTimer->readyToSend())
        guiDataOut.write(guiOut);

    // Stuff the msg and push to ROS for logging
    logData.desiredState = 0.0;
    logPort.write(logData);

    // Output for RTOps
    return co;

}

// Don't put control code below here!
bool ATCSingleLegHopping::configureHook() {

    // Connect to the subcontrollers
    pd0 = this->getPeer(pd0Name);
    if (pd0)
        pd0Controller = pd0->provides("pd")->getOperation("runController");

    pd1 = this->getPeer(pd1Name);
    if (pd1)
        pd1Controller = pd1->provides("pd")->getOperation("runController");

    legToMotorPos = this->provides("legToMotorTransforms")->getOperation("posTransform");

    // Get references to the attributes
    P0 = pd0->properties()->getProperty("P");
    D0 = pd0->properties()->getProperty("D");
    P1 = pd1->properties()->getProperty("P");
    D1 = pd1->properties()->getProperty("D");

    log(Info) << "[ATCMT] configured!" << endlog();
    return true;
}

bool ATCSingleLegHopping::startHook() {
    log(Info) << "[ATCMT] started!" << endlog();
    return true;
}

void ATCSingleLegHopping::updateHook() {
    guiDataIn.read(guiIn);
}

void ATCSingleLegHopping::stopHook() {
    log(Info) << "[ATCMT] stopped!" << endlog();
}

void ATCSingleLegHopping::cleanupHook() {
    log(Info) << "[ATCMT] cleaned up!" << endlog();
}

ORO_CREATE_COMPONENT(ATCSingleLegHopping)

}
}