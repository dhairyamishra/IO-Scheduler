#include <bits/stdc++.h>
#include <unistd.h>     // getopt
using namespace std;

struct Request {
    int id;
    int arr;            // arrival time
    int track;          // requested track
    int start = -1;     // when head starts seeking
    int end   = -1;     // when completed
};

// ---------------- scheduler enum ----------------
enum Sched { FIFO_N, SSTF_S, LOOK_L, CLOOK_C, FLOOK_F };

// ------------ minimal IOSched (FIFO only) ---------
class IOSched {
  public:
    IOSched(Sched s) : algo(s) {}

    void add(Request* r) { q.push_back(r); }
    bool empty() const   { return q.empty(); }

    // get next request according to selected algo 
    Request* next() {
        // FIFO for now
        Request* r = q.front();
        q.pop_front();
        return r;
    }
  private:
    Sched algo;                  // selected algorithm
    std::deque<Request*> q;      // pending IOs
};

/* ---------- main simulation loop (implements FIFO now) ------------- */
int main(int argc, char* argv[]) {
    // ---------------- flags ----------------
    bool v_flag = false;   // -v : verbose trace
    bool q_flag = false;   // -q : queue trace
    bool f_flag = false;   // -f : flook trace

    Sched algo = FIFO_N;   // default scheduler

    // ---------------- getopt ----------------
    int opt;
    while ((opt = getopt(argc, argv, "s:vqf")) != -1) {
        switch (opt) {
            case 's':
                switch (optarg[0]) {
                    case 'N': algo = FIFO_N;  break;
                    case 'S': algo = SSTF_S;  break;
                    case 'L': algo = LOOK_L;  break;
                    case 'C': algo = CLOOK_C; break;
                    case 'F': algo = FLOOK_F; break;
                    default:
                        cerr << "Invalid scheduler code\n";
                        return 1;
                }
                break;
            case 'v': v_flag = true; break;
            case 'q': q_flag = true; break;
            case 'f': f_flag = true; break;
            default:
                cerr << "Usage: ./iosched [-sX] [-v] [-q] [-f] <inputfile>\n";
                return 1;
        }
    }
    // silence unused warnings
    (void)v_flag; (void)q_flag; (void)f_flag;
    // remaining arg must be input file
    if (optind >= argc) {
        cerr << "No input file provided\n";
        return 1;
    }
    string infilename = argv[optind];

    /* ---------------- read all requests ---------------- */
    ifstream in(infilename);
    vector<Request> reqs;
    string line; int t, tr;
    while (getline(in,line)) {
        if (line.empty() || line[0]=='#') continue;
        stringstream ss(line);
        ss>>t>>tr;
        reqs.push_back(Request{(int)reqs.size(),t,tr});
    }
    size_t next_idx = 0;
    size_t total_ios = reqs.size();

    /* ---------------- statistics containers ------------ */
    int tot_movement = 0;
    int io_busy = 0;
    int cur_track = 0;
    IOSched scheduler(algo);

    /* ---------------- simulation loop ------------------ */
    int time = 0;
    Request* active = nullptr;
    while (true) {
        /* A. enqueue arrivals */
        if (next_idx < total_ios && reqs[next_idx].arr == time) {
            scheduler.add(&reqs[next_idx]);
            ++next_idx;                        // one arrival per tick
        }

        /* B. finish active */
        if (active && active->end == time) {
            active = nullptr;
        }

        /* C. if no active, fetch next */
        if (!active && !scheduler.empty()) {
            active = scheduler.next();
            active->start = time;
            int dist = abs(active->track - cur_track);
            active->end  = time + dist;        // finish after exact moves
            io_busy  += dist;                  // busy exactly dist units
            tot_movement += dist;              // total head movement
        }

        /* D. advance head if active */
        if (active) {
            if (active->track > cur_track)      
                {cur_track += 1;}
            else if (active->track < cur_track) 
                {cur_track -= 1;}
        }

        /* E. exit? */
        if (!active && scheduler.empty() && next_idx == total_ios) {break;}
        ++time;
    }

    /* ---------------- output per-request lines ---------- */
    int turnaround_sum = 0, wait_sum = 0, max_wait = 0;
    for (auto& r: reqs) {
        int wait = r.start - r.arr;
        turnaround_sum += r.end - r.arr;
        wait_sum       += wait;
        max_wait = max<int>(max_wait, wait);
        printf("%5d: %5d %5d %5d\n", r.id, r.arr, r.start, r.end);
    }

    /* ---------------- output SUM line ------------------ */
    double avg_turn = (double)turnaround_sum / total_ios;
    double avg_wait = (double)wait_sum       / total_ios;
    double util     = (double)io_busy / time;

    printf("SUM: %d %d %.4lf %.2lf %.2lf %d\n",
           time, tot_movement, util, avg_turn, avg_wait, max_wait);
    return 0;
}