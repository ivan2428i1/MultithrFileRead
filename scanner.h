#pragma once
#include <regex>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

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
        std::cout << "\n===== NEW THREAD +++++ \n\n" << std::endl;
        file.seekg(start, std::ifstream::beg);
        std::string line;
        int line_num {0};
        if (start != 0)
            std::getline(file, line); // we've already scanned this line before
        while (std::getline(file, line))
        {
            std::cout << "line: " << line << std::endl;
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
