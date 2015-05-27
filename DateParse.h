#include <string>
#include <vector>
#include <sstream>
#include <istream>


using namespace std;

string dateAdd(string base, string add){
	vector<string> months = { "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12" };
	vector<int> monthsDays = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	stringstream date(base);
	vector<string> parsedDate;
	string token;
	char delmiter = '/';
	bool parsing = true;
	while (getline(date, token, delmiter)) {
		parsedDate.push_back(token);
	}
	int month = stoi(parsedDate[0]) +1;
	int day = stoi(parsedDate[1]) + stoi(add) - monthsDays[month - 1];
	int year = stoi(parsedDate[2]);
	while (parsing){
		if (month > 11){
			month = 1;
			year += 1;
		}
		if (day > monthsDays[month - 1]){
			day -= monthsDays[month - 1];
			month += 1;
		}
		if (!(day > monthsDays[month - 1] && month > 11))
			break;
	}
	if (month < 10)
		parsedDate[0] = "0" + to_string(month);
	else
		parsedDate[0] = to_string(month);
	if (day < 10)
		parsedDate[1] = "0" + to_string(day);
	else
		parsedDate[1] = to_string(day);
	parsedDate[2] = to_string(year);
	return parsedDate[0] + "/" + parsedDate[1] + "/" + parsedDate[2];
}