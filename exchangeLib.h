#ifndef EXCHANGELIB_H
#define EXCHANGELIB_H

#include <string>
#include "Socket.h"
#include <vector>

using namespace std;

//Information for commands
const string USRM = "[username]";
const string PSWD = "[password]";
const string INFO = USRM + " " + PSWD + " ";

//Connects to algorithmics.bu.edu at port 8001
//    @returns void
//    @param    none
void connect();

//Gets current status
//    @returns vector of currency amounts
//    @param    none
vector<double> status();

//Gets the exchange rate between two currencies
//    @returns double of the exchange rate
//    @param    two integers for the IDs of the two currencies
double oneRate(int curr1, int curr2);

//Gets the exchange rate between all currencies 
//    @returns two dimensional vector of rates from row to column
//    @param    the reference to the two dimensional vector to store the rates in
void allRate(vector<vector<double> > &rates);

//Exchanges one currency for another
//    @returns amount of second currency recieved
//    @param    two integers for the IDs of the two currencies and the amount to exchange
double exchange(int curr1, double amount, int curr2);

//Resets all currency values
//    @returns void
//    @param    none
void saveMe();

//Disconnects from socket
//    @returns void
//    @param none
void done();

#endif