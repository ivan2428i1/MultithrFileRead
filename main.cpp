#include <future>
#include <thread>
#include <algorithm>
#include "scanner.h"

// This method contains logic to decide, how many threads we'are gonna be using depending on file size.
// We can introduce more complex logic in this method when this program is improved
int GetThreadsCount(const int filesize)
{
    return 4;
//    if (filesize < 1)
//    {
//        std::cerr << "Invalud file size\n";
//        return 0;
//    }
//    if (filesize < 536'870'912)
//        return 2;

//    return 4;
}


struct ScannerWithBounds
{
    Scanner sc;
    int start_byte; // where to scan from
    int end_byte; // where to scan through
};

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
    std::cout << input[1] << " file size: " << filesize << " bytes\n";
    file.close();
    if (filesize == 0)
    {
        std::cout << "File is empty" << std::endl;
        return 0;
    }
    // Now we can create several std::threads to read file simultaneously (no need for synchronizing if there's no writing):
    const int num_thr = GetThreadsCount(filesize);
    std::string filename(input[1]);
    std::string rgx(input[2]);
    std::replace(rgx.begin(), rgx.end(), '?', '.'); // convert to regular expressions' syntax
    std::vector<ScannerWithBounds> sc_v(num_thr, {Scanner(filename, rgx), 0, 0}); // scanner object + file-read boundaries
    for (int i{0}; i < num_thr; i++)
    {// Calculating boundaries for every thread:
        file.open(filename);
        sc_v[i].start_byte = (i == 0) ? 0: sc_v[i-1].end_byte - 1;
        if (i == (num_thr - 1))
            file.seekg(0, std::ios_base::end);
        else
        {
            int prel_end = sc_v[i].start_byte + filesize / num_thr;
            std::cout << "prelim end byte: " << prel_end << std::endl; //DEBUG
            file.seekg(prel_end);
        }
        sc_v[i].end_byte = file.eof() ? (filesize - 1): (int)file.tellg();
        std::cout << "Actual end byte: " << sc_v[i].end_byte << std::endl; // DEBUG
        file.close();
    }
    for (auto& [sc, start_b, end_b]: sc_v) // c++17 syntax
    {
        sc.Run(start_b, end_b);
    }
    // Final output:
    { // Total matches:
        int matches {0};
        for (size_t i{0}; i < sc_v.size(); i++)
            matches += sc_v[i].sc.GetResult().size();
        std::cout << matches << '\n';
    }
    int absolute_line_num {0};
    for (auto& structure: sc_v)
    {
        for (auto& ewline: structure.sc.GetResult())
        {
            std::cout << (absolute_line_num + ewline.relative_l_num) << ' ' << ewline.entry << std::endl;
        }
        absolute_line_num += structure.sc.TotalLinesScanned();
    }


    return 0;
}
