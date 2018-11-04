//
// ReadData.h
// Helper for loading binary data files from disk
//
// Looks for all files in the same folder as the running EXE
//

#include <stdint.h>
#include <exception>
#include <fstream>
#include <vector>

namespace DX
{
    inline std::vector<uint8_t> ReadData(const wchar_t* name)
    {
        wchar_t moduleName[_MAX_PATH];
        (void) GetModuleFileNameW(nullptr, moduleName, _MAX_PATH);

        wchar_t drive[_MAX_DRIVE];
        wchar_t path[_MAX_PATH];

        if (_wsplitpath_s(moduleName, drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0))
            throw std::exception("_wsplitpath_s");

        wchar_t filename[_MAX_PATH];
        if (_wmakepath_s(filename, _MAX_PATH, drive, path, name, nullptr))
            throw std::exception("_wmakepath_s");

        std::ifstream inFile(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!inFile)
            throw std::exception("ReadData");

        std::streampos len = inFile.tellg();
        if (!inFile)
            throw std::exception("ReadData");

        std::vector<uint8_t> blob;
        blob.resize(size_t(len));

        inFile.seekg(0, std::ios::beg);
        if (!inFile)
            throw std::exception("ReadData");

        inFile.read(reinterpret_cast<char*>(&blob.front()), len);
        if (!inFile)
            throw std::exception("ReadData");

        inFile.close();

        return blob;
    }
}