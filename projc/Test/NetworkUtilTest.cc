/*
 * Author: Rajan Jana
 * email: rjana@usc.edu
 *
 * Course: CSCI 551
 * Project B
 *
 * */

#include "../utility/utility.h"
#include "../utility/networkUtil.h"

int main(void){

	NetworkUtil networkUtil;

	printf("%s",networkUtil.getInterfaceAddress("eth1"));
}
