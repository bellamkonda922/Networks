#include <bits/stdc++.h>
using namespace std;
int N = 8;       // Number of ports
int B = 4;       // Buffer Size at each port
double p = 0.5;  // prob with packet generated at each port
int K = 1* N; // output buffer size
int T = 10000;   // Max Timeslots
string filename = "output.txt";
string schess = "INQ";
enum SCHEDULE
{
    INQ,
    KOUQ,
    ISLIP
};
SCHEDULE sched = KOUQ; // scheduling policy
int delay = 0;        // to know total delay
int transcnt = 0;     // total transmitted pkts count
int dropcnt = 0;      // total dropped pkts
vector<int> delays;   // store delays

/* Packet representation contains a I/P num & O/P num and time created and time transmitted*/
class Packet
{
public:
    int ip_num;
    int op_num;
    int time_created;
    int time_transmitted;
};
//Data Structures to represent input & output port conatins a queue for buffering/
class input_Port
{
public:
    queue<Packet *> ip_buffer;
};

class output_Port
{
public:
    queue<Packet *> op_buffer;
};

// generate 1 with p probability used for scheduling
int randd(double p)
{
    double ans = (double)rand() / RAND_MAX;
    return ans < p;
}

bool comp(Packet *i1, Packet *i2)
{
    return (i1->time_created < i2->time_created);
}

// sorting queue items
void sort_queue(queue<Packet *> &q)
{

    vector<Packet *> temp;
    while (q.size())
    {
        temp.push_back(q.front());
        q.pop();
    }
    sort(temp.begin(), temp.end(), comp);

    for (auto i : temp)
    {
        q.push(i);
    }
}

int main(int argc, char **argv)
{
    // Taking Arguments from the command line
    ofstream outfile;
    srand(time(0));
    for (int i = 0; i < argc; i++)
    {
        string temp;
        temp = argv[i];
        if (temp == "-N")
        {
            temp = argv[i + 1];
            N = stoi(temp);
            i++;
        }
        else if (temp == "-B")
        {
            temp = argv[i + 1];
            B = stoi(temp);
            i++;
        }
        else if (temp == "-out")
        {
            temp = argv[i + 1];
            filename = temp + ".txt";
        }
        else if (temp == "-p")
        {
            temp = argv[i + 1];
            p = stod(temp);
        }
        else if (temp == "-queue")
        {
            temp = argv[i + 1];
            schess = temp;
            if (temp == "INQ")
                sched = INQ;
            else if (temp == "KOUQ")
                sched = KOUQ;
            else if (temp == "ISLIP")
                sched = ISLIP;
        }
        else if (temp == "-K")
        {
            temp = argv[i + 1];
            double d = 0.6;
            d = stod(temp);
            K = d * N;
            //cout << K << endl;
        }
        else if (temp == "-T")
        {
            temp = argv[i + 1];
            T = stoi(temp);
        }
    }
    vector<input_Port *> ipports;    // N input ports
    vector<output_Port *> opports;   // N output Ports
    vector<int> grantpointer(N, 0);  // Grant pointers at output ports
    vector<int> acceptpointer(N, 0); // request pointers at input ports
    outfile.open(filename);

    
    /*Initialising*/
    for (int i = 0; i < N; i++)
    {
        input_Port *ipp = new input_Port();
        ipports.push_back(ipp);
    }
    for (int i = 0; i < N; i++)
    {
        output_Port *opp = new output_Port();
        opports.push_back(opp);
    }
    int t = 0; // time slots
    int linkutil[N][N];
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            linkutil[i][j] = 0;
        }
    }
    while (t < T)
    {
        // Packet Generation
        int ipcnt = 0;
        for (auto i : ipports)
        {
            if (randd(p))
            {

                Packet *pkt = new Packet;
                pkt->ip_num = ipcnt;
                pkt->time_created = t;
                pkt->op_num = rand() % N;
                if (i->ip_buffer.size() < B)
                {
                    i->ip_buffer.push(pkt);
                }
            }
            ipcnt++;
        }
        map<int, set<int>> contention; // used to store contentions

        switch (sched)
        {
        case INQ:
            // Packet Scheduling
            for (auto i : ipports)
            {

                if (i->ip_buffer.size() > 0)
                {
                    Packet *headpkt = i->ip_buffer.front();
                    contention[headpkt->op_num].insert(headpkt->ip_num);
                }
            }
            for (auto m : contention)
            {
                if (m.second.size() == 1)
                {
                    auto it = m.second.begin();
                    int ipnum = *it;
                    Packet *sentpkt = ipports[ipnum]->ip_buffer.front();
                    ipports[ipnum]->ip_buffer.pop();
                    opports[m.first]->op_buffer.push(sentpkt);
                }
                else if (m.second.size() > 1)
                {
                    int i = rand() % (m.second.size());
                    auto it = m.second.begin();
                    advance(it, i);
                    int ipnum = *it;
                    Packet *sentpkt = ipports[ipnum]->ip_buffer.front();
                    ipports[ipnum]->ip_buffer.pop();
                    opports[m.first]->op_buffer.push(sentpkt);
                }
            }
            // transmission;
            for (auto op : opports)
            {
                if (op->op_buffer.size() != 0)
                {
                    Packet *transpkt;
                    transpkt = op->op_buffer.front();
                    op->op_buffer.pop();
                    transpkt->time_transmitted = t;
                    delay += (1 + transpkt->time_transmitted - transpkt->time_created);
                    delays.push_back(1 + transpkt->time_transmitted - transpkt->time_created);
                    transcnt++;
                    linkutil[transpkt->ip_num][transpkt->op_num]++;
                }
            }
            break;
        case KOUQ:
            // packetScheduling
            for (auto i : ipports)
            {
                if (i->ip_buffer.size() > 0)
                {
                    Packet *headpkt = i->ip_buffer.front();
                    contention[headpkt->op_num].insert(headpkt->ip_num);
                }
            }
            for (auto m : contention)
            {
                int k = min(K, B);
                int sz = opports[m.first]->op_buffer.size();
                if (m.second.size() <= (k - sz))
                {
                    for (auto it : m.second)
                    {
                        int ipnum = it;
                        Packet *sentpkt = ipports[ipnum]->ip_buffer.front();
                        ipports[ipnum]->ip_buffer.pop();
                        opports[m.first]->op_buffer.push(sentpkt);
                        linkutil[sentpkt->ip_num][sentpkt->op_num]++;
                    }
                }
                else if (m.second.size() > (k - sz))
                {
                    dropcnt++;
                    set<int> s;
                    while (s.size() < (k - sz))
                    {
                        int ind = rand() % N;
                        auto it = m.second.begin();
                        advance(it, ind);
                        s.insert(*it);
                    }
                    // s contains random k chosen ip ports
                    for (auto it : m.second)
                    {
                        int ipnum = it;
                        Packet *sentpkt = ipports[ipnum]->ip_buffer.front();
                        ipports[ipnum]->ip_buffer.pop();
                        if (s.find(ipnum) != s.end())
                        {
                            opports[m.first]->op_buffer.push(sentpkt);
                            linkutil[sentpkt->ip_num][sentpkt->op_num]++;
                        }
                        
                    }
                }
            }
            // Transmission
            for (auto op : opports)
            {
                if (op->op_buffer.size() != 0)
                {
                    sort_queue(op->op_buffer);
                    Packet *transpkt;
                    transpkt = op->op_buffer.front();
                    op->op_buffer.pop();
                    transpkt->time_transmitted = t;
                    delay += (1 + transpkt->time_transmitted - transpkt->time_created);
                    delays.push_back(1 + transpkt->time_transmitted - transpkt->time_created);
                    transcnt++;
                }
            }
            break;
        case ISLIP:
            // Packet Scheduling
            vector<vector<int>> accept(N);     // accept array
            set<int> overipports, overopports; // contains set of over ip & op ports
            int cnt = 1;
            while (cnt)
            {
                cnt = 0;
                vector<vector<int>> request(N); // requests array
                for (int i = 0; i < N; ++i)
                {
                    request[i].clear();
                }

                // Request Phase
                for (auto ip : ipports)
                {
                    vector<bool> voq(N, 0);
                    queue<Packet *> temp;
                    int kaki = ip->ip_buffer.size();
                    if (kaki != 0)
                    {
                        while (ip->ip_buffer.size())
                        {
                            Packet *reqpkt = ip->ip_buffer.front();
                            ip->ip_buffer.pop();
                            if (voq[reqpkt->op_num] == 0 && !(overipports.find(reqpkt->ip_num) != overipports.end() || overopports.find(reqpkt->op_num) != overopports.end()))
                            {
                                request[reqpkt->op_num].push_back(reqpkt->ip_num);
                                cnt = 1;
                                voq[reqpkt->op_num] = 1;
                            }
                            temp.push(reqpkt);
                        }
                        ip->ip_buffer = temp;
                    }
                }
                // Grant Phase
                for (int i = 0; i < N; i++)
                {
                    if (overopports.count(i))
                    {
                        continue;
                    }
                    output_Port *op = opports[i];
                    vector<int> curopreq = request[i];
                    int curpointer = grantpointer[i];
                    for (int j = 0; j < N; j++)
                    {
                        // FInd curpointer is present or not in curopreq if found stop and accept this request else continue vector lo ela ethukutham ra?? for loop aa
                        bool ok = 0;
                        for (auto ipno : curopreq)
                        {
                            if (ipno == curpointer)
                            {
                                // granting the request to ip port ipno
                                accept[ipno].push_back(i);
                                ok = 1;
                                break;
                            }
                        }
                        if (ok)
                        {
                            break;
                        }
                        curpointer = (curpointer + 1) % N;
                    }
                }

                // Accept phase
                for (int i = 0; i < N; i++)
                {
                    if (overipports.count(i))
                    {
                        continue;
                    }
                    input_Port *ip = ipports[i];
                    vector<int> curipacc = accept[i];
                    int curpointer = acceptpointer[i];
                    for (int j = 0; j < N; j++)
                    {
                        bool ok = 0;
                        for (auto opno : curipacc)
                        {
                            if (opno == curpointer)
                            {
                                ok = 1;
                                break;
                            }
                        }

                        if (ok)
                        {
                            // I/P port i to O/P port curpointer link has been established in maximal matching
                            grantpointer[curpointer] = (i + 1) % N;
                            acceptpointer[i] = (curpointer + 1) % N;
                            /* inserting i in over ip ports & curpointer in overopports as
                            further iterations we shouldnt consider these ports fopr matching*/
                            overipports.insert(i);
                            overopports.insert(curpointer);
                            // remove that pkt from ip port and place in op port
                            queue<Packet *> temp;

                            while (ip->ip_buffer.size())
                            {
                                Packet *reqpkt = ip->ip_buffer.front();
                                ip->ip_buffer.pop();
                                if (reqpkt->op_num == curpointer)
                                {
                                    opports[reqpkt->op_num]->op_buffer.push(reqpkt);
                                    break;
                                }
                                else
                                {
                                    temp.push(reqpkt);
                                }
                            }
                            while (ip->ip_buffer.size())
                            {
                                Packet *reqpkt = ip->ip_buffer.front();
                                ip->ip_buffer.pop();
                                temp.push(reqpkt);
                            }

                            ip->ip_buffer = temp;
                            break;
                        }
                        curpointer = (curpointer + 1) % N;
                    }
                }
                // transmission;
                for (auto op : opports)
                {
                    if (op->op_buffer.size() != 0)
                    {
                        Packet *transpkt;
                        transpkt = op->op_buffer.front();
                        op->op_buffer.pop();
                        transpkt->time_transmitted = t;
                        delay += (1 + transpkt->time_transmitted - transpkt->time_created);
                        delays.push_back(1 + transpkt->time_transmitted - transpkt->time_created);
                        transcnt++;
                        linkutil[transpkt->ip_num][transpkt->op_num]++;
                    }
                }
            }
            break;
        }
        t++;
    }

    // statistics
    double mean = (double)delay / transcnt;
    double var = 0;
    for (auto i : delays)
    {
        double temp = double(i) - mean;
        var += temp * temp;
    }
    var /= (delays.size());
    cout << "Total Delay is : " << delay << endl;
    cout << "Total Packets Transmitted : " << transcnt << endl;
    cout << fixed <<setprecision(2)<< (double)delay / transcnt << endl;
    cout <<fixed<<setprecision(2)<< (double)delay / transcnt << endl;
    cout << "Standard Deviation of Delay is : " << double(sqrt(var)) << endl;
    if (sched == KOUQ)
        cout << "Packet Drop Prob is : " << (double)dropcnt / (T * N) << endl;
    double sum = 0;
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            sum += (double)linkutil[i][j];
        }
    }
    cout << "AVG utitilisation:" << 100 * (double)sum / (N * T) << "%" << endl;
    
    if (sched == KOUQ)
    {
        // NpQueue typeAvg PDStd Dev of PDAvg link utilizationTable
        outfile << "N"
                << "\t"
                << "K\t"
                << "p\t\t"
                << "queue\t"
                << "Pkt Delay\t"
                << "Stand Dev\t"
                << "Link Utilisation\t"
                << "Pkt drop Prob" << endl;
        outfile << "-------------------------------------------------------------------------------" << endl;
        outfile << N << "\t" << K << "\t" << p << "\t\t" << schess << "\t" << (double)delay / transcnt << "\t\t" << double(sqrt(var)) << "\t\t" << 100 * (double)sum/ (N * T) << "%"
                << "\t\t\t" << (double)dropcnt / (T * N) << endl;
    }
    else
    {
        outfile << "N"
                << "\t"
                << "P\t\t"
                << "queue\t"
                << "Pkt Delay\t"
                << "Stand Dev\t"
                << "Link Utilisation" << endl;
        outfile << "----------------------------------------------------------------" << endl;
        outfile << N << "\t" << p << "\t\t" << schess << "\t\t" << (double)delay / transcnt << "\t\t" << double(sqrt(var)) << "\t\t" << 100 * (double)sum/ (N * T) << "%" << endl;
    }
}