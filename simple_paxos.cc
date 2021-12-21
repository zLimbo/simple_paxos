#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;
using namespace std::chrono;

template <typename T>
static void shuffle(deque<T> &dq) {
    static random_device rd;
    static mt19937 g(rd());
    shuffle(dq.begin(), dq.end(), g);
    // random_shuffle(dq.begin(), dq.end());
}

void net_trans() {
    default_random_engine e;
    e.seed(time(nullptr));
    uniform_int_distribution<int> d(20, 50);
    this_thread::sleep_for(chrono::milliseconds(d(e)));
}

class Acceptor {
public:
    Acceptor(int id) : id_(id) {}

    pair<int, int> prepare(int n) {
        net_trans();
        unique_lock<timed_mutex> locker(mtx_, chrono::milliseconds(20));
        if (!locker.owns_lock()) return {-2, -2};
        if (n > min_proposal_) min_proposal_ = n;
        return {accepted_proposal_, accepted_value_};
    }

    int accept(int n, int value) {
        net_trans();
        unique_lock<timed_mutex> locker(mtx_, chrono::milliseconds(20));
        if (!locker.owns_lock()) return -2;
        if (n >= min_proposal_) {
            accepted_proposal_ = min_proposal_ = n;
            accepted_value_ = value;
        }
        return min_proposal_;
    }

    int get_id() const { return id_; }
    int get_n() const { return accepted_proposal_; }
    int get_v() const { return accepted_value_; }

private:
    int min_proposal_ = -1;
    int accepted_proposal_ = -1;
    int accepted_value_ = -1;
    int id_;

    timed_mutex mtx_;
};

class Proposer {
public:
    Proposer(int id) : id_(id) {}

    void run(vector<shared_ptr<Acceptor>> &acceptors) {
        int f = acceptors.size() / 2 + 1;
        while (true) {
            int n = get_assign_n();
            int v = n;
            printf("== proposal %d prepare [n=%d]\n", id_, n);
            deque<shared_ptr<Acceptor>> prepare_dq(acceptors.begin(),
                                                   acceptors.end());
            shuffle(prepare_dq);
            int max_accept_proposal = 0;
            int promise_num = 0;
            while (!prepare_dq.empty()) {
                auto acceptor = prepare_dq.front();
                prepare_dq.pop_front();
                auto [accepted_proposal, accepted_value] = acceptor->prepare(n);
                if (accepted_proposal == -2) {
                    prepare_dq.push_back(acceptor);
                    continue;
                }
                if (accepted_proposal > n) continue;
                if (accepted_proposal > max_accept_proposal) v = accepted_value;
                if (++promise_num >= f) break;
            }
            if (promise_num < f) continue;

            printf("== proposal %d accept [n=%d, v=%d]\n", id_, n, v);
            deque<shared_ptr<Acceptor>> accept_dq(acceptors.begin(),
                                                  acceptors.end());
            shuffle(accept_dq);
            int accept_num = 0;
            while (!accept_dq.empty()) {
                auto acceptor = accept_dq.front();
                accept_dq.pop_front();
                int min_proposal = acceptor->accept(n, v);
                if (min_proposal == -2) {
                    accept_dq.push_back(acceptor);
                    continue;
                }
                if (min_proposal > n) break;
                if (++accept_num >= f) break;
            }

            if (accept_num < f) continue;

            printf("== proposal %d chosen [n=%d, v=%d]\n", id_, n, v);
            break;
        }
    }

private:
    int id_;

    inline static int assign_no = 1;
    inline static mutex mtx;

    static int get_assign_n() {
        lock_guard<mutex> locker(mtx);
        return assign_no++;
    }
};

int main() {
    int acceptor_num = 5;
    int proposer_num = 2;
    vector<shared_ptr<Acceptor>> acceptors;
    for (int i = 0; i < acceptor_num; ++i) {
        acceptors.emplace_back(make_shared<Acceptor>(i));
    }

    vector<thread> ths;
    for (int i = 0; i < proposer_num; ++i) {
        ths.emplace_back([i, &acceptors] { Proposer(i).run(acceptors); });
    }

    for (auto &th : ths) {
        th.join();
    }

    unordered_map<int, int> ump;
    for (auto acceptor : acceptors) {
        printf("accpetor %d chosen [n=%d, v=%d]\n", acceptor->get_id(),
               acceptor->get_n(), acceptor->get_v());
        ++ump[acceptor->get_v()];
    }

    for (auto [v, count] : ump) {
        printf("v=%d, count=%d\n", v, count);
    }

    return 0;
}