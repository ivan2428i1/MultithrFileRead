#include <thread>
#include <algorithm>
#include "scanner.h"

// This method contains logic to decide how many threads we'are gonna be using depending on file size.
// More complex logic may be introduced here when this program is improved
int GetThreadsCount(const int filesize)
{
    if (filesize < 1)
    {
        std::cerr << "Invalid file size\n";
        return 0;
    }
    if (filesize < 1'048'576) // small file
        return 2;

    if (filesize < 536'870'912) // large file
        return 3;

    return 4; // huge file
}

// file scanner with limits to scan
struct ScannerWithBounds
{
    Scanner sc;
    int start_byte; // where to scan from
    int end_byte; // where to scan to
};

// Calculating byte boundaries for every thread:
void CalculateThreadsBoundaries(std::ifstream& file, const std::string& filename,
                                const int filesize, std::vector<ScannerWithBounds>* pV)
{
    auto& sc_v = *pV; // alias
    for (size_t i{0}; i < sc_v.size(); i++)
    {
        file.open(filename);
        sc_v[i].start_byte = (i == 0) ? 0: sc_v[i-1].end_byte - 1;
        if (i == (sc_v.size() - 1))
            file.seekg(0, std::ios_base::end); // last thread always scans to the end of file
        else
        {
            file.seekg(sc_v[i].start_byte + filesize / sc_v.size());
        }
        sc_v[i].end_byte = file.eof() ? (filesize - 1): (int)file.tellg();
        file.close();
    }
}
int main(int argc, char *input[])
{
    if (argc != 3)
    {
        std::cerr << "Two arguments must be provided" << std::endl;
        return 1;
    }
    // Measure filesize in bytes:
    std::ifstream file(input[1]); // pass filename
    if (!file.is_open())
    {
        std::cerr << "Invalid file name\n";
        return 2;
    }
    file.seekg(0, std::ios_base::end);
    int filesize = file.tellg();
    file.close();
    if (filesize == 0)
    {
        std::cout << "File is empty" << std::endl;
        return 0;
    }
    // auto start = std::chrono::high_resolution_clock::now(); // for debug purposes

    // Now we can create several std::threads to read file simultaneously (no need for synchronizing if there's no writing):
    const int num_thr = GetThreadsCount(filesize);
    std::string filename(input[1]);
    std::string rgx(input[2]);
    std::replace(rgx.begin(), rgx.end(), '?', '.'); // convert to regular expressions' syntax

    std::vector<ScannerWithBounds> sc_v(num_thr, {Scanner(filename, rgx), 0, 0}); // scanner object + file-read boundaries

    CalculateThreadsBoundaries(file, filename, filesize, &sc_v);

    std::vector<std::future<std::vector<EntryWithLine>>> futures;

    for (size_t i{0}; i < sc_v.size(); i++)
    {
        futures.emplace_back(sc_v[i].sc.Run(sc_v[i].start_byte, sc_v[i].end_byte)); // launch scanning
    }
    for (const auto& f: futures)
    { // wait for all futures to be ready
        while (f.wait_for(1ms) == std::future_status::timeout)
        {
            std::this_thread::sleep_for(10ms);
        }
    }
    std::vector<std::vector<EntryWithLine>> results_v(futures.size());
    for (size_t i{0}; i < results_v.size(); i++)
        if (futures[i].valid())
            results_v[i] = futures[i].get();

    // Final output:
    { // Total matches:
        int matches {0};
        for (size_t i{0}; i < futures.size(); i++)
            matches += results_v[i].size();
        std::cout << matches << '\n';
    }
    int absolute_line_num {0};
    for (size_t i{0}; i < results_v.size(); i++)
    {
        const auto& v = results_v[i];
        for (const auto& ewline: v)
        {
            std::cout << (absolute_line_num + ewline.relative_l_num) << ' ' << ewline.entry << std::endl;
        }
        absolute_line_num += sc_v[i].sc.TotalLinesScanned();
    }
    //-- debug output ---
    /*auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = (end-start); // debug output
    std::cout << "threads: " << num_thr <<", time passed: " << elapsed.count() * 1000 << " msec" << std::endl;*/

    return 0;
}
