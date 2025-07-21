#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <zmq.hpp>

using namespace std;
using namespace zmq;

struct TreeNode
{
    int id;
    pid_t pid;
    TreeNode *left = 0;
    TreeNode *right = 0;

    TreeNode(int nodeId, pid_t processId) : id(nodeId), pid(processId) {}
};

context_t context(1);
socket_t main_socket(context, ZMQ_REP);
TreeNode *root = 0;
map<int, TreeNode *> nodes;

void delete_tree(TreeNode *node)
{
    if (node == 0)
        return;
    delete_tree(node->left);
    delete_tree(node->right);
    delete node;
}

vector<string> split(const string &s)
{
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (ss >> item)
        tokens.push_back(item);
    return tokens;
}

string send_receive(socket_t &socket, const string &message)
{
    socket.send(buffer(message), send_flags::none);
    message_t reply;
    socket.set(sockopt::rcvtimeo, 2000);
    if (socket.recv(reply, recv_flags::none))
        return reply.to_string();
    return "Error: Node is unavailable";
}

int main()
{
    int port = 4040;
    string adr = "tcp://127.0.0.1:" + to_string(port);
    main_socket.bind(adr);
    string line;
    while (cout << "> " && getline(cin, line))
    {
        if (line == "exit")
            break;
        auto args = split(line);
        if (args.empty())
            continue;
        string command = args[0];
        if (command == "create" && (args.size() == 2 || args.size() == 3))
        {
            int id = stoi(args[1]);
            if (nodes.count(id))
            {
                cout << "Error: Already exists" << endl;
                continue;
            }

            if (args.size() == 3 && root != 0)
            {
                int parentId = stoi(args[2]);
                if (!nodes.count(parentId))
                {
                    cout << "Error: Parent not found" << endl;
                    continue;
                }
                socket_t parent_socket(context, ZMQ_REQ);
                parent_socket.set(sockopt::rcvtimeo, 2000);
                parent_socket.connect("tcp://127.0.0.1:" + to_string(4040 + parentId));
                parent_socket.send(buffer("ping"), send_flags::none);
                message_t parent_reply;
                if (!parent_socket.recv(parent_reply, recv_flags::none))
                {
                    cout << "Error: Parent is unavailable" << endl;
                    continue;
                }
            }
            pid_t pid = fork();
            if (pid == -1)
            {
                perror("fork");
                cout << "Error: [Custom error] Failed to create process" << endl;
            }
            else if (pid == 0)
            {
                execl("./computing_node", "computing_node", to_string(id).c_str(), adr.c_str(), (char *)NULL);
                perror("execl");
                exit(1);
            }
            else
            {
                message_t request;
                if (!main_socket.recv(request, recv_flags::none)) {
                    cerr << "Error" << endl;
                }
                main_socket.send(buffer("OK"), send_flags::none);
                TreeNode *newNode = new TreeNode(id, pid);
                nodes[id] = newNode;
                if (root == 0)
                    root = newNode;
                else
                {
                    TreeNode *current = root;
                    TreeNode *parent = 0;
                    while (current != 0)
                    {
                        parent = current;
                        if (id < current->id)
                            current = current->left;
                        else
                            current = current->right;
                    }
                    if (id < parent->id)
                        parent->left = newNode;
                    else
                        parent->right = newNode;
                }
                cout << "Ok: " << pid << endl;
            }
        }
        else if (command == "exec" && args.size() >= 2)
        {
            int id = stoi(args[1]);
            string exec_params;
            if (args.size() >= 3)
            {
                for (size_t i = 2; i < args.size(); ++i)
                {
                    exec_params += args[i] + " ";
                }
            }
            else
            {
                cout << "> ";
                getline(cin, exec_params);
            }
            // проверяем существование узла
            if (!nodes.count(id))
            {
                cout << "Error:" << id << ": Not found" << endl;
                continue;
            }
            socket_t socket(context, ZMQ_REQ);
            socket.connect("tcp://127.0.0.1:" + to_string(4040 + id));
            string result = send_receive(socket, exec_params);

            if (result == "Error: Node is unavailable")
            {
                cout << "Error:" << id << ": Node is unavailable" << endl;
            }
            else
            {
                cout << result << endl;
            }
        }
        else if (command == "pingall")
        {
            string unavailable_nodes;
            for (auto const &[id, node] : nodes)
            {
                socket_t ping_socket(context, ZMQ_REQ);
                ping_socket.set(sockopt::rcvtimeo, 2000);
                ping_socket.connect("tcp://127.0.0.1:" + to_string(4040 + id));
                ping_socket.send(buffer("ping"), send_flags::none);
                message_t reply;
                if (!ping_socket.recv(reply, recv_flags::none))
                {
                    unavailable_nodes += to_string(id) + ";";
                }
            }
            if (unavailable_nodes.empty())
                cout << "Ok: -1" << endl;
            else
            {
                if (!unavailable_nodes.empty())
                    unavailable_nodes.pop_back();
                cout << "Ok: " << unavailable_nodes << endl;
            }
        }
        else
        {
            cout << "Error: Unknown command" << endl;
        }
    }
    for (auto const &[id, node] : nodes)
    {
        kill(node->pid, SIGTERM);
    }
    delete_tree(root);
    return 0;
}