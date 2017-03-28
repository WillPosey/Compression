/******************************************************
 *
 *  File:       SIM.cpp
 *  Created By: William Posey
 *  Course:     CDA5636 Embedded Systems
 *  Project:    Project 2, Compression/Decompression
 *
 ******************************************************/
#include <string>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <bitset>

/* File name definitions */
#define COMPRESS_INPUT_FILENAME "original.txt"
#define COMPRESS_OUTPUT_FILENAME "cout.txt"
#define DECOMPRESS_INPUT_FILENAME "compressed.txt"
#define DECOMPRESS_OUTPUT_FILENAME "dout.txt"

using namespace std;

/********************************
 *  DictionaryEntry struct
 ********************************/
struct DictionaryEntry
{
    string binary;
    int frequency;
    int index;

    DictionaryEntry() : binary("00000000000000000000000000000000"),
                        frequency(1),
                        index(-1){};
};

/************************************************
 *  Dictionary Class
 ************************************************/
class Dictionary
{
public:
    /**************************/
    Dictionary()
    {
        entryCount = 0;
    }

    string GetEntry(string entryIndex)
    {
        bitset<4> index(entryIndex);
        return dictionaryContent[index.to_ulong()].binary;
    }

    /**************************/
    void CreateDictionary(string filename)
    {
        string line;
        ifstream file (filename);

        if(filename.compare(COMPRESS_INPUT_FILENAME) == 0)
        {
            while(getline(file, line))
            {
                if(line.length() > 32)
                    line.erase(32,1);
                AddInstance(line);
            }
        }
        else if(filename.compare(DECOMPRESS_INPUT_FILENAME) == 0)
        {
            bool foundDictionary = false;
            int index = 0;

            while(getline(file, line))
            {
                if(foundDictionary)
                {
                    if(line.length() > 32)
                        line.erase(line.length()-1,1);
                    dictionaryContent[index++].binary = line;
                }
                else
                {
                    line.erase(line.length()-1,1);
                    if(line.compare("xxxx") == 0)
                        foundDictionary = true;
                }
            }
        }
        file.close();
    }

    /**************************/
    void AddInstance(string binary)
    {
        DictionaryEntry entry,temp;
        unordered_map<string,DictionaryEntry>::iterator it = entryMap.find(binary);

        if(it == entryMap.end())
        {
            entry.binary = binary;
            entry.frequency = 1;
            if(entryCount < 16)
            {
                entry.index = entryCount;
                dictionaryContent[entryCount] = entry;
                entryCount++;
            }
            else
                entry.index = -1;

            entryMap[binary] = entry;
        }
        else
        {
            entry = entryMap[binary];
            int newFreq = entry.frequency + 1;
            entry.frequency = newFreq;

            int index = entry.index;

            if(index == 0)
            {
                dictionaryContent[0] = entry;
                entryMap[binary] = entry;
            }
            else
            {
                int numPassed = 0, newIndex, startIndex;

                int i = index<0 ? entryCount-1 : index-1;

                while(newFreq >= dictionaryContent[i].frequency && i>=0)
                {
                    numPassed++;
                    i--;
                }

                if(numPassed!=0)
                {
                    if(index<0)
                    {
                        newIndex = entryCount-numPassed;
                        startIndex = entryCount;
                    }
                    else
                    {
                        newIndex = index-numPassed;
                        startIndex = index;
                    }
                }
                else
                {
                    entryMap[binary] = entry;
                    if(index)
                        dictionaryContent[index] = entry;
                    return;
                }

                for(i=startIndex; i>newIndex; i--)
                {
                    temp = entryMap[dictionaryContent[i-1].binary];
                    temp.index = i>15 ? -1 : temp.index+1;
                    entryMap[dictionaryContent[i-1].binary] = temp;
                    if(i<=15)
                        dictionaryContent[i] = dictionaryContent[i-1];
                }

                entry.index = newIndex;
                entryMap[binary] = entry;
                dictionaryContent[newIndex] = entry;
            }
        }
    }

    /**************************/
    string GetDictionary()
    {
        string dictionary = "";
        for(int i=0; i<16; i++)
        {
            dictionary += dictionaryContent[i].binary;
            if(i<15)
                dictionary += "\r\n";
        }
        return dictionary;
    }

private:
    unordered_map<string,DictionaryEntry> entryMap;
    DictionaryEntry dictionaryContent[16];
    int entryCount;
};

/************************************************
 *  Compression Class
 ************************************************/
class Compression
{
public:
    void StartCompression()
    {
        d.CreateDictionary(COMPRESS_INPUT_FILENAME);
        OpenFiles();
        RunCompression();
        CloseFiles();
    }

private:
    void OpenFiles()
    {
        inputFile.open(COMPRESS_INPUT_FILENAME);
        outputFile.open(COMPRESS_OUTPUT_FILENAME);
    }

    void CloseFiles()
    {
        inputFile.close();
        outputFile.close();
    }

    void RunCompression()
    {

    }

    ifstream inputFile;
    ofstream outputFile;
    Dictionary d;
};

/************************************************
 *  Decompression Class
 ************************************************/
class Decompression
{
public:
    void StartDecompression()
    {
        complete = false;
        d.CreateDictionary(DECOMPRESS_INPUT_FILENAME);
        OpenFiles();
        RunDecompression();
        CloseFiles();
    }

private:
    void OpenFiles()
    {
        inputFile.open(DECOMPRESS_INPUT_FILENAME);
        outputFile.open(DECOMPRESS_OUTPUT_FILENAME);
    }

    void CloseFiles()
    {
        inputFile.close();
        outputFile.close();
    }

    void RunDecompression()
    {
        while(!complete)
        {
            bitset<3> type(ReadBinary(3));
            if(complete)
                break;
            string decompressed = Decompress(type.to_ulong());
            WriteOutput(decompressed);
        }
    }

    string ReadBinary(int length)
    {
        int numRead = 0;
        char c;
        string bits;

        while(numRead!=length)
        {
            inputFile.get(c);
            if(c == '\r' || c == '\n')
                continue;
            if(c == 'x')
            {
                complete = true;
                return "";
            }
            bits.push_back(c);
            numRead++;
        }
        return bits;
    }

    string Decompress(int type)
    {
        string binary = "";

        switch(type)
        {
            case 0:
            {
                binary = ReadBinary(32);
            }
            break;

            case 1:
            {
                bitset<3> amountBits (ReadBinary(3));
                int amount = amountBits.to_ulong()+1;
                for(int i=0; i<amount; i++)
                {
                    binary += lastBinary;
                    if(i != amount-1)
                        binary += "\r\n";
                }
            }
                break;

            case 2:
            {
                bitset<5> start (ReadBinary(5));
                bitset<4> mask (ReadBinary(4));
                bitset<32>fullMask;
                for(int i=0; i<4; i++)
                    fullMask[31-start.to_ulong()-i] = mask[3-i];
                bitset<4> index (ReadBinary(4));
                bitset<32> dictBinary ( d.GetEntry(index.to_string()) );
                dictBinary^=fullMask;
                binary = dictBinary.to_string();
            }
                break;

            case 3:
            {
                bitset<5> mismatchLoc (ReadBinary(5));
                bitset<4> index (ReadBinary(4));
                bitset<32> dictBinary ( d.GetEntry(index.to_string()) );
                int startIndex = mismatchLoc.to_ulong();
                dictBinary[31-startIndex] = !dictBinary[31-startIndex];
                binary = dictBinary.to_string();
            }
                break;

            case 4:
            {
                bitset<5> mismatchLoc (ReadBinary(5));
                bitset<4> index (ReadBinary(4));
                bitset<32> dictBinary ( d.GetEntry(index.to_string()) );
                int startIndex = mismatchLoc.to_ulong();
                dictBinary[31-startIndex] = !dictBinary[31-startIndex];
                dictBinary[31-startIndex-1] = !dictBinary[31-startIndex-1];
                binary = dictBinary.to_string();
            }
                break;

            case 5:
            {
                bitset<5> mismatchLoc (ReadBinary(5));
                bitset<4> index (ReadBinary(4));
                bitset<32> dictBinary ( d.GetEntry(index.to_string()) );
                int startIndex = mismatchLoc.to_ulong();
                dictBinary[31-startIndex] = !dictBinary[31-startIndex];
                dictBinary[31-startIndex-1] = !dictBinary[31-startIndex-1];
                dictBinary[31-startIndex-2] = !dictBinary[31-startIndex-2];
                dictBinary[31-startIndex-3] = !dictBinary[31-startIndex-3];
                binary = dictBinary.to_string();
            }
                break;

            case 6:
            {
                bitset<5> mismatchLoc1 (ReadBinary(5));
                bitset<5> mismatchLoc2 (ReadBinary(5));
                bitset<4> index (ReadBinary(4));
                bitset<32> dictBinary ( d.GetEntry(index.to_string()) );
                int startIndex1 = mismatchLoc1.to_ulong();
                int startIndex2 = mismatchLoc2.to_ulong();
                dictBinary[31-startIndex1] = !dictBinary[31-startIndex1];
                dictBinary[31-startIndex2] = !dictBinary[31-startIndex2];
                binary = dictBinary.to_string();
            }
                break;

            case 7:
            {
                bitset<4> index (ReadBinary(4));
                binary = d.GetEntry(index.to_string());
            }
                break;
        }

        lastBinary = binary;
        return binary;
    }

    void WriteOutput(string output)
    {
        outputFile << output << "\r\n";
    }

    ifstream inputFile;
    ofstream outputFile;
    Dictionary d;
    bool complete;
    string lastBinary;
};

/************************************************
 *  Main Method
 ************************************************/
int main(int argc, char** argv)
{
    int type;
    Compression compress;
    Decompression decompress;

    if(argc != 2)
        cout << "Error: must specify" << endl << "[1] for compression" << endl << "[2] for decompression\n" << endl;
    else
    {
        type = atoi(argv[1]);

        if(type == 1)
            compress.StartCompression();
        else if(type == 2)
            decompress.StartDecompression();
        else
            cout << "Error: must specify" << endl << "[1] for compression" << endl << "[2] for decompression\n" << endl;
    }

    return 0;
}
