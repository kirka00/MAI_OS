#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <zmq.hpp>
#include <sstream>

using namespace std;
using namespace zmq;

vector<string> split(const string &s)
{
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (ss >> item) tokens.push_back(item);
    return tokens;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cerr << "Usage: computing_node <id> <control_node_address>" << endl;
        return 1;
    }
    int id = stoi(argv[1]);
    string control_adr = argv[2];
    context_t context(1);

    socket_t control_socket(context, ZMQ_REQ);
    control_socket.connect(control_adr);
    control_socket.send(buffer("Ready " + to_string(id)));
    message_t ack;
    if (!control_socket.recv(ack)) {
        cerr << "Error: Failed to receive acknowledgment from control node. Exiting." << endl;
        return 1;
    }

    socket_t worker_socket(context, ZMQ_REP);
    worker_socket.bind("tcp://127.0.0.1:" + to_string(4040 + id));

    map<string, int> dictionary;

    while (true)
    {
        message_t request;
        if (worker_socket.recv(request, recv_flags::none))
        {
            string msg_str = request.to_string();
            string reply_str;

            if (msg_str == "ping") {
                reply_str = "Ok:id: pong";
            } else {
                auto args = split(msg_str);
                if (args.size() == 2) {
                    string name = args[0];
                    int value = stoi(args[1]);
                    dictionary[name] = value;
                    reply_str = "Ok:" + to_string(id);
                } else if (args.size() == 1) {
                    string name = args[0];
                    if (dictionary.count(name)) {
                        reply_str = "Ok:" + to_string(id) + ": " + to_string(dictionary[name]);
                    } else {
                        reply_str = "Ok:" + to_string(id) + ": '" + name + "' not found";
                    }
                } else {
                    reply_str = "Error:id:" + to_string(id) + ": [Custom error] Invalid command format";
                }
            }
            worker_socket.send(buffer(reply_str), send_flags::none);
        }
    }

    return 0;
}