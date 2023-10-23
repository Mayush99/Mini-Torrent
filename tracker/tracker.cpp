#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
using namespace std;
#define filesize 524288

struct user
{
    string password;
    bool login;
    int port;
    string ip;
    user(string pwd, bool log, int prt, string i)
    {
        password = pwd;
        login = log;
        port = prt;
        ip = i;
    }
};

struct filemetadata
{
    // string filename;
    unordered_map<string, string> userpath; // userid   path
    string sha;
    string noOfChunks;
    string fsize;
    // bool share;
    filemetadata(string a, string b, string s, string nc, string fs)
    {
        userpath.insert({a, b});
        sha = s;
        noOfChunks = nc;
        fsize = fs;
    }
};

unordered_map<string, user *> userlog;
unordered_map<string, pair<string, vector<string>>> grpinfo; // grpid, <admin, users>
unordered_map<string, vector<string>> pending_req;
unordered_map<string, vector<string>> grpfile; //<grpid, filenames>
unordered_map<string, filemetadata *> filemd;  // filename , filemetadata>

void createusr(string cmd, int fd)
{
    vector<string> com;
    string tmp;
    cout << cmd << "\n";
    for (int i = 0; i < cmd.size(); i++)
    {
        if (cmd[i] == ',')
        {
            com.push_back(tmp);
            tmp = "";
        }
        else
        {
            tmp += cmd[i];
        }
    }
    com.push_back(tmp);
    // cout << com[3] << "\n";
    if (com[0] == "create_user")
    {
        user *usr = new user(com[2], false, stoi(com[4]), com[3]);
        userlog.insert({com[1], usr});
        send(fd, "User created Successfully", 20, 0);
    }
    // cout << userlog[com[1]]->ip << " " << userlog[com[1]]->login << " " << userlog[com[1]]->password << " " << userlog[com[1]]->port << endl;

    else if (com[0] == "login")
    {
        auto itr = userlog.find(com[1]);
        if (itr != userlog.end())
        {
            if (com[2] == userlog[com[1]]->password)
            {
                // need dynamic ip.........
                userlog[com[1]]->ip = "127.0.0.1";
                userlog[com[1]]->port = stoi(com[4]);
                userlog[com[1]]->login = true;
                send(fd, "LogIn Successful", 16, 0);
            }
        }
        else
        {
            send(fd, "User not found", 15, 0);
        }
    }

    else if (com[0] == "create_group")
    {
        if (grpinfo.find(com[2]) == grpinfo.end())
        {
            pair<string, vector<string>> pr;
            vector<string> vec;
            vec.push_back(com[1]);
            pr.first = com[1];
            pr.second = vec;

            grpinfo.insert({com[2], pr});
            send(fd, "Group created successfully", 27, 0);
        }
        else
        {
            send(fd, "Group already exist", 20, 0);
        }
    }

    else if (com[0] == "join_group")
    {
        // join-grp    usrid    grpid
        bool flag = 0;
        if (grpinfo.find(com[2]) != grpinfo.end())
        {
            for (int i = 0; i < grpinfo[com[2]].second.size(); i++)
            {
                if (com[1] == grpinfo[com[2]].second[i])
                {
                    flag = 1;
                    break;
                }
            }
            if (flag)
            {
                send(fd, "User already exist in group", 28, 0);
            }
            else
            {
                // grpinfo[com[2]].second.push_back(com[1]);
                pending_req[com[2]].push_back(com[1]);
                send(fd, "Joined request send successfully", 33, 0);
            }
        }
        else
        {
            send(fd, "Group does not exist", 21, 0);
        }
    }

    else if (com[0] == "leave_group")
    {
        bool flag = 0;
        if (grpinfo.find(com[2]) != grpinfo.end())
        {
            int i = 0;
            for (; i < grpinfo[com[2]].second.size(); i++)
            {
                if (com[1] == grpinfo[com[2]].second[i])
                {
                    flag = 1;
                    break;
                }
            }
            if (flag)
            {
                auto iter = grpinfo[com[2]].second.begin() + i;
                grpinfo[com[2]].second.erase(iter);

                // if admin left new group admin/****************/
                if (grpinfo[com[2]].first == com[1])
                {
                    grpinfo[com[2]].first = grpinfo[com[2]].second[0];
                }
                send(fd, "User left group", 16, 0);
            }
            else
            {
                send(fd, "User not present in group", 26, 0);
            }
        }
        else
        {
            send(fd, "Group doesn,t exist", 16, 0);
        }
    }

    else if (com[0] == "list_requests")
    {
        if (pending_req.find(com[2]) == pending_req.end())
        {
            send(fd, "Group doesn,t exist", 16, 0);
        }
        else
        {
            for (int i = 0; i < pending_req[com[2]].size(); i++)
            {
                send(fd, (pending_req[com[2]][i] + "\n").c_str(), (pending_req[com[2]][i] + "\n").size(), 0);
            }
        }
    }

    else if (com[0] == "list_groups")
    {
        for (const auto &grp : grpinfo)
        {
            send(fd, grp.first.c_str(), grp.first.size(), 0);
        }
    }

    else if(com[0]=="stop_share"){
        if(grpfile.find(com[2])!=grpfile.end()){
            for (auto it = grpfile[com[2]].begin(); it != grpfile[com[2]].end(); ++it){
                if(*it==com[3]){
                    grpfile[com[2]].erase(it);
                }
            }
            filemd.erase(com[3]);
        }
    }

    // else if()
    
    else if (com[0] == "accept_request")
    {
        if (grpinfo.find(com[2]) != grpinfo.end())
        {
            if (grpinfo[com[2]].first == com[1])
            {
                grpinfo[com[2]].second.push_back(com[3]);
                send(fd, "Join request accepted", 22, 0);
            }
            else
            {
                send(fd, "Only admin can accept request", 30, 0);
            }
        }
        else
        {
            send(fd, "Group doesn't exist", 16, 0);
        }
        char ch[9];
        int n;
        if ((n = read(fd, ch, 9)) > 0)
        {
            if (string(ch) == "accepted")
            {
                pending_req[com[2]].clear();
            }
        }
    }

    else if (com[0] == "upload_file")
    {
        if (grpinfo.find(com[2]) == grpinfo.end())
        {
            send(fd, "Group doesn't exist", 20, 0);
        }
        else
        {
            bool flag = 0;
            int i = 0;
            for (; i < grpinfo[com[2]].second.size(); i++)
            {
                if (com[1] == grpinfo[com[2]].second[i])
                {
                    // cout << "228" << grpinfo[com[2]].second[i] << endl;
                    flag = 1;
                    break;
                }
            }

            if (flag)
            {
                string filecmd = "";
                for (int i = com[3].size() - 1; i >= 0; i--)
                {
                    if (com[3][i] == '/')
                        break;
                    filecmd = com[3][i] + filecmd;
                }
                cout << "filecmd " << filecmd << endl;
                bool fileFlag = false;
                if (grpfile.find(com[2]) != grpfile.end())
                {
                    for (int i = 0; i < grpfile[com[2]].size(); i++)
                    {
                        if (grpfile[com[2]][i] == filecmd)
                        {
                            cout << " 295 " << grpfile[com[2]][i] << endl;
                            fileFlag = true;
                            break;
                        }
                    }
                }
                if (fileFlag)
                {
                    filemd[filecmd]->userpath.insert({com[1], com[3]});
                    cout << " 303 " << filemd[filecmd]->userpath[com[1]] << endl;
                    send(fd, "File updated successfully", 26, 0);
                }
                else
                {
                    vector<string> vec;
                    grpfile.insert({com[2], vec});
                    grpfile[com[2]].push_back(filecmd);
                    filemetadata *fmd = new filemetadata(com[1], com[3], com[4], com[6], com[5]);
                    filemd.insert({filecmd, fmd});
                    send(fd, "File uploaded successfully", 27, 0);
                }
            }
            else
            {
                send(fd, "User not part of group", 23, 0);
            }
        }
    }

    else if (com[0] == "list_files")
    {
        string msg = "";
        if (grpinfo.find(com[2]) != grpinfo.end())
        {
            for (int i = 0; i < grpfile[com[2]].size(); i++)
            {
                msg += grpfile[com[2]][i] + "\n";
            }
            send(fd, msg.c_str(), msg.size(), 0);
        }
        else
        {
            send(fd, "Group does not exist", 21, 0);
        }
    }

    else if (com[0] == "download_file")
    {
        if (grpinfo.find(com[2]) != grpinfo.end())
        {
            bool flag = false;
            for (int i = 0; i < grpfile[com[2]].size(); i++)
            {
                if (grpfile[com[2]][i] == com[3])
                {
                    flag = true;
                    break;
                }
            }
            if (flag)
            {
                // cerr << "File found for downloading\n";
                filemetadata *flmd = filemd[com[3]];
                string filedata = filemd[com[3]]->noOfChunks + "," + filemd[com[3]]->fsize + "," + filemd[com[3]]->sha + ",";

                // send the above string.....
                

                vector<string> keys;
                vector<string> fpath;
                for (auto it = filemd[com[3]]->userpath.begin(); it != filemd[com[3]]->userpath.end(); it++)
                {
                    keys.push_back(it->first);
                    fpath.push_back(filemd[com[3]]->userpath[it->first]);
                }
                string fildat = filedata;
                for (int i = 0; i < keys.size(); i++)
                {
                    //                userid           ip                          port                                       filename       filepath
                    fildat += keys[i] + "," + userlog[keys[i]]->ip + "," + to_string(userlog[keys[i]]->port) + "," + com[3] + "," + fpath[i] + ",";

                    // send the above message as well......
                }
                cout << "337 Sending concat info.....\n";
                // cout << filemd["info.txt"]->userpath["may"] << endl;
                send(fd, fildat.c_str(), fildat.size(), 0);
                cout << fildat << endl;
                cout << "339 Sent info......\n";

                // send the message to stop data transfer.....
                // send(fd, "Data transfer over", 19, 0);
            }
            else
            {
                send(fd, "File don't exist", 17, 0);
            }
        }
        else
        {
            send(fd, "Group don't exist", 18, 0);
        }
    }

    else if (com[0] == "logout")
    {
        userlog[com[1]]->login = false;
        send(fd, "Logout successful", 18, 0);
    }
}

void handle_request(int fd)
{
    int n;
    char ch[filesize];
    while (true)
    {
        n = read(fd, ch, filesize);
        // cout << "Client: " << string(ch) << endl;
        // if (n > 0)
        // {
        //     cout << "Enter message: \n";
        //     bzero(ch, filesize);
        //     cin >> ch;
        //     send(fd, ch, strlen(ch), 0);
        //     memset(ch, 0, filesize);
        // }
        if (n > 0)
        {
            createusr(string(ch), fd);
            // send(fd, "Command Recieved", 16, 0);
        }
        memset(ch, 0, filesize);
    }
    close(fd);
}

void server(int port)
{
    char ch[filesize];
    bzero(ch, filesize);
    int rd, wrt, opt = 0;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        cout << "error while creating socket \n";
        exit(1);
    }
    else
    {
        cout << ">Socket creation successful\n";
    }

    sockaddr_in server_addr, client_addr;
    // int port = atoi(argv[1]);
    explicit_bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind() the socket to current IP address
    cout << server_addr.sin_addr.s_addr << endl;
    int bnd = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bnd == -1)
    {
        cout << "error while binding \n";
        exit(1);
    }
    cout << ">Socket bind successful\n";

    /*listen to listen to the incomming connections
    a backlog queue will be created for connection requests
    and accept() will accept the requests.
    5 here is the queue size for client requests..... */

    if (listen(fd, 5) < 0)
    {
        cout << "error while listening \n";
        exit(1);
    }
    cout << ">Socket is listening successfully...\n";

    socklen_t client_len = sizeof(client_addr);
    while (1)
    {
        int acpt_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
        if (acpt_fd == -1)
        {
            cout << "error while accepting requests \n";
            exit(1);
        }
        cout << "Server got connected from " << inet_ntoa(client_addr.sin_addr) << " port " << ntohs(client_addr.sin_port) << endl;

        // create a thread here to handle incoming requests***************

        thread thrd(handle_request, acpt_fd);
        thrd.detach();
    }
}

int main(int argc, char **argv)
{
    ifstream inp;
    inp.open(argv[1]);
    if (!inp.is_open())
    {
        cout << "tracker_info.txt file not opening\n";
        exit(1);
    }
    vector<string> vec;
    string str;
    while (getline(inp, str, ' '))
    {
        vec.push_back(str);
    }
    string tip, tport;
    for (int i = 0; i < vec.size(); i += 3)
    {
        if(vec[i]==argv[2]){
            tip = vec[i + 1];
            tport = vec[i + 2];
        }
    }
    int sport = atoi(tport.c_str());
    thread thrd(server, sport); // we need thread here***************
    thrd.detach();
    while (1){
        string a;
        cin >> a;
        if(a=="quit"){
            exit(1);
        }
    }

    return 0;
}