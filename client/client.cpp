// create a peer which acts as a client and server at the same time
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
using namespace std;
#define filesize 524288

struct user
{
    string userid;
    bool login;
    int port;
    user(string id, int prt, bool log = false)
    {
        userid = id;
        login = log;
        port = prt;
    }
};

struct filechunk
{
    string filename;
    string filepath;
    long long fsize;
    int chunks;
    string bitmap;
    int lastchunksize;
    string *sha;

    filechunk(string fn, string fp, long long fs, int nc, string bm, int lcs, string *s)
    {
        filename = fn;
        filepath = fp;
        fsize = fs;
        chunks = nc;
        bitmap = bm;
        lastchunksize = lcs;
        sha = s;
    }
};

user *usr;
unordered_map<string, filechunk *> filechunkmap; //<filepath, filechunk>

void handle_request(int fd)
{
    int n;
    char ch[filesize];
    // n = read(fd, ch, filesize);
    // string df = string(ch);
    memset(ch, 0, filesize);

    n = read(fd, ch, filesize);
    // cout << "60 Peer as server: " << string(ch) << endl;
    if (n > 0)
    {
        string cmd = string(ch);
        memset(ch, 0, filesize);
        vector<string> finfo;
        string fname, fpath, temp = "";
        // cout << "70  " << cmd << endl;
        for (int i = 0; i < cmd.size(); i++)
        {
            if (cmd[i] == ',')
            {
                finfo.push_back(temp);
                temp = "";
            }
            else
                temp += cmd[i];
        }
        finfo.push_back(temp);
        fname = finfo[1];
        fpath = finfo[2];
        cout << '\"' << finfo[0]<<"  " << fname<<"  " << fpath << "\"\n";
    
        if (finfo[0] != "download_file")
        {
            cout << "getting_data bitmap...........\n";
            if(filechunkmap.find(fname)!=filechunkmap.end()){
            string bm = filechunkmap[fname]->bitmap;
            send(fd, bm.c_str(), bm.size(), 0);
            }
        }
        else
        {
            // send sha....
            cout << "downloading.................\n";
            int offset = stoi(finfo[3]);
            int file = open(fpath.c_str(), O_RDONLY);
            n = pread(file, ch, filesize, offset*filesize);
            send(fd, ch, n, 0);
        }
        // cout << "Enter message: \n";
        // cin >> ch;
        // send(fd, ch, strlen(ch), 0);
        // memset(ch, 0, filesize);
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
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind() the socket to current IP address
    // cout << server_addr.sin_addr.s_addr << endl;
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

    if (listen(fd, 5000) < 0)
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

string getsha(string path)
{
    char buffer[filesize], SHA_ARR[SHA_DIGEST_LENGTH];
    int readByte;

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return "-1";
    else
    {
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        while ((readByte = read(fd, buffer, filesize)) > 0)
            SHA1_Update(&ctx, buffer, readByte);

        SHA1_Final((unsigned char *)SHA_ARR, &ctx);
        string str = "";
        for (int i = 0; i < 20; i++)
        {
            if (SHA_ARR[i] != '\0')
                str += SHA_ARR[i];
            else
                str += '?';
        }
        return str;
    }
}

pair<string, int> getchunksha(string path)
{
    char buffer[filesize], SHA_ARR[SHA_DIGEST_LENGTH];
    int readByte;

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return make_pair("-1", -1);
    else
    {
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        if ((readByte = read(fd, buffer, filesize)) > 0)
            SHA1_Update(&ctx, buffer, readByte);

        SHA1_Final((unsigned char *)SHA_ARR, &ctx);
        string str = "";
        for (int i = 0; i < 20; i++)
        {
            if (SHA_ARR[i] != '\0')
                str += SHA_ARR[i];
            else
                str += '?';
        }
        return make_pair(str, readByte);
    }
}

void download_file(int fd, string dest, string b, int offset)
{
    char ch[filesize];
    memset(ch, 0, filesize);
    // cout << c << "    " << b << endl;
    string cmdir = dest + "/" + b;
    int file = open((cmdir).c_str(), O_CREAT | O_WRONLY, 0777);
    cout << "Downloading..................   " << endl;
    
    memset(ch, 0, filesize);
    // reading sha  ->  read(fd, ch, filesize);
    // memset(ch, 0, filesize);
    int n = read(fd, ch, filesize);
    pwrite(file, ch, n, offset*filesize);
}

int main(int argc, char **argv)
{
    string prt = string(argv[1]);
    int i = 0;
    for (; i < prt.size(); i++)
    {
        if (prt[i] == ':')
            break;
    }
    string sip = prt.substr(i + 1, prt.size());
    prt = prt.substr(0, i);
    int sport = stoi(sip);
    thread thrd(server, sport); // we need thread here***************
    // cout << sport << endl;
    // thrd.detach();

    char ch[filesize];
    sockaddr_in server_addr;
    hostent *server;
    int port = 20001;
    // int port = 5000;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        cout << "socket creation failed \n";
        exit(1);
    }
    bzero((char *)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    // we have to create a thread to implement multiple client calls**************
    int cnct = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (cnct < 0)
    {
        cout << "Connection failed \n";
        exit(1);
    }

    while (true)
    {
        // user *usr = new user();
        string msg, a, b;
        int n;
        while (true)
        {
            memset(ch, 0, filesize);
            cin >> msg;
            // create_user userid password
            if (msg == "create_user" || msg == "login")
            {
                cin >> a >> b;
                msg += "," + a + "," + b + ",127.0.0.1," + to_string(sport);
                // cout << msg << endl;
                n = send(fd, msg.c_str(), msg.size(), 0);
                if (n <= 0)
                    cout << "Command not sent\n";
                else
                {
                    n = read(fd, ch, filesize);
                    // if(string(ch).substr(0,n)=="User created successfully")
                    if (string(ch).substr(0, n) == "LogIn Successful")
                    {
                        vector<string> com;
                        string tmp;
                        for (int i = 0; i < msg.size(); i++)
                        {
                            if (msg[i] == ',')
                            {
                                com.push_back(tmp);
                                tmp = "";
                            }
                            else
                            {
                                tmp += msg[i];
                            }
                        }
                        com.push_back(tmp);
                        usr = new user(com[1], port, true);
                        cout << ch << "\n";
                    }
                }
                memset(ch, 0, filesize);
            }

            else if (msg == "create_group" || msg == "join_group" || msg == "leave_group" || msg == "list_requests")
            {
                if (usr->login)
                {
                    cin >> a;
                    msg += "," + usr->userid + "," + a;
                    n = send(fd, msg.c_str(), strlen(msg.c_str()), 0);
                    if (n <= 0)
                        cout << "Command not sent\n";
                    else
                    {
                        n = read(fd, ch, filesize);
                        cout << ">> " << ch << "\n";
                    }
                }
            }
            else if (msg == "list_groups" || msg == "logout")
            {
                if (usr->login)
                {
                    msg += "," + usr->userid;
                    n = send(fd, msg.c_str(), msg.size(), 0);
                    if (n <= 0)
                        cout << "Command not sent\n";
                    else
                    {
                        n = read(fd, ch, filesize);
                        cout << ">> " << ch << "\n";
                    }
                    if (msg == "logout")
                    {
                        usr->login = false;
                    }
                }
            }

            else if (msg == "accept_request")
            {
                cin >> a >> b;
                msg += "," + usr->userid + "," + a + "," + b;
                n = send(fd, msg.c_str(), msg.size(), 0);
                if (n <= 0)
                    cout << "Command not sent\n";
                else
                {
                    n = read(fd, ch, filesize);
                    cout << ">> " << ch << "\n";
                    send(fd, "accepted", 9, 0);
                }
            }

            else if (msg == "upload_file")
            {
                if (usr->login)
                {
                    cin >> a >> b; // file path      grpid
                    if (realpath(a.c_str(), NULL))
                    { //***debug***
                        string path = realpath(a.c_str(), NULL);
                        string sha = getsha(path);
                        struct stat stfile;
                        stat(path.c_str(), &stfile);
                        long long fsize = (long long)(stfile.st_size);
                        int noOfChunks = ceil((double)fsize / (double)filesize);

                        msg += "," + usr->userid + "," + b + "," + a + "," + sha + "," + to_string(fsize) + ","+ to_string(noOfChunks);
                        n = send(fd, msg.c_str(), msg.size(), 0);
                        if (n <= 0)
                            cout << "Command not sent\n";
                        else
                        {
                            n = read(fd, ch, filesize);
                            cout << ">> " << ch << "\n";
                            if (string(ch) == "File uploaded successfully" || string(ch) == "File updated successfully")
                            {
                                string chunksha[noOfChunks];
                                string bmap = "";
                                int lcs = -1;
                                for (int i = 0; i < noOfChunks; i++)
                                {
                                    bmap += '1';
                                    chunksha[i] = getchunksha(path).first;
                                    lcs = getchunksha(path).second;
                                }
                                string fn = "";
                                for (int i = path.size() - 1; i >= 0; i--)
                                {
                                    if (path[i] == '/')
                                        break;
                                    fn = path[i] + fn;
                                }
                                filechunk *fc = new filechunk(fn, path, fsize, noOfChunks, bmap, lcs, chunksha);
                                filechunkmap.insert({fn, fc});
                                cout << "425 " << fn << endl;
                            }
                        }
                    }
                    else
                    {
                        cout << "file doesn't exist\n";
                    }
                }
                else
                {
                    cout << "Login to the application\n";
                }
            }

            else if (msg == "list_files")
            {
                cin >> a;
                msg += "," + usr->userid + "," + a;
                n = send(fd, msg.c_str(), msg.size(), 0);
                if (n <= 0)
                    cout << "Command not sent\n";
                else
                {
                    n = read(fd, ch, filesize);
                    if (n <= 0)
                    {
                        cout << "No sharable files\n";
                    }
                    else
                    {
                        cout << ">> " << ch << "\n";
                    }
                }
            }

            else if(msg == "stop_share"){
                cin >> a >> b; //  grp id     fname
                msg += "," + usr->userid + ',' + a + ',' + b;
                memset(ch, 0, filesize);
                send(fd, msg.c_str(), msg.size(), 0);
            }

            else if(msg=="show_downloads"){
            }

            else if (msg == "download_file")
            {
                string c;
                cin >> a >> b >> c; // grp id    filename    destPath
                string dest = c;
                msg += "," + usr->userid + "," + a + "," + b;
                n = send(fd, msg.c_str(), msg.size(), 0);
                if (n <= 0)
                {
                    cout << ">> Command not sent\n";
                }
                else
                {
                    n = read(fd, ch, filesize);
                    cout << "473 " << string(ch) << "\n"
                         << endl;
                    if (n > 0)
                    {

                        // uid_t uid = getuid();
                        // passwd *pw = getpwuid(uid);
                        // n = read(fd, ch, filesize);
                        if (string(ch) == "Group don't exist" || string(ch) == "File don't exist")
                        {
                            cout << ">> " << string(ch) << "\n";
                        }
                        else
                        {
                            string c = string(ch);
                            memset(ch, 0, filesize);
                            vector<string> fmd;
                            string cmdstr = "";
                            cout << "490 " << c.size() << "\n";
                            for (int i = 0; i < c.size(); i++)
                            {
                                if (c[i] == ',')
                                {
                                    fmd.push_back(cmdstr);
                                    cmdstr = "";
                                }
                                else
                                {
                                    cmdstr += c[i];
                                }
                            }
                            // fmd.push_back(cmdstr);
                            pair<string, string> usrpath;
                            pair<string, string> iport;
                            vector<pair<string, string>> up; // userid  filepath
                            vector<pair<string, string>> ip; // ip   port
                            string fname;
                            cout << "509 receiving frm tracker" << endl;
                            memset(ch, 0, filesize);
                            // n = read(fd, ch, filesize);
                            if (n > 0)
                            {
                                string c = string(ch);
                                vector<string> cmd = fmd;
                                // string cmdstr = "";
                                cout << "515 " << c << endl;
                                // for (int i = 0; i < c.size(); i++)
                                // {
                                //     if (c[i] == ',')
                                //     {
                                //         cmd.push_back(cmdstr);
                                //         cmdstr = "";
                                //     }
                                //     else
                                //     {
                                //         cmdstr += c[i];
                                //     }
                                // }
                                cout << "531   "<<cmd.size() << endl;
                                for (int i = 3; i < cmd.size(); i += 5)
                                {
                                    usrpath.first = cmd[i];
                                    usrpath.second = cmd[i + 4];
                                    iport.first = cmd[i + 1];
                                    iport.second = cmd[i + 2];
                                    up.push_back(usrpath);
                                    ip.push_back(iport);
                                    fname = cmd[i + 3];
                                }
                                // first see if all the chunks are available....
                                int nchnk = stoi(fmd[0]);
                                bool chnkavl[nchnk] = {false};
                                cout << "542 " << nchnk << endl;
                                vector<pair<string, string>> idbit;
                                cout << "546 " << up.size() << endl;
                                for (int i = 0; i < up.size(); i++)
                                {
                                    int sock = socket(AF_INET, SOCK_STREAM, 0);
                                    if (sock == -1)
                                    {
                                        cout << ">> socket creation failed for download\n";
                                        exit(1);
                                    }
                                    cout << "554 socket created\n";
                                    sockaddr_in server_addr;
                                    hostent *server;
                                    bzero((char *)&server_addr, sizeof(server_addr));
                                    server_addr.sin_family = AF_INET;

                                    cout << ip[i].first << "  560  " << ip[i].second << endl;

                                    inet_pton(AF_INET, ip[i].first.c_str(), &server_addr.sin_addr);
                                    // server_addr.sin_addr.s_addr = ip[i].first;
                                    server_addr.sin_port = htons(stoi(ip[i].second));
                                    int cnct = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
                                    if (cnct < 0)
                                    {
                                        cout << "Connection failed while downloading \n";
                                        exit(1);
                                    }
                                    send(sock, ("get_data," + fname + "," + up[i].second).c_str(), ("get_data," + fname + "," + up[i].second).size(), 0);
                                    memset(ch, 0, filesize);
                                    n = read(sock, ch, filesize);
                                    cout << "bitmaps " << ch << endl;
                                    idbit.push_back({up[i].first, string(ch)});
                                }
                                bool dwnld = true;
                                for (int it = 0; it < idbit.size(); it++)
                                {
                                    cout << "582 idbit:  " << idbit[it].first << endl;
                                    for (int j = 0; j < nchnk; j++)
                                    {
                                        if (idbit[it].second[j] == '1')
                                        {
                                            chnkavl[j] = true;
                                        }
                                    }
                                }
                                for (int i = 0; i < nchnk; i++)
                                {
                                    if (chnkavl[i] == false)
                                        dwnld = false;
                                }
                                if (dwnld)
                                {
                                    // code for downloading here......
                                    cout << "599 chunk dwload start idbit size:" <<idbit.size()<< endl;
                                    int id = 0;

                                    cout << ip[0].first << " : " << ip[0].second << endl;
                                    cout << ip[1].first << " : " << ip[1].second << endl;
                                    
                                    for (int i = 0; i < nchnk; i++)
                                    {
                                        for (int it = 0; it < idbit.size(); it++)
                                        {
                                            if (idbit[(id + it) % idbit.size()].second[i] == '1')
                                            {
                                                int sock = socket(AF_INET, SOCK_STREAM, 0);
                                                if (sock == -1)
                                                {
                                                    cout << ">> socket creation failed for download\n";
                                                    exit(1);
                                                }
                                                sockaddr_in server_addr;
                                                hostent *server;
                                                bzero((char *)&server_addr, sizeof(server_addr));
                                                server_addr.sin_family = AF_INET;
                                                inet_pton(AF_INET, ip[(id + it) % idbit.size()].first.c_str(), &server_addr.sin_addr);
                                                server_addr.sin_port = htons(stoi(ip[(id + it) % idbit.size()].second));
                                                int cnct = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
                                                if(cnct<0){
                                                    exit(1);
                                                }
                                                cout << "628 download_file," + fname + "," + up[(id + it) % idbit.size()].first +"," + up[(id + it) % idbit.size()].second + "," + to_string(i) << endl;
                                                send(sock, ("download_file," + fname + "," + up[(id + it) % idbit.size()].second + "," + to_string(i)).c_str(), ("download_file," + fname + "," + up[(id + it) % idbit.size()].second + "," + to_string(i)).size(), 0);
                                                id = it++;
                                                thread thr(download_file, sock, dest,b, i);
                                                thr.detach();
                                                break;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    cout << ">> File can't be downloaded as not all chunks are present.\n";
                                }

                                // string cmdir = c + "/" + b;
                                // int file;
                                // file = open((cmdir).c_str(), O_CREAT | O_WRONLY, 0777);
                                // while ((n = read(fd, ch, filesize)) > 0)
                                // {
                                // }
                            }
                            else
                            {
                                cout << "Nothing is read\n";
                            }
                        }
                    }
                }

                memset(ch, 0, filesize);
            }
        }
        return 0;
    }
}