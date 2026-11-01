// Compiler: MSVC
// Author: Safianova Mariia, K25
// Variant: 3

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <random>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

#include "fields.h"

using namespace std;

const int TOTAL_OPS = 600000;
const int RUNS = 5; //result is the average of runs measurements
const string TESTS_DIR = "tests";

//Operation kinds:
//0 - read 0, 1 - write 0, 2 - read 1, 3 - write 1, 4 - read 2, 5 - write 2, 6 - string
const int OPS_COUNT = 7;

//Frequencies in percent a - from my variant, b - all equal, c - chosen by hand
const int FREQ_A[OPS_COUNT] = {10, 10, 50, 10, 5, 5, 10};
const int FREQ_B[OPS_COUNT] = {1, 1, 1, 1, 1, 1, 1};
const int FREQ_C[OPS_COUNT] = {2, 20, 5, 40, 1, 4, 28};

struct Op
{
    int kind;
    int value;
};

string file_name(const string &set, int threads, int part)
{
    return TESTS_DIR + "/" + set + "_" + to_string(threads) + "threads_" + to_string(part) + ".txt";
}

void generate_file(const string &name, const int freq[], int count, mt19937 &gen)
{
    discrete_distribution<int> pick_op(freq, freq + OPS_COUNT);

    ofstream out(name);
    for (int i = 0; i < count; i++)
    {
        int kind = pick_op(gen);
        if (kind == 6)
            out << "string\n";
        else if (kind % 2 == 0)
            out << "read " << kind / 2 << "\n";
        else
            out << "write " << kind / 2 << " 1\n";
    }
}

//3 sets (a, b, c) * (1 + 2 + 3) = 18 files
void generate_all_files(const string sets[], const int *freqs[])
{
    filesystem::create_directory(TESTS_DIR);
    mt19937 gen{random_device()()};
    int files = 0;
    for (int s = 0; s < 3; s++)
    {
        for (int threads = 1; threads <= 3; threads++)
        {
            for (int part = 0; part < threads; part++)
            {
                generate_file(file_name(sets[s], threads, part), freqs[s], TOTAL_OPS / threads, gen);
                files++;
            }
        }
    }
    cout << "Generated " << files << " test files in " << filesystem::absolute(TESTS_DIR).string() << "\n";
}

vector<Op> read_file(const string &name)
{
    vector<Op> ops;
    ifstream in(name);
    string word;
    while (in >> word)
    {
        Op op{0, 0};
        int field;
        if (word == "read")
        {
            in >> field;
            op.kind = 2 * field;
        }
        else if (word == "write")
        {
            in >> field >> op.value;
            op.kind = 2 * field + 1;
        }
        else // string
        {
            op.kind = 6;
        }
        ops.push_back(op);
    }
    return ops;
}

void run_ops(ThreeFields &data, const vector<Op> &ops)
{
    for (const Op &op : ops)
    {
        switch (op.kind)
        {
        case 0: data.get0(); break;
        case 1: data.set0(op.value); break;
        case 2: data.get1(); break;
        case 3: data.set1(op.value); break;
        case 4: data.get2(); break;
        case 5: data.set2(op.value); break;
        case 6: static_cast<string>(data); break;
        }
    }
}

//Each thread runs its own sequence. Returns time in ms
double measure(const vector<vector<Op>> &ops_per_thread)
{
    ThreeFields data;

    auto start = chrono::steady_clock::now();
    {
        vector<jthread> threads;
        for (const vector<Op> &ops : ops_per_thread)
            threads.emplace_back(run_ops, ref(data), cref(ops));
    }
    auto end = chrono::steady_clock::now();

    return chrono::duration<double, milli>(end - start).count();
}

int main()
{
    const string sets[3] = {"a", "b", "c"};
    const int *freqs[3] = {FREQ_A, FREQ_B, FREQ_C};

    generate_all_files(sets, freqs);

    cout << "Total operations in each test: " << TOTAL_OPS << ", runs: " << RUNS << "\n";
    cout << "a - frequencies from the task, b - equal frequencies, c - very different frequencies\n\n";
    cout << fixed << setprecision(2);

    for (int s = 0; s < 3; s++)
    {
        for (int threads = 1; threads <= 3; threads++)
        {
            vector<vector<Op>> ops_per_thread;
            for (int part = 0; part < threads; part++)
                ops_per_thread.push_back(read_file(file_name(sets[s], threads, part)));

            cout << sets[s] << ", " << threads << " thread(s): ";
            double total = 0;
            for (int r = 0; r < RUNS; r++)
            {
                double ms = measure(ops_per_thread);
                total += ms;
                cout << setw(8) << ms << " ";
            }
            cout << ", avg = " << total / RUNS << " ms\n";
        }
    }

    return 0;
}
