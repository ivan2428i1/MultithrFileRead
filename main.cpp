#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <cassert>
#include <future>
#include <thread>
#include <fstream>
#include <algorithm>

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
struct EntryWithLine
{
    EntryWithLine(const std::string& e, const int num)
    {
        this->entry = e;
        this->relative_l_num = num;
    }
    std::string entry; // entry
    int relative_l_num; // line number for corresponding thread (relative, not absolute)
};

class Scanner
{
    std::vector<EntryWithLine> _v;
    const std::string _filename;
    const std::regex _mask;
    int _total_lines_scanned; // how many lines have this scanned scanned totally

    void ScanOneLine(const int n, const std::string& line)
    {
        std::smatch m;
        int ind_in_line {1}; // we need to output match's index from the start of the line, even if there are several matches
        auto searchStart( line.cbegin() );
        while (std::regex_search(searchStart, line.cend(), m, _mask))
        { // Need to create a descriptive string:
            ind_in_line += m.position(0);
            _v.emplace_back(std::to_string(ind_in_line) + " " + // position
                            m[0].str(), //  // match case
                            n /* relative line number*/);
            searchStart = m.suffix().first;
            ind_in_line += m[0].str().size();
        }
    }
    void MultiLineScan(const std::string& filename, int start, int stop)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Unable to open file " << filename << std::endl;
            return;
        }
        file.seekg(start, std::ifstream::beg);
        std::string line;
        int line_num {0};
        while (std::getline(file, line))
        {
            _total_lines_scanned++;
            ScanOneLine(++line_num, line);
            if (file.tellg() >= stop)
                break;
        }
        file.close();
    }
public:
    Scanner(const std::string& fname, const std::string& mask):
        _filename(fname),
        _mask(mask),
        _total_lines_scanned(0)
    {
    }
    void Run(int start_byte, int end_byte)
    {
        //auto fut = std::async(std::launch::async, MultiLineScan, filename, start, stop);
        MultiLineScan(_filename, start_byte, end_byte);
    }
    const std::vector<EntryWithLine>& GetResult()
    {
        return _v;
    }
    int TotalLinesScanned() const { return _total_lines_scanned; }
};
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
        sc_v[i].start_byte = (i == 0) ? 0: sc_v[i-1].end_byte;
        if (i == (num_thr - 1))
            file.seekg(0, std::ios_base::end);
        else
        {
            file.seekg(sc_v[i].start_byte + filesize / num_thr);
            std::string oneline;
            if (!file.eof())
                std::getline(file, oneline); // we can't fraction a line, boundaries must be between lines
        }
        sc_v[i].end_byte = file.eof() ? (filesize - 1): (int)file.tellg();
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
