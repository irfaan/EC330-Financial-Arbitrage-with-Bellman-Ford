#include <string>
#include <iostream>
#include "exchangeLib.h"
#include <cmath>
#include <Windows.h>
#include <algorithm>
#include <list>
#include <set>
#include <chrono>

using namespace std;

//Declare methods to be defined later
void initializeSingleSource();
void relax(int, int, vector< vector<double> >&rates);
bool findCycle(vector<vector<double> > &rates);
void convertToUS(vector<double> &amounts);

//Initialize variables to be used by all functions
vector<double> distances(100);
vector<int> pi(100);
list<int> cycle;

const double MAX_RATE = 1.5; //Absolute value of log base ten of the maximum rate allowed
const double TO_EXCHANGE = 4000; //Starting amount to exchange in USD when entering a cycle

//Main
int main()
{
    //Initialize random seed (used if no cycles are found)
    srand(time(NULL));

    //If an exception is caught, convert all back to USD
    bool toUS = false;

    //Do until I stop or break
    do
    {
        //Try running the loop
        try
        {
            //Start clock to ensure connection is quit before dropped
            chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
            
            //Connect to server
            connect();

            //Get all rates to search for a cycle in
            vector< vector<double> > rates;
            allRate(rates);

            //Get current amounts of each currency
            vector<double> amounts = status();

            //Convert all back to USD
            if (toUS)
            {
                convertToUS(amounts);
                amounts = status();
                toUS = false;
            }

            //Try to find a profitable cycle, if found, process it to make money
            if (findCycle(rates))
            {
                //Define Variables and Print Info for User
                double startVal; //Variable to hold the starting value of each cycle
                double exchangeVal = TO_EXCHANGE; //Amount to exchange, initialized to a chosen amount
                list<int>::iterator next = cycle.begin(); //Variable to hold which is the next currency to buy
                list<int>::iterator prev; //Variable to hold currency just bought
                chrono::duration<double> time_span; //Variable to hold how much time as passed
                cout << "Cycle found, executing " << cycle.size() << " exchanges..." << endl;

                //If the first currency in the cycle is not zero, buy it
                //    Assumes plenty of currency is in USD for transaction
                //    Update amounts after exchange
                //    Updates and increments the two cycle iterators prev and next
                if (*next != 0)
                {
                    exchangeVal = exchange(0, exchangeVal, *next);
                    amounts = status();
                }
                prev = next;
                next++;

                //Do while less than 15 seconds have elapsed and the cycle is making money
                do
                {
                    //Update starting value before cycle executes
                    startVal = amounts[100];

                    //Go through cycle trading currencies
                    //    Trades, updates amounts, updates prev and increments next
                    //    Goes until end of cycle
                    for (; next != cycle.end(); next++)
                    {
                        exchangeVal = exchange(*prev, exchangeVal, *next);
                        amounts = status();
                        prev = next;
                    }

                    //Prepare to repeat cycle by buying the first currency again
                    //    Return next to the beginning of the cycle
                    //    Make the exchange, update amounts and prev and increment next
                    next = cycle.begin();
                    exchangeVal = exchange(*prev, exchangeVal, *next);
                    amounts = status();
                    prev = next;
                    next++;

                    //Get the current duration since the connection began
                    //    Connection is dropped after 20 seconds so only repeat 
                    //    cycle if there is time and money is beging made
                    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
                    time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
                } while (time_span.count() < 12 && amounts[100] > startVal);
                
                //Convert back to USD
                exchangeVal = exchange(*prev, exchangeVal, 0);
                amounts = status();
            }
            //If no profitable cycle is found, throw a bunch of money into a random
            //    currency with a decent exchange rate and hope a cycle is created
            else
            {
                cout << "No cycles found " << endl;
                bool traded = false; //Switched to true if exchanges are made

                //Keep looking for random currencies until trading conditions are satisfied
                do
                {
                    //Randomly choose a currency. If it has a fairly small exchange not to much
                    //    money will be lost exchanging to it and back, exchange for it and then
                    //    back to USD. Get new status and switch traded to true
                    int val = rand() % 100;
                    double exchangeVal = TO_EXCHANGE;
                    if (rates[0][val] < 10 && rates[0][val] > 0.1 && 1.0 / rates[0][val] * rates[val][0] < 1.0001&& val != 0)
                    {
                        exchangeVal = exchange(0, exchangeVal, val);
                        exchangeVal = exchange(val, exchangeVal, 0);
                        traded = true;
                        amounts = status();
                    }
                } while (!traded);
                
            }

            //Close connection and wait for 20 seconds
            //    Can only reconnect to server every 20 seconds
            cout << "Done." << endl;
            cout << endl;
            if (amounts[100] < 12650)
                break;
            done();
            Sleep(20000);
        }
        //If any exceptions are caught, close connection and sleep for 20 seconds
        //    Switch toUS to true so any disconnections don't affect future transactions
        catch (...)
        {
            toUS = true;
            done();
            Sleep(20000);
        }
    } while (true);
    getchar();
    return 0;
}

//Initializes distance and pi vectors for Bellman-Ford
//    Initializes single source starting spot
//    @returns void
//    @param    none
void initializeSingleSource()
{
    //Fills distance and pi values with essentially infinity
    fill(distances.begin(), distances.end(), 999999);
    fill(pi.begin(), pi.end(), 999999);

    //Creates starting point
    distances[0] = 0;
}

//Method to relax edges
//    @returns void
//    @param    two integers u, v as vertices
//    @param    two dimensional vector of exchange rates between vertices
void relax(int u, int v, vector<vector<double> > &rates)
{
    //Get the weight between the given two edges
    double weight = log10(rates[u][v]);

    //If neither of the currencies will create an extroardinarily large amount of one currency (~500,000+ i.e.)
    if (log10(rates[0][v]) > -1*MAX_RATE && log10(rates[0][u]) > -1*MAX_RATE && 
        log10(rates[0][v]) < MAX_RATE && log10(rates[0][u]) < MAX_RATE)
    {
        //If edge would reduce current distance, use it
        //    Modifies distance and pi vectors as needed
        if (distances[v] > distances[u] - weight && weight != 0)
        {
            distances[v] = distances[u] - weight;
            pi[v] = u;
        }
    }
}

//Uses Bellman-Ford algorithm to search graph for a profitable cycle
//    @returns true if a cycle is found, false if not
//    @param    two dimensional vector of exchange rates between vertices    
bool findCycle(vector<vector<double> > &rates)
{
    //Initialize pi and distance vectors
    initializeSingleSource();

    //Go through each edge once per vertex and relax it
    for (int ii = 0; ii < 99; ii++)
    {
        for (int jj = 0; jj < 100; jj++)
        {
            for (int kk = 0; kk < 100; kk++)
            {
                relax(jj, kk, rates);
            }
        }
    }

    //Go through each edge once more, if can be relaxed, a negative cycle is found
    for (int jj = 0; jj < 100; jj++)
    {
        for (int kk = 0; kk < 100; kk++)
        {
            //Get weight of current edge
            //If edge would reduce path length and if neither of the currencies will 
            //    create an extroardinarily large amount of one currency (~500,000+ i.e.),
            //    a negative cycle has been found
            double weight = log10(rates[jj][kk]);
            if (distances[kk] > distances[jj] - weight && 
                log10(rates[0][kk]) > -1 * MAX_RATE && log10(rates[0][jj]) > -1 * MAX_RATE &&
                log10(rates[0][kk]) < MAX_RATE && log10(rates[0][jj]) < MAX_RATE)
            {
                int test = pi[kk]; //Save current value in pi vector in case path turns out not to be suitable
                pi[kk] = jj; //Update pi vector with current edge

                //Insert into used and cycles the IDs of the
                //    currencies in the cycle by going back through 
                //    the pi vector
                set<int> used;
                int temp = kk;
                do
                {
                    used.insert(temp);
                    cycle.push_back(temp);
                    temp = pi[temp];
                } while (used.find(temp) == used.end());

                //Get rid of excess values in the cycle
                //    These were used to get to the cycle but are not in it
                while (temp != cycle.front())
                {
                    cycle.pop_front();
                }

                //Check how much the cycle should make
                double mult = rates[cycle.front()][cycle.back()]; //Holds the amount the cycle would make
                list<int>::iterator next = cycle.begin(); //Next value in cycle
                list<int>::iterator prev = next; //Previous value in cycle
                next++;
                for (; next != cycle.end(); next++)
                {
                    mult = mult*rates[*next][*prev]; //Go from next to prev because cycle is in reverse order
                    prev = next; //Update prev
                }

                //If the cycle makes enough, reverse it and return true
                //    makes sure a cycle will make enough to not lose money on transactions
                if (mult > 1.01)
                {
                    cycle.reverse();
                    return true;
                }
                //If it doesn't make enough money, clear it, reset pi vector, and start over
                else
                {
                    cycle.clear();
                    pi[kk] = test;

                }
            }
        }
    }
    //If no suitable cycle is found, return false
    return false;
}

//Converts all currencies to USD
//    Used as a recovery function, not in normal operation
//    @returns void
//    @param    vector of current amounts of each currency owned
void convertToUS(vector<double> &amounts)
{
    //Go through all currencies, if significant amount, trade it back to USD
    for (int ii = 0; ii < 100; ii++)
    {
        if (amounts[ii] > 0.01 && ii != 0)
        {
            exchange(ii, amounts[ii], 0);
        }
    }
}