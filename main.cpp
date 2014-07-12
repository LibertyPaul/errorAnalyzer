#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <limits>


using namespace std;

#include "ErrorAnalyzer.hpp"

void man(const string &progName){
	cout << "Usage: " << progName << " source_file destination_file log_file" << endl;
	cout << "Example:" << endl << progName << " src.txt dst.txt log.txt" << endl;
}

int main(int argc, char **argv){
	string progName(argv[0]);
	if(argc < 4){
		man(progName);
		return 1;
	}

	string srcFileName(argv[1]);
	string dstFileName(argv[2]);
	string logFileName(argv[3]);

	ErrorAnalyzer ea(srcFileName, dstFileName, logFileName, 30, 40, 20);
	ea.analyze();

    return 0;
}
