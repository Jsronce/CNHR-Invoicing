
#include <string>
#include <iostream>
#include <exception>
#include <fstream>
#include <vector>
#include <set>
#include "hpdf.h"
#include <unordered_map>


using namespace std;

//Error Handler for LibHPDF
#ifdef HPDF_DLL
void __stdcall
#else
void
#endif
error_handler(HPDF_STATUS error_no,
	HPDF_STATUS detail_no,
	void *user_data)
{
	printf("ERROR: error_no=%04X, detail_no=%u\n",
		(HPDF_UINT)error_no, (HPDF_UINT)detail_no);

	throw std::exception();
}

//Used Headers for invoicing
vector<string> HEADER_LIST{ "Invoice#", "OT" "Man_PO", "Date", "Terms",
"Sales Person", "Ship Via", "SoldTO", "ShipTO", "Ship-Ad1",
"Ship-Ad2", "Ship-City", "Ship-ST", "Ship-Zip", "Message1",
"Message2", "Message3", "Credit" };

//Determines whether invoice is sent or not
set<string> INVOICED_ORDER_TYPES = { "05", "06", "07", "14" ,"08", "15", "71" };
set<string> MOVE_TO_AP_TYPES = { "70", "73", "74" };


//A class to contain all of the data for an invoice
//Header values are in a dictionary
//Rows are stored in a 2D Vector
//Used for easy minipulation during Invoice generation
class record{
private:
	string invoice_number;
	unordered_map<string, string> headers;
	vector<vector<string>> transactions;
	

public:
	record(vector<string> invoice){
		invoice_number = invoice[0];
		headers["Invoice#"] = invoice[0];
		headers["OT"] = invoice[1];
		headers["Man_PO"] = invoice[2];
		headers["Date"] = invoice[21];
		headers["Terms"] = invoice[26];
		headers["Sales Person"] = invoice[38];
		headers["Ship Via"] = invoice[25];
		headers["SoldTO"] = invoice[31];
		headers["ShipTO"] = invoice[7];
		headers["Ship-Ad1"] = invoice[33];
		headers["Ship-Ad2"] = invoice[34];
		headers["Ship-City"] = invoice[35];
		headers["Ship-ST"] = invoice[36];
		headers["Ship-Zip"] = invoice[37];
		headers["Message1"] = invoice[23];
		headers["Message2"] = invoice[24];
		headers["Message3"] = invoice[25];
		headers["Credit"] = invoice[9];



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

	string name() { return headers["SoldTO"]+ " " + invoice_number; }

	int invoice_total(){
		int ar = 0;

		for (auto i : transactions)
			ar += atoi((i[4] + i[5]).c_str())*atoi(i[1].c_str());
		return ar;
	}
};



//adding a line here


//Fucntion to create a table of values from a delimited file
//Takes File as fname and char Delimiter defaults to '\t'(tab)
vector<vector<string>> create_table(string fname, char delimeter='\t'){
	cout << "Importing File";
	ifstream infile;
	infile.open(fname, ios::in);
	if (!infile.is_open())//open file and check exit if file not found
	{
		cout << "File Error:File Not Found" << endl;
		cout << "Press any key to exit....";
		cin.get();
		exit(0);
	}
	char token;
	string record;
	vector<vector<string>> data;
	vector<string> line;
	//Iterates through file adding valid tokens to lines and valid lines to data
	//returns data, a 2d vector of strings containing the delmited data by line
	do
	{
		token = infile.get();
		if (token == delimeter || token == '\n'){
			line.push_back(record);
			record = "";
			if (token == '\n'){
				if (INVOICED_ORDER_TYPES.count(line[1]) != 0 || MOVE_TO_AP_TYPES.count(line[1]) != 0)
					data.push_back(line);
				line.clear();
			}
		}
		else
			record += token;
	} while (!infile.eof());
	infile.close();
	return data;
}

int create_PDF(record* invoice, vector<vector<string>> customers){
	
	
	
	HPDF_Doc pdf = HPDF_New(error_handler, NULL);
	if (!pdf) {
		printf("error: cannot create PdfDoc object\n");
		return 1;
	}
	try{
		const char* image_path = "\\\\psserver1\\CNHR_Depts\\Accounting\\Shipment Reports\\Processing\\CNHI Reman Logo Compressed.jpg";
		HPDF_Image logo = HPDF_LoadJpegImageFromFile(pdf, image_path);
		HPDF_Page page = HPDF_AddPage(pdf);
		HPDF_REAL height;
		height = HPDF_Page_GetHeight(page);
		HPDF_REAL width;
		width = HPDF_Page_GetWidth(page);
		HPDF_Page_DrawImage(page, logo, 0, HPDF_Page_GetHeight(page)- 56, 160, 56);
		
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 24);
		const char* title = "Invoice:";
		if (invoice->get_header("Credit") == "C")
			title = "Credit:";
		string strstuf = string(title) + " " + invoice->get_header("Credit") + invoice->get_header("Invoice#");
		const char* stuff = strstuf.c_str();
		HPDF_REAL space = HPDF_Page_TextWidth(page, title);
		HPDF_Page_BeginText(page);
		HPDF_Page_MoveTextPos(page, (width - HPDF_Page_TextWidth(page, stuff)) * 4 / 9, height*.95);
		HPDF_Page_ShowText(page, stuff);
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		HPDF_Page_MoveTextPos(page, 0, -24);
		HPDF_Page_ShowText(page, ("Date: " + invoice->get_header("Date")).c_str());
		HPDF_Page_MoveTextPos(page, width / 3, 36);

		HPDF_Page_ShowText(page, "Please Remit to CNH Reman, LLC");
		HPDF_Page_MoveTextPos(page, 0, -12);
		HPDF_Page_ShowText(page, "2702 N FR 123, Suite A");
		HPDF_Page_MoveTextPos(page, 0, -12);
		HPDF_Page_ShowText(page, "Springfield MO, 65807");
		HPDF_Page_EndText(page);
		HPDF_Page_SetLineWidth(page, 1);
		HPDF_Page_Rectangle(page, (width - HPDF_Page_TextWidth(page, "Please Remit to CNH Reman, LLC")) -30, height*.95 - 22, width + 1, height + 1);
		HPDF_Page_Stroke(page);
		
		
		
		
		
		
		
		
		
		
		HPDF_SaveToFile(pdf, (invoice->name() + ".pdf").c_str());

	}
	catch (...){
		HPDF_Free(pdf);
		return 1;
	}
	
	
	
	HPDF_Free(pdf);

	return 0;



}




int main(int argc, char **argv)
{

	string path = "\\\\psserver1\\CNHR_Depts\\Accounting\\Shipment Reports\\Processing";
	vector<vector<string>> invoices = create_table(path + "\\invoice.csv", '\t');
	vector<vector<string>> customers = create_table(path + "\\customer.csv", ',');
	
	string previous;
	record* invoice = NULL;
	vector<string> temp;
	for (int i = 1; i < invoices.size(); i++){
		string* current = new string( invoices[i][0]);
		if (*current == previous){//If this line is associated with the current invoice add to it
			string qty = invoices[i][10];
			string part = invoices[i][4];
			if (part != "@MSG"  && qty != "0")
			{
				//		 PSO			 QTY   Part    DESCRIPTION       EXCHANGE			CORE
				temp = { invoices[i][40], qty, part, invoices[i][5], invoices[i][12], invoices[i][14] };
				invoice->add_line(temp);
			}
		

		}
		else{//save the invoice and delete the record to release the memory
			if (invoice != NULL){
				if (invoice->invoice_total() != 0){
					create_PDF(invoice, customers);
					invoice->print();
				}
				delete invoice;
			}
			//create new record
			invoice = new record(invoices[i]);
		}
		previous = *current;
		delete current;
	}






	return 0;

}