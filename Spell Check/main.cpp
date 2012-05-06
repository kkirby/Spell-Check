#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include "sqlite3.h"

std::string getcwd_string( void ) {
   char buff[PATH_MAX];
   getcwd( buff, PATH_MAX );
   std::string cwd( buff );
   return cwd;
}

float CompareWords(std::string firstString, std::string secondString){
	std::string compareFrom, compareTo;
	if(firstString.size() > secondString.size()){
		compareFrom = firstString;
		compareTo = secondString;
	}
	else {
		compareFrom = secondString;
		compareTo = firstString;
	}
	
	int letterCount = compareFrom.size();
	
	float matchCount = 0;
	
	int lastMatchIndex = 0, lastMatchKey = 0;
	
	int lastContinutyKey = -1;
	float continuity = 0;
	
	float continuityWeight = (letterCount * letterCount) / 400.0;
	continuityWeight = continuityWeight > 0.80 ? 0.80 : continuityWeight;
	float totalMatchWeight = 1.0 - continuityWeight;
	
	for(int key = 0; key < letterCount; key++){
		char currentLetter = compareFrom[key];
		int indexOfCurrentLetter = compareTo.find_first_of(currentLetter);
		if(indexOfCurrentLetter != -1){
			matchCount++;
			if(lastMatchIndex == indexOfCurrentLetter - 1){
				if(lastContinutyKey != key - 1)
					continuity++;
				continuity++;
				lastContinutyKey = key;
			}
			lastMatchKey = key;
			lastMatchIndex = indexOfCurrentLetter;
			compareTo.replace(indexOfCurrentLetter,1," ");
		}
	}
	return (continuity / matchCount + continuity / letterCount) / 2.0 * continuityWeight + matchCount / letterCount * totalMatchWeight;
}

class WordMap{
public:
	float percent;
	std::string word;
	WordMap(float inputPercent, std::string inputWord){
		percent = inputPercent;
		word = inputWord;
	}
	
	inline bool operator<(const WordMap& comparison) const {
		return !(percent < comparison.percent);
    }
};


class WordDatabase{
private:
    sqlite3 *databaseResource;
    char *zErrMsg;
    bool databaseOpen;
public:
    WordDatabase(std::string tablename="init.db"){
		int returnCode = sqlite3_open(tablename.c_str(), &databaseResource);
		if(returnCode){
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(databaseResource));
			sqlite3_close(databaseResource);
			databaseOpen = false;
		}
		else databaseOpen= true;
    }
	
	bool doesWordExist(std::string word){
		if(databaseOpen){
			char query[100];
			sprintf(query, "SELECT word FROM words WHERE word = \"%s\" LIMIT 1", word.c_str());
			int rowCount, columnCount;
			char **result;
			sqlite3_get_table(databaseResource,query,&result,&rowCount,&columnCount,&zErrMsg);
			return rowCount;
		}
		else throw "Database not open.";
	}
	
	std::vector< std::string > getWordsByLength(int min, int max){
		if(databaseOpen){
			std::vector < std::string > words;
			char query[100];
			sprintf(query, "SELECT word FROM words WHERE length(word) >= %i AND length(word) <= %i", min, max);
			int rowCount, columnCount;
			char **result;
			sqlite3_get_table(databaseResource,query,&result,&rowCount,&columnCount,&zErrMsg);
			for(int i = 0; i < rowCount; i++)
				words.push_back(result[i+columnCount]);
			return words;
		}
		else throw "Database not open.";
	}
};


int main (int argc, char * const argv[]) {
	int startTime = clock();
	std::string inputWord;
	bool debug;
	
	if(argv[1] != NULL){
		if(std::string(argv[1]) == "-d"){
			debug = true;
			if(argv[2] == NULL){
				std::cout << "No word given to spell check.\n";
				return -1;
			}
			else inputWord = argv[2];
		}
		else {
			inputWord = argv[1];
			debug = false;
		}
	}
	else {
		std::cout << "No word given to spell check.\n";
		return -1;
	}
	
	if(argv[2] != NULL && debug == false){
		std::string inputWord1 = argv[1];
		std::string inputWord2 = argv[2];
		float percent = CompareWords(inputWord1,inputWord2);
		std::cout << "The comparison between \"" << inputWord1 << "\" and \"" << inputWord2 << "\" is: " << std::setprecision(3) << percent * 100 << "%.\n";
		return 0;
	}
	else {
		WordDatabase wordDatabase("./word_database.sqlite3");
		if(wordDatabase.doesWordExist(inputWord)){
			std::cout << "Word spelled correctly.\n";
			return 0;
		}
		else {
			std::vector< std::string > words = wordDatabase.getWordsByLength(inputWord.size() - 5,inputWord.size() + 5);
			std::vector< WordMap > filteredWords;
			if(words.size() > 0){
				for(int index = 0; index < words.size(); index++){
					float comparePercent = CompareWords(inputWord,words[index]);
					if(comparePercent >= 0.74)
						filteredWords.push_back(WordMap(comparePercent,words[index]));
				}
				
				if(filteredWords.size() > 0){
					sort(filteredWords.begin(),filteredWords.end());
					std::cout << "Did you mean:\n";
					for(int index = 0; index < filteredWords.size(); index++){
						if(index == 10)break;
						std::cout << filteredWords[index].word << "\n";
					}
					if(debug == true){
						std::cout << "\n-- Debug --\n";
						std::cout << words.size() << " rows found.\n";
						std::cout << "Finished in " << std::setprecision(2) << (float)(((float)clock() - (float)startTime) / 1000000.0) << " seconds.\n";
					}
					return 0;
				}
				else {
					std::cout << "No matching words found. Try spelling it a litte better.\n";
					return -1;
				}
			}
			else {
				std::cout << "No words found from the database.\n";
				return -1;
			}
		}
		
	}
}