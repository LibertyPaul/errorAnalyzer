#include <vector>
#include <sstream>
#include <fstream>
#include <limits>


using namespace std;

#include "ErrorAnalyzer.hpp"



ErrorAnalyzer::ErrorAnalyzer(
	const vector<uint8_t> &src,
	const vector<uint8_t> &dst,
	const string &logFileName,
	uint16_t insertionErrorLowerBound,
	uint16_t insertionErrorHigherBound,
	uint64_t insertionTestRangeSize
):
	src(src),
	dst(dst),
	#ifdef LOG
		log(logFileName, ios_base::out | ios_base::trunc),
	#endif // LOG
	insertionErrorLowerBound(insertionErrorLowerBound),
	insertionErrorHigherBound(insertionErrorHigherBound),
	insertionTestRangeSize(insertionTestRangeSize){}


ErrorAnalyzer::ErrorAnalyzer(
	const string &srcFileName,
	const string &dstFileName,
	const string &logFileName,
	uint16_t insertionErrorLowerBound,
	uint16_t insertionErrorHigherBound,
	uint64_t insertionTestRangeSize
):
	#ifdef LOG
		log(logFileName, ios_base::out | ios_base::trunc),
	#endif // LOG
	insertionErrorLowerBound(insertionErrorLowerBound),
	insertionErrorHigherBound(insertionErrorHigherBound),
	insertionTestRangeSize(insertionTestRangeSize){

	ifstream srcFile(srcFileName), dstFile(dstFileName);

	while(srcFile.eof() == false){
		uint8_t t;
		srcFile >> t;
		src.push_back(t);
	}

	srcFile.close();

	while(dstFile.eof() == false){
		uint8_t t;
		dstFile >> t;
		dst.push_back(t);
	}

	dstFile.close();
}



uint64_t ErrorAnalyzer::calcErrors(const range src, const range dst) const{
	uint64_t errorCount = 0;
	for(auto is = src.beg, id = dst.beg; is != src.end && id != dst.end; ++is, ++id)
		if(*is != *id)
			++errorCount;
	return errorCount;
}

int32_t ErrorAnalyzer::insertionsAnalysis(const range src, const range dst) const{//вычислает множественный сдвиг. возвращает смещение с наименьшим числом ошибок.
	uint16_t rangeSize = min(src.end - src.beg, dst.end - dst.beg);

	uint16_t minErrors = 100;//%
	int32_t result = 0;
	for(uint16_t i = 0; i < rangeSize / 2; ++i){
		uint64_t errorsCount = calcErrors(range(src.beg + i, src.end - (rangeSize / 2 - i)), dst);//LOSS src.end - (rangeSize / 2 - i) - для равного доверительного интервала
		uint16_t currentErrors = errorsCount * 100 / (rangeSize - i);
		if(currentErrors < minErrors){
			minErrors = currentErrors;
			result = -i;
		}
	}

	for(uint16_t i = 1; i < rangeSize / 2; ++i){
		uint64_t errorsCount = calcErrors(src, range(dst.beg + i, dst.end - (rangeSize / 2 - i)));//INSERTION src.end - (rangeSize / 2 - i) - для равного доверительного интервала
		uint16_t currentErrors = errorsCount * 100 / (rangeSize - i);
		if(currentErrors < minErrors){
			minErrors = currentErrors;
			result = i;
		}
	}

	return result;
}




Error ErrorAnalyzer::getErrorType(const range src, const range dst){
	uint64_t rangeSize = min(src.getLength(), dst.getLength());
	uint64_t errorCount = calcErrors(src, dst);
	if(static_cast<double>(errorCount * 100) / rangeSize < insertionErrorHigherBound){//детектирование инъективной ошибки по верхней границе
		return Error(ErrorType::addition, 1);
	}
	else{
		int32_t insertionSize = insertionsAnalysis(src, dst);

		if(insertionSize == 0)
			return Error(ErrorType::addition, 1);
		else if(insertionSize < 0)
			return Error(ErrorType::loss, -insertionSize);
		else
			return Error(ErrorType::insertion, insertionSize);

	}
}


void ErrorAnalyzer::analyze(){
	#ifdef LOG
	log << "Channel error analyzer." << endl << "Pavel \"LibertyPaul\" Yazev. 2014." << endl << endl;
	log << "Parameters: " << endl << "insertionErrorLowerBound = " << insertionErrorLowerBound << "%" << endl;
	log << "insertionErrorHigherBound = " << insertionErrorHigherBound << "%" << endl;
	log << "insertionTestRangeSize = " << insertionTestRangeSize << endl;
	log << endl << "Src length = " << src.size() << endl;
	log << "Dst length = " << dst.size() << endl;
	log << "Size difference: " << static_cast<int64_t>(dst.size() - src.size()) << endl;


	log << endl << endl << endl;
	#endif // LOG
	for(auto is = src.begin(), id = dst.begin(); is != src.end() && id != dst.end(); ++is, ++id){
		if(*is != *id){
			#ifdef LOG
			log << "Error found at " << id - dst.begin() << "\t";
			#endif // LOG

			range srcRange(is, min(is + insertionTestRangeSize, src.end()));
			range dstRange(id, min(id + insertionTestRangeSize, dst.end()));

			Error error = getErrorType(srcRange, dstRange);
			switch(error.type){

			case ErrorType::addition:{
				uint32_t pre = calcErrors(srcRange, dstRange);
				++additionalErrors;

				#ifdef LOG
				log << " Type: ADDITION\t\t\tFollowing errors: " << pre * 100 / min(srcRange.getLength(), dstRange.getLength()) << "%" << endl;
				#endif // LOG
				break;
			}

			case ErrorType::insertion:{
				uint32_t pre = calcErrors(srcRange, dstRange);
				uint32_t post = calcErrors(range(is, min(is + insertionTestRangeSize, src.end())), range(id + error.length, min(id + insertionTestRangeSize + error.length, dst.end())));//УЖОС


				if(post < pre){
					insertionErrors += error.length;
					id += error.length;

					#ifdef LOG
					log << " Type: INSERTION * " << error.length << "\t\tFollowing errors: " << pre * 100 / min(srcRange.getLength(), dstRange.getLength()) << "%\t" << pre << " -> " << post;
					if(static_cast<double>(post * 100) / insertionTestRangeSize < insertionErrorLowerBound)
						log << "\tSUCCESS" << endl;
					else
						log << "\tFAIL" << endl;
					#endif // LOG

				}
				else{
					++additionalErrors;
					#ifdef LOG
					log << " Type: ADDITION, probably\tFollowing errors: " << pre * 100 / min(srcRange.getLength(), dstRange.getLength()) << "%" << endl;
					#endif
				}
				break;
			}
			case ErrorType::loss:{
				uint32_t pre = calcErrors(srcRange, dstRange);
				uint32_t post = calcErrors(range(is + error.length, min(is + insertionTestRangeSize + error.length, src.end())), range(id, min(id + insertionTestRangeSize, dst.end())));//УЖОС



				if(post < pre){
					lossErrors += error.length;
					is += error.length;
					#ifdef LOG
					log << " Type: LOSS * " << error.length << "\t\t\tFollowing errors: " << pre * 100 / min(srcRange.getLength(), dstRange.getLength()) << "%\t" << pre << " -> " << post;
					if(static_cast<double>(post * 100) / insertionTestRangeSize <= insertionErrorLowerBound)
						log << "\tSUCCESS" << endl;
					else
						log << "\tFAIL" << endl;
					#endif // LOG
				}
				else{
					++additionalErrors;
					#ifdef LOG
					log << " Type: ADDITION, probably\tFollowing errors: " << pre * 100 / min(srcRange.getLength(), dstRange.getLength()) << "%" << endl;
					#endif
				}


				break;

			}
			}

		}
	}
	#ifdef LOG
	log << "Additional errors: " << additionalErrors << "(" << static_cast<double>(additionalErrors * 100) / src.size() << "%)" << endl;
	log << "Insertional errors: " << insertionErrors << "(" << static_cast<double>(insertionErrors * 100) / src.size() << "%)" << endl;
	log << "Loss errors: " << lossErrors << "(" << static_cast<double>(lossErrors * 100) / src.size() << "%)" << endl;

	log.close();
	#endif // LOG
}




uint64_t ErrorAnalyzer::getErrorCount() const{
	return additionalErrors + insertionErrors + lossErrors;
}


