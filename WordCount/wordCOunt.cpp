#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>

using namespace std;

mutex mtx;
void mapper(const vector<string>& lines,
            unordered_map<string, int>& local_map)
{
    for (const auto& line : lines) {
        string word;
        stringstream ss(line);
        while (ss >> word) {
            local_map[word]++;
        }
    }
}

void reducer(unordered_map<string, int>& global_map,
             unordered_map<string, int>& local_map)
{
    lock_guard<mutex> lock(mtx);
    for (auto& p : local_map) {
        global_map[p.first] += p.second;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: ./wordcount <input_file>\n";
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Cannot open file\n";
        return 1;
    }

    vector<string> lines;
    string line;

    while (getline(file, line)) {
        lines.push_back(line);
    }

    int num_threads = 4;  
    int chunk_size = lines.size() / num_threads;

    vector<thread> threads;
    vector<unordered_map<string, int>> local_maps(num_threads);
    unordered_map<string, int> global_map;

    for (int i = 0; i < num_threads; i++) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? lines.size() : start + chunk_size;

        vector<string> chunk(lines.begin() + start, lines.begin() + end);

        threads.emplace_back(mapper, chunk, ref(local_maps[i]));
    }

    for (auto& t : threads) t.join();

    threads.clear();

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(reducer, ref(global_map), ref(local_maps[i]));
    }

    for (auto& t : threads) t.join();

    for (auto& p : global_map) {
        cout << p.first << " : " << p.second << endl;
    }

    return 0;
}
