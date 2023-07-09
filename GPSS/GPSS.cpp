#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <random>
#define RND 34
#define CHANNELS 2

using namespace std;

float frand(int lborder, int rborder)
{
    if (lborder > rborder || lborder < 0)
    {
        cout << "incorrect borders in frand. Programm stopped\n";
        exit(-2);
    }
    int change = rborder - lborder + 1;
    int intpart = lborder + rand() % change;
    float floatpart = ((rand() % 1000) / 1000.0);
    return ((float)intpart + floatpart);
}

class Transact_generation
{
private:
    float next_time_generate;
    int l_border;
    int r_border;
public:

    Transact_generation(int _l_border = 0, int _r_border = 100) : l_border(_l_border), r_border(_r_border) { next_time_generate = frand(l_border, r_border); }
    void set_next_generate() { next_time_generate = frand(l_border, r_border); }
    void set_next_generate(float offset) { next_time_generate -= offset; }
    float get_next_generate() { return next_time_generate; }
};

struct communication
{
    float current_time;
    float finish_time;
    int count_served;
    int count_loaded;
    int count_total;
    ofstream file;
};

char* statuses[] = { (char*)"generated", (char*)"in queue", (char*)"request is serving", (char*)"serving ended" }; // 0 1 2 3

class Transact
{
private:
    int id;
    int status;
    float time_before_event;
public:
    //static float set_next_generate(float _time) {static float next_generate_time; if (_time >= 0) next_generate_time = _time; return next_generate_time;};
    int get_id() { return id; };
    int get_status() { return status; };
    float get_time_before_event() { return time_before_event; };
    friend void time_correction(vector<Transact>&, float);
    friend void time_correction(Transact&, float);
    Transact();
    friend ostream& operator<<(ostream& out, Transact a) { out << "transact id: " << a.id << " time before event " << a.time_before_event << " status: " << statuses[a.status] << endl; return out; };
    void serve(int, communication&);
    void wait(float);
};

Transact::Transact()
{
    static int _id = 0;
    id = _id++;
    status = 0;
    time_before_event = 0;
}

void Transact::serve(int _channel, communication& info)
{
    float serve_time;
    if (!_channel)
        serve_time = frand(13, 33);
    else
        serve_time = frand(16, 30);
    status = 2;
    info.count_loaded++;
    info.file << info.current_time << "s: transact " << this->get_id() << " loaded channel " << _channel + 1 << endl;
    time_before_event = serve_time;
}

void Transact::wait(float channels_delay)
{
    status = 1;
    time_before_event = channels_delay;
}

class Service
{
private:
    int channel[CHANNELS];
    list<int> office[CHANNELS];
public:
    Service();
    Transact* get_in_line(Transact*, int, float, communication&);
    int choose_queue();
    Transact* serve_transact(Transact*, communication&);
    int slide_queue(Transact&, communication&);
};

Service::Service()
{
    for (int i = 0; i < CHANNELS; i++)
    {
        channel[i] = 0;
    }
}

int Service::choose_queue()
{
    int q_number = 0;
    long long unsigned int min_size = office[0].size();
    for (int i = 1; i < CHANNELS; i++) {
        if (office[i].size() < min_size) {
            min_size = office[i].size();
            q_number = i;
        }
    }
    return q_number;
}

Transact* Service::get_in_line(Transact* a, int _channel, float channels_delay, communication& info)
{
    int place_in_queue = office[_channel].size() + 1;
    if (place_in_queue != 1)
        channels_delay += place_in_queue * 30;
    a->wait(channels_delay);
    info.file << info.current_time << "s: transact " << a->get_id() << " got in queue " << _channel + 1 << endl;
    office[_channel].push_back(a->get_id());
    return a;
}
Transact* Service::serve_transact(Transact* a, communication& info)
{
    for (int i = 0; i < CHANNELS; i++)
    {
        if (channel[i] == 0)
        {
            a->serve(i, info);
            channel[i] = a->get_id();
            return a;
        }
    }
    double channels_delay;
    int desicion = choose_queue();
    channels_delay = 30;
    a = get_in_line(a, desicion, channels_delay, info);
    return a;
}

int Service::slide_queue(Transact& done_tr, communication& info)
{
    int tr_channel = -1, first_in_queue;
    for (int i = 0; i < CHANNELS; i++)
    {
        if (done_tr.get_id() == channel[i])
        {
            tr_channel = i;
            break;
        }
    }
    info.count_served++;
    info.file << info.current_time << "s: transact " << done_tr.get_id() << " freed channel " << tr_channel + 1 << endl;
    channel[tr_channel] = 0;
    if (office[tr_channel].size()) {
        first_in_queue = office[tr_channel].front();
        office[tr_channel].erase(office[tr_channel].begin());
        return first_in_queue;
    }
    return 0;
}

Transact* generate_ta(communication& info, Transact_generation& device)
{
    info.count_total++;
    float _time = device.get_next_generate();
    device.set_next_generate();
    Transact* a = new Transact;
    info.file << info.current_time << "s: transact " << a->get_id() << " entered the system\n";
    return a;
}

void time_correction(vector<Transact>& chain, float offset)
{
    vector<Transact>::iterator it1 = chain.begin();
    for (; it1 != chain.end(); it1++)
    {
        it1->time_before_event -= offset;
    }
}

void time_correction(Transact& chain, float offset)
{
    chain.time_before_event -= offset;
}

void fix_FEC(vector<Transact>& FEC, Transact& CEC)
{
    int id_list;
    vector<Transact>::iterator it1;
    id_list = CEC.get_id();
    for (it1 = FEC.begin(); it1 != FEC.end(); it1++)
    {
        if (id_list == it1->get_id())
        {
            FEC.erase(it1);
            break;
        }
    }
}

void timer_correction_phase(vector<Transact>& Fchain, Transact& Cchain, Service& some_service, communication& info, Transact_generation& device)
{
    //&Cchain = NULL;
    vector<Transact>::iterator it1;
    float next_gener_time = device.get_next_generate();
    float nearest_event = next_gener_time + 1;
    for (it1 = Fchain.begin(); it1 != Fchain.end(); it1++)
    {
        if (Fchain.size() == 0) break;
        if (it1->get_status() != 2) continue;
        float current_event = it1->get_time_before_event();
        if (current_event < nearest_event)
        {
            nearest_event = current_event;
            Cchain = *it1;
        }
    }
    if ((next_gener_time <= nearest_event) || (Fchain.size() == 0))
    {
        if (Fchain.size() != 0)
            time_correction(Fchain, next_gener_time);
        nearest_event -= next_gener_time;
        info.current_time += next_gener_time;
        if (info.current_time < info.finish_time)
        {
            Transact* tmp = generate_ta(info, device);
            tmp = some_service.serve_transact(tmp, info);
            Fchain.push_back(*tmp);
            delete tmp;
            timer_correction_phase(Fchain, Cchain, some_service, info, device);
        }
    }
    else
    {
        info.current_time += nearest_event;
        if (info.current_time < info.finish_time)
        {
            time_correction(Fchain, nearest_event);
            time_correction(Cchain, nearest_event);
            device.set_next_generate(nearest_event);
        }
    }
}

void viewing_phase(vector<Transact>& Fchain, Transact& Cchain, Service& some_service, communication& info)
{
    vector<Transact>::iterator it1;
    int tr_id;
    tr_id = some_service.slide_queue(Cchain, info);
    if (tr_id != 0) {
        for (it1 = Fchain.begin(); it1 != Fchain.end(); it1++)
        {
            if (it1->get_id() == tr_id) {
                some_service.serve_transact(&(*it1), info);
                break;
            }
        }
    }
    fix_FEC(Fchain, Cchain);
}

int main()
{
    srand(RND + 3);
    Transact_generation device1(0, 23);
    struct communication info = { 0, 0, 0, 0, 0 };
    info.file.open("logfile.txt", ios_base::out);
    if (!info.file.is_open()) return -1;
    cout << "Enter generation time\n";
    cin >> info.finish_time;
    //info.finish_time = 5000;
    vector<Transact> FEC;
    Transact CEC;
    Transact* tmp;
    info.current_time += device1.get_next_generate();
    tmp = generate_ta(info, device1);
    Service tr_service;
    tmp = tr_service.serve_transact(tmp, info);
    FEC.push_back(*tmp);
    delete tmp;
    vector<Transact>::iterator it1;
    while (info.current_time < info.finish_time)
    {

        timer_correction_phase(FEC, CEC, tr_service, info, device1);
        if (info.current_time > info.finish_time)
            break;
        viewing_phase(FEC, CEC, tr_service, info);
    }
    info.file.close();
    cout << "\nanalis has finished\n\n" << "number of served transacts: " << info.count_served << endl;
    cout << "number of transacts those were loaded on all the channels: " << info.count_loaded << endl;
    cout << "total number of transacts: " << info.count_total << endl;
    return 0;
}