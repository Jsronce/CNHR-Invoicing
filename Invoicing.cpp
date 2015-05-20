
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
"Sales Person", "Ship Via", "SoldTO", "Sold-Name", "Sold-Ad1", "Sold-City", "Sold-St",
"Sold-Zip" ,"ShipTO", "Ship-Name" ,"Ship-Ad1", "Ship-Ad2", "Ship-City", "Ship-ST", 
"Ship-Zip", "Message1", "Message2", "Message3", "Credit", "BOL" };

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
	record(vector<string> invoice, vector<vector<string>> customers){
		invoice_number = invoice[0];
		headers["Invoice#"] = invoice[0];
		headers["OT"] = invoice[1];
		headers["Man_PO"] = invoice[3];
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

		for (int i = 0; i < customers.size(); i++){
			if (customers[i][0] == headers["SoldTO"]){
				headers["Sold-Name"] = customers[i][1];
				headers["Sold-Ad1"] = customers[i][3];
				headers["Sold-City"] = customers[i][4];
				headers["Sold-ST"] = customers[i][5];
				headers["Sold-Zip"] = customers[i][6];
				break;
			}
		}



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
vector<vector<string>> create_table(string fname, char delimeter='\t', bool invoice = true){
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
				if (INVOICED_ORDER_TYPES.count(line[1]) != 0 || MOVE_TO_AP_TYPES.count(line[1]) != 0 || !invoice)
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

vector<HPDF_REAL> setText(HPDF_Page page, HPDF_REAL x, HPDF_REAL y, vector<HPDF_REAL> pos){
	vector<HPDF_REAL> Newpos = pos;
	HPDF_REAL xPos = pos[0];
	HPDF_REAL yPos = pos[1];
	HPDF_Page_MoveTextPos(page, x, y);
	xPos = xPos + x;
	yPos = yPos - y;
	Newpos[0] = xPos;
	Newpos[1] = yPos;
	return Newpos;
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


		//Future Create Page Method
		HPDF_Page page = HPDF_AddPage(pdf);
		HPDF_REAL height;
		

		height = HPDF_Page_GetHeight(page);
		HPDF_REAL heightMargin = height*.95;
		HPDF_REAL width;
		width = HPDF_Page_GetWidth(page);
		HPDF_REAL widthMargin = width*.95;
		HPDF_Page_DrawImage(page, logo, width*.05, height*.95 -56, 160, 56);
		vector<HPDF_REAL> pos = { 0, 0 };

		
		HPDF_Page_BeginText(page);
		pos = setText(page, width*.05 + 160 + 20, height*.95 -16, pos);


		//HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		//HPDF_Page_ShowText(page, "Please Remit to:");
		//pos = setText(page, 0, -12, pos);

		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica-Bold", NULL), 16);
		HPDF_Page_ShowText(page, "CNH Reman, LLC");
		pos = setText(page, 0, -16, pos);
	
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		HPDF_Page_ShowText(page, "2702 N FR 123, Suite A");
		pos = setText(page, 0, -12, pos);

		HPDF_Page_ShowText(page, "Springfield MO, 65807");


		//BELOW HERE NEEDS TO BE EDITED FOR Re-arrangement
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica-Bold", NULL), 16);
		const char* title = "Invoice:";
		if (invoice->get_header("Credit") == "C")
			title = "Credit:";
		string strstuf = string(title) + " " + invoice->get_header("Credit") + invoice->get_header("Invoice#");
		const char* stuff = strstuf.c_str();
		HPDF_REAL space = HPDF_Page_TextWidth(page, title);

		pos = setText(page, width*.95 - pos[0] - HPDF_Page_TextWidth(page, title), 28, pos);
	
		
		HPDF_Page_ShowText(page, title);

		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		string tempStr = "#" + invoice->get_header("Credit") + invoice->get_header("Invoice#");
		const char* invNum = tempStr.c_str();
		pos = setText(page, width*.95 - pos[0] - HPDF_Page_TextWidth(page, invNum), -16, pos);
		HPDF_Page_ShowText(page, invNum);

		tempStr = "Date: " + invoice->get_header("Date");
		const char* date = tempStr.c_str();
		pos = setText(page, width*.95 - pos[0] - HPDF_Page_TextWidth(page, date), -12, pos);
		HPDF_Page_ShowText(page, date);

		HPDF_Page_EndText(page);

		
		//End Create Page

		HPDF_Page_BeginText(page);
		pos = { 0, 0 };
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 10);
		pos = setText(page, widthMargin, heightMargin - 86, pos);
		HPDF_Page_ShowText(page, "Sold to");
		pos = setText(page, 10, -10, pos);
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		HPDF_Page_ShowText(page, invoice->get_header("SoldTO").c_str());
		HPDF_Page_ShowText(page, invoice->get_header("SoldTO").c_str());
		HPDF_Page_ShowText(page, invoice->get_header("SoldTO").c_str());
		HPDF_Page_ShowText(page, invoice->get_header("SoldTO").c_str());


		
		
		
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
	vector<vector<string>> customers = create_table(path + "\\customer.csv", ',', false);
	
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
			invoice = new record(invoices[i], customers);
		}
		previous = *current;
		delete current;
	}






	return 0;

}