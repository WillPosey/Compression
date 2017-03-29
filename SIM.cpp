/* On my honor, I have neither given nor received unauthorized aid on this assignment */

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
    int instrNum;
    int frequency;
    int index;

    DictionaryEntry() : binary("00000000000000000000000000000000"),
                        instrNum(0),
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

    bool InDictionary(string check, string& index)
    {
        bitset<4> indexBits;
        bool found = false;
        for(int i=0; i<16; i++)
        {
            if(dictionaryContent[i].binary.compare(check) == 0)
            {
                found = true;
                indexBits = bitset<4> (i);
                index = indexBits.to_string();
            }
        }
        return found;
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
        int instrNum = 0;

        if(filename.compare(COMPRESS_INPUT_FILENAME) == 0)
        {
            while(getline(file, line))
            {
                instrNum++;
                if(line.length() > 32)
                    line.erase(32,1);
                AddInstance(line, instrNum);
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
    void AddInstance(string binary, int instrNum)
    {
        DictionaryEntry entry,temp;
        unordered_map<string,DictionaryEntry>::iterator it = entryMap.find(binary);

        if(it == entryMap.end())
        {
            entry.binary = binary;
            entry.frequency = 1;
            entry.instrNum = instrNum;
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
            int instrNum = entry.instrNum;
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

                while(((newFreq > dictionaryContent[i].frequency) ||
                      ((newFreq == dictionaryContent[i].frequency) && (instrNum < dictionaryContent[i].instrNum)))
                      && i>=0)
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
    /**************************/
    void StartCompression()
    {
        d.CreateDictionary(COMPRESS_INPUT_FILENAME);
        OpenFiles();
        RunCompression();
        CloseFiles();
    }

private:
    /**************************/
    void OpenFiles()
    {
        inputFile.open(COMPRESS_INPUT_FILENAME);
        outputFile.open(COMPRESS_OUTPUT_FILENAME);
    }

    /**************************/
    void CloseFiles()
    {
        inputFile.close();
        outputFile.close();
    }

    /**************************/
    void RunCompression()
    {
        string line, compressedBinary;
        bitset<32> binary;
        bool isRLE;

        fillLastLine = false;

        while(getline(inputFile,line))
        {
            if(line.length() > 32)
                line.erase(line.length()-1,1);
            binary = bitset<32>(line);
            compressedBinary = Compress(binary, isRLE);
            if(!isRLE)
                WriteOutput(compressedBinary);
        }

        fillLastLine = true;
        WriteOutput("");

        WriteDictionary();
    }

    /**************************/
    string Compress(bitset<32> binary, bool& isRLE)
    {
        static bool firstBinary = true, lastRLE = false, ignoreRLE = false;
        static bitset<32> lastBinary;
        static int RLEcount = 0;
        string compressed, dictionaryIndex, bitmask, location, location2;
        bitset<3> rleBits;

        isRLE = false;

        if(!firstBinary && binary==lastBinary && !ignoreRLE)
        {
            RLEcount++;
            lastRLE = true;
            isRLE = true;
        }
        else if(DictionaryMatch(binary, dictionaryIndex))
            compressed = "111" + dictionaryIndex;
        else if(OneBitMismatch(binary, location, dictionaryIndex))
            compressed = "011" + location + dictionaryIndex;
        else if(TwoBitMismatch(binary, location, dictionaryIndex))
            compressed = "100" + location + dictionaryIndex;
        else if(FourBitMismatch(binary, location, dictionaryIndex))
            compressed = "101" + location + dictionaryIndex;
        else if(Bitmask(binary, location, bitmask, dictionaryIndex))
            compressed = "010" + location + bitmask + dictionaryIndex;
        else if(DoubleBitMismatch(binary, location, location2, dictionaryIndex))
            compressed = "110" + location + location2 + dictionaryIndex;
        else
            compressed = "000" + binary.to_string();

        firstBinary = false;
        ignoreRLE = false;
        lastBinary = binary;

        if(isRLE)
        {
            if(RLEcount < 8)
                compressed = "";
            else
            {
                lastRLE = false;
                rleBits = bitset<3>((RLEcount-1));
                RLEcount = 0;
                compressed = "001" + rleBits.to_string();
                ignoreRLE = true;
                isRLE = false;
            }
        }
        else if(!isRLE && lastRLE)
        {
            lastRLE = false;
            rleBits = bitset<3>((RLEcount-1));
            RLEcount = 0;
            compressed = "001" + rleBits.to_string() + compressed;
        }

        return compressed;
    }

    /**************************/
    bool DictionaryMatch(bitset<32> binary, string& dictionaryIndex)
    {
        return d.InDictionary(binary.to_string(), dictionaryIndex);
    }

    /**************************/
    bool OneBitMismatch(bitset<32> binary, string& location, string& dictionaryIndex)
    {
        bitset<4> index(0);
        bitset<5> locationBits(0);
        bitset<32> dictionaryEntry, xorResult;
        int count = 0;

        for(int i=0; i<16; i++)
        {
            index = bitset<4>(i);
            dictionaryEntry = bitset<32>(d.GetEntry(index.to_string()));
            xorResult = binary^dictionaryEntry;
            if(xorResult.count()==1)
            {
                for(int j=31; j>=0; j--)
                {
                    if(xorResult[j])
                        break;
                    count++;
                }
                dictionaryIndex = index.to_string();
                locationBits = bitset<5>(count);
                location = locationBits.to_string();
                return true;
            }
        }
        return false;
    }

    /**************************/
    bool TwoBitMismatch(bitset<32> binary, string& location, string& dictionaryIndex)
    {
        bitset<4> index(0);
        bitset<5> locationBits(0);
        bitset<32> dictionaryEntry, xorResult;
        int count=0;

        for(int i=0; i<16; i++)
        {
            index = bitset<4>(i);
            dictionaryEntry = bitset<32>(d.GetEntry(index.to_string()));
            xorResult = binary^dictionaryEntry;
            if(xorResult.count()==2)
            {
                for(int j=31; j>=1; j--)
                {
                    if(xorResult[j] && xorResult[j-1])
                        break;
                    else if(xorResult[j])
                        return false;
                    count++;
                }
                dictionaryIndex = index.to_string();
                locationBits = bitset<5>(count);
                location = locationBits.to_string();
                return true;
            }
        }
        return false;
    }

    /**************************/
    bool FourBitMismatch(bitset<32> binary, string& location, string& dictionaryIndex)
    {
        bitset<4> index(0);
        bitset<5> locationBits(0);
        bitset<32> dictionaryEntry, xorResult;
        int count=0;

        for(int i=0; i<16; i++)
        {
            index = bitset<4>(i);
            dictionaryEntry = bitset<32>(d.GetEntry(index.to_string()));
            xorResult = binary^dictionaryEntry;
            if(xorResult.count()==4)
            {
                for(int j=31; j>=1; j--)
                {
                    if(xorResult[j] && xorResult[j-1] && xorResult[j-2] && xorResult[j-3])
                        break;
                    else if(xorResult[j])
                        return false;
                    count++;
                }
                dictionaryIndex = index.to_string();
                locationBits = bitset<5>(count);
                location = locationBits.to_string();
                return true;
            }
        }
        return false;
    }

    /**************************/
    bool Bitmask(bitset<32> binary, string& location, string& bitmask, string& dictionaryIndex)
    {
        bitset<4> index(0), mask;
        bitset<5> locationBits(0);
        bitset<32> dictionaryEntry, xorResult, fullMask;
        int count=0;

        for(int i=0; i<16; i++)
        {
            count = 0;
            index = bitset<4>(i);
            dictionaryEntry = bitset<32>(d.GetEntry(index.to_string()));
            for(int j=31; j>=4; j--)
            {
                for(int k=1; k<=15; k++)
                {
                    mask = bitset<4>(k);
                    fullMask = bitset<32>(k);
                    fullMask<<=(j-3);
                    xorResult = binary^fullMask;
                    if(xorResult==dictionaryEntry)
                    {
                        bitmask = mask.to_string();
                        dictionaryIndex = index.to_string();
                        locationBits = bitset<5>(count);
                        location = locationBits.to_string();
                        return true;
                    }
                }
                count++;
            }
        }
        return false;
    }

    /**************************/
    bool DoubleBitMismatch(bitset<32> binary, string& location, string& location2, string& dictionaryIndex)
    {
        bitset<4> index(0);
        bitset<5> locationBits(0), location2Bits(0);
        bitset<32> dictionaryEntry, xorResult;
        int numFound, count = 0, count2 = 0;

        for(int i=0; i<16; i++)
        {
            index = bitset<4>(i);
            dictionaryEntry = bitset<32>(d.GetEntry(index.to_string()));
            xorResult = binary^dictionaryEntry;
            if(xorResult.count()==2)
            {
                numFound = 0;
                for(int j=31; j>=1; j--)
                {
                    if(xorResult[j])
                    {
                        numFound++;
                        if(numFound==2)
                            break;
                    }
                    count2++;
                    if(numFound==0)
                        count++;
                }
                dictionaryIndex = index.to_string();
                locationBits = bitset<5>(count);
                location = locationBits.to_string();
                location2Bits = bitset<5>(count2);
                location2 = location2Bits.to_string();
                return true;
            }
        }
        return false;
    }

    /**************************/
    void WriteOutput(string compressedBinary)
    {
        static int outputCount = 0;
        int newOutputCount = outputCount + compressedBinary.length();
        int space = 32- outputCount;
        int nextLine = compressedBinary.length()-space;

        if(fillLastLine)
        {
            //
            //outputFile << " ";
            //

            for(int i=outputCount; i<32; i++)
                outputFile << "0";
            outputFile << "\r\n";
        }
        else if(newOutputCount > 32)
        {
            outputFile << compressedBinary.substr(0,space) << "\r\n";
            if(nextLine <= 32)
            {
                outputFile << compressedBinary.substr(space, nextLine);
                outputCount = (nextLine==32) ? 0 : nextLine;
            }
            else
            {
                outputFile << compressedBinary.substr(space, 32) << "\r\n";
                outputFile << compressedBinary.substr(space+32, nextLine-32);
                outputCount = nextLine-32;
            }

            //
            //outputFile << " ";
            //

            if(outputCount > 32)
                cout << "output Count: " << outputCount << " length: " << compressedBinary.length() << " space: " << space << endl;
        }
        else
        {
            outputFile << compressedBinary;

            //
            //outputFile << " ";
            //

            if(newOutputCount == 32)
                outputFile << "\r\n";
            outputCount = (newOutputCount==32) ? 0 : newOutputCount;
        }
    }

    /**************************/
    void WriteDictionary()
    {
        outputFile << "xxxx" << "\r\n";
        outputFile << d.GetDictionary() << "\r\n";
    }

    ifstream inputFile;
    ofstream outputFile;
    bool fillLastLine;
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
