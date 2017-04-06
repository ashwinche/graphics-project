//using namespace std;
#include "Actuator.h"
/*
Abstract class which can be implemented in order to control Agents.

The actuator provides useful functions to Controllers (i.e. AIs or an InputMap)
that can be used to set the steering state. doSteering is then called during
the agent's update tick, and updates it according to it's steering state.
*/

// Agent_Type is the type of the agent that this actuator is controlling


template <>
class Actuator <TestAgent> {

	public:
	    Actuator(Agent_Type agent) {
	        //Enforce that Agent_Type be a subclass of Agent at compile time
	        static_assert(std::is_base_of<Agent, Agent_Type>::value, "Agent_Type not derived from Agent");

	        this.agent = agent;
	        //steeringState = new SteeringState<Agent_Type>();
	    }

	    // pure virtual function providing interface framework.
	    virtual void doSteering() = 0;
	    //Should the agent pass itself into this instead?
	   
	protected:
	    Agent_Type agent;    // See comment under doSteering
	    //SteeringState<Agent_Type> steeringState;
    void doSteering() {
     	agent.x = 5;
    }
};