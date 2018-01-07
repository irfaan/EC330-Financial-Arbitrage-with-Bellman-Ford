#include "exchangeLib.h"
#include <iostream>
#include <sstream>
#include <cmath>

using namespace std;

//Information for connection
const string ADDRESS = "[address]";
const int PORT = [port];
SocketClient soc;

//Connects to algorithmics.bu.edu at port 8001
//    @returns void
//    @param    none
void connect()
{
    soc = SocketClient(ADDRESS, PORT);
    cout << soc.ReceiveLine() << endl;
}

//Gets current status
//    @returns vector of currency amounts
//    @param    none
vector<double> status()
{
    //Sends the command
    soc.SendLine(INFO + "getStatus");

    //Puts currency data into vector
    string status = soc.ReceiveLine();
    status = status.substr(29, status.size());
    cout << status << endl;
    vector<double> amounts;
    stringstream stream(status);
    double val;
    while (stream >> val)
    {
        amounts.push_back(val);
    }

    //Puts total value into location 100 in vector
    status = soc.ReceiveLine();
    cout << status << endl;
    stringstream stream2(status.substr(10, status.size()));
    stream2 >> val;
    amounts.push_back(val);

    //Returns vector of amounts
    return amounts;
}

//Gets the exchange rate between two currencies
//    @returns double of the exchange rate
//    @param    two integers for the IDs of the two currencies
double oneRate(int curr1, int curr2)
{
    //Sends command
    soc.SendLine(INFO + "getOneRate " + to_string(curr1) + " " + to_string(curr2));

    //Gets the rate and converts it to a double
    string rates = soc.ReceiveLine();
    rates = rates.substr(32, rates.size());
    stringstream stream(rates);
    double rate = 0;
    stream >> rate;

    //Returns the rate
    return rate;
}

//Gets the exchange rate between all currencies 
//    @returns two dimensional vector of rates from row to column
//    @param    the reference to the two dimensional vector to store the rates in
void allRate(vector<vector<double> > &rates)
{
    //Sends the command
    soc.SendLine(INFO + "getAllRates");
    
    //Each line contains rates from one currency to all others
    //    Put each line in its own vector in the passed through two
    //    dimensional vector
    for (int ii = 0; ii < 100; ii++)
    {
        rates.push_back(vector<double>());
        string input = soc.ReceiveLine();
        stringstream rate(input);
        double val;
        while (rate >> val)
        {
            rates[ii].push_back(val);
        }
    }
}

//Exchanges one currency for another
//    @returns void
//    @param    two integers for the IDs of the two currencies and the amount to exchange
double exchange(int curr1, double amount, int curr2)
{
    //Amount of currency recieved by all exchanges
    double recieved = 0;

    //If the given currency is USD, subtract 5
    //    Keeps all USD from being exchanged since it is needed for all other exchanges
    if (curr1 == 0)
        amount = amount - 5;

    //If given value is less than zero, do nothing
    if (amount < 0)
        cout << "NEGATIVE AMOUNT" << endl;
    //If the currencies are the same, do nothing
    else if (curr1 == curr2)
        cout << "CURRENCIES EQUAL" << endl;
    //Else make exchanges
    else
    {
        //Only 1000 of each currency can be traded at a time
        //    If over a 1000 to exchange, exchange a thousand at a time
        //    until less than a thousand
        int thousands = 0;
        if (amount > 1000)
        {
            do
            {
                amount = amount - 1000.0;
                thousands++;
            } while (amount > 1000);
            for (int ii = 0; ii < thousands; ii++)
            {
                soc.SendLine(INFO + "exchange " + to_string(curr1) + " 1000 " + to_string(curr2));
                soc.ReceiveLine();
                soc.ReceiveLine();
                soc.ReceiveLine(); 
                string input = soc.ReceiveLine();
                stringstream stream(input.substr(35,input.size()));
                double val;
                stream >> val;
                recieved += val;
            }
        }
        //Trade what is left after all the thousands have been traded
        soc.SendLine(INFO + "exchange " + to_string(curr1) + " " + to_string(floor(amount * 1000000) / 1000000) + " " + to_string(curr2));
        soc.ReceiveLine();
        soc.ReceiveLine();
        soc.ReceiveLine();
        string input = soc.ReceiveLine();
        stringstream stream(input.substr(35, input.size()));
        double val;
        stream >> val;
        recieved += val;
    }

    return recieved;
}

//Resets all currency values
//    @returns void
//    @param    none
void saveMe()
{
    soc.SendLine(INFO + "saveMe");
    cout << soc.ReceiveLine() << endl;
}

//Disconnects from socket
//    @returns void
//    @param none
void done()
{
    soc.SendLine("DONE");
    soc.ReceiveLine();
}