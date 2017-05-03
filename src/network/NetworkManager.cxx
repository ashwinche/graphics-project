#include "NetworkManager.hxx"

#include <world/world.hxx>
class World;
extern World* world; //global 


handler NetworkManager::table[5] = {{ CODE_PING,          &NetworkManager::pingCommand }, 
                                    { CODE_PONG,          &NetworkManager::pongCommand }, 
                                    { CODE_INIT,          &NetworkManager::initCommand },
                                    { CODE_OBJECT_CHANGE, &NetworkManager::objectChangeCommand },
                                    { CODE_CONTROLLER,    &NetworkManager::controllerStateCommand}};

NetworkManager::NetworkManager() 
{}


void NetworkManager::sendMessage(uint8_t* message, int len)
{
    struct sockaddr_in targetAddr = this->getTargetAddr();
    int sock = this->getTargetSocket();

    log(LOGMEDIUM, "\nSending message: (length: %d)\nTo: %s at port on socket %d\n", len,
        inet_ntoa(targetAddr.sin_addr), sock);
    logPrintBuf(LOGMEDIUM, message, len);
    int bytesSent = sendto(sock, message, len, 0, (struct sockaddr*)&targetAddr, sizeof(struct sockaddr_in));
    log(LOGMEDIUM, "Sent %d bytes\n\n", bytesSent);
}


void NetworkManager::recieveMessage(uint8_t* message, int len) {
    log(LOGMEDIUM, "\nrecieved message: ");
    logPrintBuf(LOGMEDIUM, message, len);
    log(LOGMEDIUM, "\n");
    
    uint8_t *current = message;

    while(current - message < len) {
        struct CommandHeader *command = (struct CommandHeader *)(current);

        if(current + sizeof(CommandHeader) + ntohs(command->len) - message >= len) {
            //TODO: error();
            return;
        }

        processCommand(ntohs(command->code), ntohs(command->len), current + sizeof(CommandHeader));

        current += sizeof(CommandHeader) + ntohs(command->len);
    }

    // message freeing is handled by the calling function
}

/* This is a wrapper for checkDispatch. Subclasses can replace this to
   checkDispatch with their own table. */
bool NetworkManager::processCommand(short code, short len, uint8_t *message) {
    log(LOGLOW, "command: %d %d\n", code, len);

    return checkDispatch(code, len, message);
}

// Actually checks the dispatch table for a code. Returns true if found.
bool NetworkManager::checkDispatch(short code, short len, uint8_t *message) {
    int dispatch_size = sizeof(table) / sizeof(table[0]);

    for(int i=0; i<dispatch_size; i++) {
        if(code == table[i].code) {
            table[i].func(*this, len, message);
            return true;
        }
    }

    return false;
}

void NetworkManager::sendCommand(short code, short len, uint8_t *payload) {
    int total_len = sizeof(code) + sizeof(len) + len;
    uint8_t message[total_len];
    short *m_short = (short*)message;
    m_short[0] = htons(code);
    m_short[1] = htons(len);
    memcpy(message + 4, payload, len);

    sendMessage(message, total_len);
}

void NetworkManager::pingCommand(COMMAND_PARAMS) {
    log(LOGLOW, "got ping\n");
    this->sendCommand(CODE_PONG, 0, nullptr);
}

void NetworkManager::pongCommand(COMMAND_PARAMS) {
    log(LOGLOW, "got pong\n");
    //this->sendCommand(CODE_PING, 0, nullptr);
}
void NetworkManager::initCommand(COMMAND_PARAMS) {}


void NetworkManager::objectChangeCommand(COMMAND_PARAMS) 
{
    posUpBuf *msg = getPosUpBuf(message);
    world->setEntData(msg);
    free(msg);

}

void NetworkManager::controllerStateCommand(COMMAND_PARAMS) {
    if(world->isClient())
        return;
    Actuator *actuator = ((ServerNetworkManager*)this)->actuator;
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    ControllerState state;
    memcpy(&state, message, len);
    //state->moude = ntoh(state->mouse)

    if(state.switchWeaponsKey != 0) {
        actuator->switchWeapons(state.switchWeaponsKey);
    }

    if(state.riseKey) {
        actuator->rise();
    }
    if(state.diveKey) {
        actuator->dive();
    }

    if(state.leftKey && !state.rightKey) {
        actuator->turnLeft();
    }
    if(state.rightKey && !state.leftKey) {
        actuator->turnRight();
    }

    if(state.forwardKey) {
        actuator->accelerate();
    }
    if(state.fireKey) {
        actuator->fire();
    }
}