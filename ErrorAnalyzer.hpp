#ifndef ERRORANALYZER_HPP_INCLUDED
#define ERRORANALYZER_HPP_INCLUDED


#define LOG

enum class ErrorType{
	addition,
	insertion,
	loss,
};

struct range{
	vector<uint8_t>::iterator beg, end;

	range(vector<uint8_t>::iterator beg, vector<uint8_t>::iterator end): beg(beg), end(end){}

	uint64_t getLength() const{
		return this->end - this->beg;
	}
};

struct Error{
	ErrorType type;
	int32_t length;

	Error(ErrorType type, int32_t length): type(type), length(length){}
};



class ErrorAnalyzer{
	vector<uint8_t> src, dst;
	#ifdef LOG
		ofstream log;
	#endif // LOG

	uint16_t insertionErrorLowerBound;
	uint16_t insertionErrorHigherBound;
	uint64_t insertionTestRangeSize;


	uint64_t additionalErrors = 0;
	uint64_t insertionErrors = 0;
	uint64_t lossErrors = 0;//ошибки потери тоже инъективные.


	uint64_t calcErrors(const range src, const range dst) const;
	Error getErrorType(const range src, const range dst);

	ostream &showStatistics(ostream &o);

	int32_t insertionsAnalysis(const range src, const range dst) const;


public:

	ErrorAnalyzer(
		const vector<uint8_t> &src,
		const vector<uint8_t> &dst,
		const string &logFileName,
		uint16_t insertionErrorLowerBound = 15,
		uint16_t insertionErrorHigherBound = 35,
		uint64_t insertionTestRangeSize = 7
	);

	ErrorAnalyzer(
		const string &srcFileName,
		const string &dstFileName,
		const string &logFileName,
		uint16_t insertionErrorLowerBound = 15,
		uint16_t insertionErrorHigherBound = 35,
		uint64_t insertionTestRangeSize = 7
	);

	void analyze();

	uint64_t getErrorCount() const;

};



#endif // ERRORANALYZER_HPP_INCLUDED
