/* Header for invoicing.cpp*/


#include <string>
#include <iostream>
#include <exception>
#include <fstream>
#include <vector>
#include <sstream>
#include <set>
#include "hpdf.h"
#include <unordered_map>
#include "DateParse.h"

using namespace std;

vector<vector<string>> create_table(string fname, char delimeter, bool invoice);

vector<HPDF_REAL> setText(HPDF_Page page, HPDF_REAL x, HPDF_REAL y, vector<HPDF_REAL> pos);

string dollarFormat(string number);

string totalDollars(string exchangeExt, string coreExt);


vector<HPDF_REAL> addiPageHeaders(HPDF_Doc pdf, HPDF_Page page, int pageCount);

vector<string> HEADER_LIST{ "Invoice#", "OT" "Man", "Date", "Terms",
"Sales Person", "Ship Via", "SoldTO", "Sold-Name", "Sold-Ad1", "Sold-City", "Sold-St",
"Sold-Zip", "ShipTO", "Ship-Name", "Ship-Ad1", "Ship-Ad2", "Ship-City", "Ship-ST",
"Ship-Zip", "Message1", "Message2", "Message3", "Credit", "BOL" };



class record{
private:
	string invoice_number;
	unordered_map<string, string> headers;
	vector<vector<string>> transactions;


public:
	record(vector<string> invoice, vector<vector<string>> customers){
		invoice_number = invoice[0];
		headers["Invoice#"] = invoice[0];
		headers["OT"] = invoice[1];
		if (invoice[1] == "05")
			headers["PO"] = invoice[24].erase(0, 4);
		else
			headers["PO"] = invoice[3];
		headers["Date"] = invoice[21];
		headers["Terms"] = invoice[30];
		headers["Sales Person"] = invoice[38];
		headers["Ship Via"] = invoice[29];
		headers["SoldTO"] = invoice[31];
		headers["Ship-Name"] = invoice[7];
		headers["ShipTO"] = invoice[6];
		headers["Ship-Ad1"] = invoice[33];
		headers["Ship-Ad2"] = invoice[34];
		headers["Ship-City"] = invoice[35];
		headers["Ship-ST"] = invoice[36];
		headers["Ship-Zip"] = invoice[37];
		headers["Message1"] = invoice[23];
		headers["Message2"] = invoice[24];
		headers["Message3"] = invoice[25];
		headers["Credit"] = invoice[9];
		headers["Sold-Name"] = invoice[42];
		headers["Sold-Ad1"] = invoice[43];
		headers["Sold-City"] = invoice[44];
		headers["Sold-ST"] = invoice[45];
		headers["Sold-Zip"] = invoice[46];
		headers["BOL"] = invoice[39];




	}


	void print_lines(){
		for (auto i = transactions.begin(); i != transactions.end(); i++){
			for (auto j = i->begin(); j != i->end(); j++)
				cout << *j << " ";
			cout << endl;
		}
	}


	string get_header(string key){
		return headers[key];
	}



	void print(){
		cout << this->name() << endl;
		this->print_lines();

	}


	void add_line(vector<string> line){
		transactions.push_back(line);
	}


	//returns list of headers
	vector<string> get_headers(){
		vector<string> header_out;
		for (int i = 0; i < 13; i++){
			header_out.push_back(headers[HEADER_LIST[i]]);
		}
		return header_out;
	}

	string name() { return headers["SoldTO"] + " " + invoice_number; }

	string invoice_total(){
		string ar = "0";

		for (auto i : transactions)
			ar = totalDollars(ar, totalDollars(i[8], totalDollars(i[7], i[6])));
		return ar;
	}

	string productTotal(){
		string pr = "0";
		for (auto i : transactions){
			pr = totalDollars(pr, totalDollars(i[6], i[7]));
		}
		return pr;

	}

	string freight(){
		string fr = "0";
		for (auto i : transactions){
			fr = totalDollars(fr, i[8]);
		}
		return fr;

	}

	vector<vector<string>> get_transactions(){
		return transactions;
	}

	string dueDate(){
		if (this->get_header("Terms") == "")
			return "";
		return dateAdd(this->get_header("Date"), this->get_header("Terms"));
	}

};