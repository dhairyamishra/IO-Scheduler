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

// Scheduler class
class IOSched {
  public:
    IOSched(Sched s) : algo(s) {}

    // enqueue request
    void add(Request* r) {
        if (algo == FLOOK_F) {
            // During sweep, new arrivals go to new batch.
            if (!in_sweep)
                q.push_back(r);
            else
                add_q.push_back(r);
        } else {
            q.push_back(r);
        }
    }

    // check if scheduler has pending
    bool empty() const {
        if (algo == FLOOK_F)
            return q.empty() && add_q.empty();
        return q.empty();
    }

    // get next request according to algo
    Request* next(int cur_track, int now) {
        // ---------- FIFO ----------
        if (algo == FIFO_N) {
            if (q.front()->arr <= now) {
                Request* r = q.front();
                q.pop_front();
                return r;
            }
            return nullptr; // first request not yet arrived
        }

        // ---------- SSTF ---------- 
        if (algo == SSTF_S) {
            auto best_it = q.begin();
            int best_dist = abs((*best_it)->track - cur_track);// its distance from the head
            // scan pending request
            for (auto it = q.begin(); it != q.end(); ++it) {
                if ((*it)->arr > now) continue; // skip future arrivals
                int d = abs((*it)->track - cur_track); // distance of current request
                // update head if better distance found
                if (d < best_dist) {
                    best_dist = d;
                    best_it = it;
                    if (best_dist == 0) break;   // early exit: head already on track from proff guidance
                }
            }
            // minimum seek distance
            if (best_it != q.end()) {
                Request* r = *best_it;//get pointer
                q.erase(best_it);// remove it from the queue
                return r; //return to simulator
            }
            return nullptr; // none arrived yet
        }

        // ---------- LOOK ----------
        if (algo == LOOK_L) {
            if (q.empty()) return nullptr;

            auto best_it = q.end();
            int min_seek = INT_MAX;

            // pass 1: current direction
            for (auto it = q.begin(); it != q.end(); ++it) {
                if ((*it)->arr > now) continue; // skip future
                int delta = (*it)->track - cur_track;
                if ((direction == 1 && delta >= 0) || (direction == -1 && delta <= 0)) {
                    int seek = abs(delta);
                    if (seek < min_seek) { min_seek = seek; best_it = it; }
                    if (min_seek == 0) break;    // early exit
                }
            }

            // if nothing, reverse and scan again
            if (best_it == q.end()) {
                direction = -direction;
                for (auto it = q.begin(); it != q.end(); ++it) {
                    if ((*it)->arr > now) continue;
                    int delta = (*it)->track - cur_track;
                    if ((direction == 1 && delta >= 0) || (direction == -1 && delta <= 0)) {
                        int seek = abs(delta);
                        if (seek < min_seek) { min_seek = seek; best_it = it; }
                        if (min_seek == 0) break; // early exit
                    }
                }
            }

            // return best
            if (best_it != q.end()) {
                Request* r = *best_it;
                q.erase(best_it);
                return r;
            }
            return nullptr; // none arrived yet in either direction
        }

        // ---------- CLOOK ----------
        if (algo == CLOOK_C) {
            if (q.empty()) return nullptr;
            auto best_it = q.end();
            int best_track = INT_MAX;
            // first pass: tracks >= cur_track (forward sweep)
            for (auto it = q.begin(); it != q.end(); ++it) {
                if ((*it)->arr > now) continue; // not arrived
                if ((*it)->track >= cur_track && (*it)->track < best_track) {
                    best_track = (*it)->track;
                    best_it = it;
                }
            }
            // if none found, wrap: pick smallest track overall
            if (best_it == q.end()) {
                for (auto it = q.begin(); it != q.end(); ++it) {
                    if ((*it)->arr > now) continue;
                    if ((*it)->track < best_track) {
                        best_track = (*it)->track;
                        best_it = it;
                    }
                }
            }
            if (best_it != q.end()) {
                Request* r = *best_it;
                q.erase(best_it);
                return r;
            }
            return nullptr; // none ready yet
        }

        // ---------- FLOOK ----------
        if (algo == FLOOK_F) {
            //next batch if active queue empty
            if (q.empty()) {
                if (add_q.empty()) return nullptr;           // nothing ready yet
                q.swap(add_q);                               // promote waiting batch
                direction = 1;                               // start upward
                in_sweep  = false;                           // new sweep not started
            }

            //pick nearest request in current direction
            auto best_it   = q.end();
            int  best_dist = INT_MAX;

            if (direction == 1) { 
                // scanning upward
                for (auto it = q.begin(); it != q.end(); ++it) {
                    int d = (*it)->track - cur_track;
                    if (d >= 0 && d < best_dist) { best_dist = d; best_it = it; }
                    if (best_dist == 0) break;   // early exit
                }
                if (best_it == q.end()) {         
                    // nothing upward,reverse once
                    direction = -1;
                }
            }

            if (direction == -1) { 
                // scanning downward
                best_dist = INT_MAX;
                for (auto it = q.begin(); it != q.end(); ++it) {
                    int d = cur_track - (*it)->track;
                    if (d >= 0 && d < best_dist) { best_dist = d; best_it = it; }
                    if (best_dist == 0) break;   // early exit
                }
                if (best_it == q.end()) {
                    // waitm, queue not empty but all lower requests still future
                    return nullptr;
                }
            }

            if (best_it != q.end()) {
                Request* r = *best_it;
                q.erase(best_it);
                // inside active sweep now
                in_sweep = true;                   
                return r;
            }
            return nullptr;
        }
        // if none Default
        return nullptr;
    }

    // expose read-only queue for debug printing
    const std::deque<Request*>& queue() const { return q; }

    // update sweep direction
    void set_direction(bool up) { direction = up ? 1 : -1; }

  private:
    Sched algo;                  // selected algorithm
    std::deque<Request*> q;      // active queue
    std::deque<Request*> add_q;  // FLOOK buffer for next batch
    int direction = 1;           // current head direction
    bool in_sweep = false;       // FLOOK true once first request of batch given
};

/* ---------------- simulation loop ------------------ */
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

    // helper 
    auto dump_queue = [&](const deque<Request*>& dq, int dir, int cur_track){
        // internal check for q flag
        if (!q_flag) return;
        cout << "    Q DIR=" << dir << " ";
        for (auto* r : dq) {
            if (dir == 0) {
                // add phase: just id:track
                cout << "[" << r->id << ":" << r->track << "] ";
            } else {
                int dist = r->track - cur_track;
                if (dir == -1) dist = -dist;   // distance negative if opposite dir
                cout << "[" << r->id << ":" << r->track << ":" << dist << "] ";
            }
        }
        cout << "\n";
    };

    /* ---------------- simulation loop ------------------ */
    int time = 0;
    Request* active = nullptr;
    while (true) {
        /* A. enqueue arrivals */
        if (next_idx < total_ios && reqs[next_idx].arr == time) {
            scheduler.add(&reqs[next_idx]);
            if (v_flag)
            {
                cout << time << ":\n" << reqs[next_idx].id << " add " << reqs[next_idx].track << "\n";
            }
            dump_queue(scheduler.queue(), 0, cur_track);
            ++next_idx;                        // one arrival per tick
        }

        /* B. finish active */
        if (active && active->end == time) {
            if (v_flag)
            {
                cout << time << ":\n" << active->id << " finish " << (time - active->arr) << "\n";
            }
            active = nullptr;
        }

        /* C. if no active, fetch next */
        if (!active && !scheduler.empty()) {
            active = scheduler.next(cur_track, time);
            if (!active) {            
                // queue has future arrivals
                ++time;
                continue;
            }

            // keep FLOOK sweep direction in sync
            if (active->track != cur_track)
                scheduler.set_direction(active->track > cur_track);

            if (v_flag) {
                cout << time << ":\n" << active->id << " issue " << active->track << " " << cur_track << "\n";
            }

            // movement direction for dump
            int dir = 0;
            if (active->track > cur_track) {dir = 1;}
            else if (active->track < cur_track) {dir = -1;}
            dump_queue(scheduler.queue(), dir, cur_track);
            active->start = time;
            int dist = abs(active->track - cur_track);
             // finishes after exact moves
            active->end  = time + dist;       

            //proff directions, instantly complete same track requests so multiple can be done without tick time
            if (dist == 0) {
                if (v_flag) {
                    cout << time << ":\n" << active->id << " finish " << (time - active->arr) << "\n";
                }
                // request done, clear and loop again
                active = nullptr;
                continue;
            }
        }

        /* D. advance head if active (tick-by-tick accounting) */
        if (active) {
            if (active->track > cur_track) {
                cur_track += 1;
                ++tot_movement;
                ++io_busy;
            } else if (active->track < cur_track) {
                cur_track -= 1;
                ++tot_movement;
                ++io_busy;
            }
        }

        /* E. exit */
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