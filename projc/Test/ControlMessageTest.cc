/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 *
 * This test file validates control message generation and
 * extraction functionality.
 *
 * */

#include "../control/controlMessage.h"
#include "../utility/networkUtil.h"


void handleMessage(char* message){
	int circuitID, portNo;
	switch(ControlMessage::getMessageType(message)){
		case CIRCUIT_EXTEND:
			ControlMessage::extractCircuitExtendMessage(message,&circuitID,&portNo);
			printf("Circuit ID = %d\n",circuitID);
			printf("Port no = %d\n",portNo);
			break;
		case CIRCUIT_EXTEND_DONE:
			ControlMessage::extractCircuitExtendDoneMessage(message,&circuitID);
			printf("Circuit ID = %d\n",circuitID);
			break;
	}
}

int main(void){

	char message[25];
	ControlMessage::getCircuitExtendMessage(1,23452,message);
	NetworkUtil::printPacket((unsigned char*)message,25);

	handleMessage(message);

	ControlMessage::getCircuitExtendDoneMessage(2,message);
	NetworkUtil::printPacket((unsigned char*)message,25);

	handleMessage(message);

	//printf("Type is %d\n",(*(message+20)));
	//printf("Circuit ID is %d\n",ntohs((*(unsigned short*)(message+21))));
	//printf("Port no is %d\n",ntohs((*(unsigned short*)(message+23))));



}
