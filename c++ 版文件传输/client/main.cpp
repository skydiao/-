#include "connect.hpp"
#include <iostream>

using namespace std;

int main()
{
    string addr_client("172.11.0.117");
    Connect client(7788,addr_client);
    while(1)
    {
        client.make_orders();
    }
}