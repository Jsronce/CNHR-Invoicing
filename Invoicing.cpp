/*Invocing.Exe
Summary:
A project that takes invoice data downloaded from the CNH Reman
PICK System and creates PDF Invoices.  

Description:
Takes invoicing data from a csv named invoice.csv located in:
"\\\\psserver1\\CNHR_Depts\\Accounting\\Shipment Reports\\Processing"
that is delimited by tabs.  Creates an Invoice object for each invoice
and uses that object to create a PDF Invoice. 

Dependances: 
Libpdfs: a LibHaru Library for PDF writing
Boost: a c++ advanced library

Author: John Sronce
Last Edit: 5/22/15
*/














#include <string>
#include <iostream>
#include <exception>
#include <fstream>
#include <vector>
#include <sstream>
#include <set>
#include "hpdf.h"
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "invoice.h"


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



//A class to contain all of the data for an invoice
//Header values are in a dictionary
//Rows are stored in a 2D Vector
//Used for easy minipulation during Invoice generation




//Determines whether invoice is sent or not
set<string> INVOICED_ORDER_TYPES = { "05", "06", "07", "14", "08", "15","53" ,"71","75" };
set<string> MOVE_TO_AP_TYPES = { "70", "73", "74" };
vector<string> MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
set<string> MOVE_TO_AP_CUST = { "RM6210", "RM2620", "RM4800", "RM7800", "rm2610", "rm2620", "rm4800", "rm7800" };


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
	/*Takes positon adustmetns to the HPDF_Page and the current position vector, and returns
	the updates position.*/
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

string dollarFormat(string number, bool dollarBool = true){
	string dollarSign = "";
	if (dollarBool)
		dollarSign = "$ ";
	string neg = "";
	if (number[0] == '-'){
		neg = "-";
		number.erase(0, 1);
	}
	if (number[0] == '('){
		neg = "-";
		number.erase(0, 1);
		number.erase(number.size() - 1, number.size());
	}
	if (number == "")
		number = "0.00";
	int insertPosition = number.length() - 6;
	while (insertPosition > 0) {
		number.insert(insertPosition, ",");
		insertPosition -= 3;
	}
	string tempStr;
	if (neg == "-" || neg == "(")
		tempStr = dollarSign + "(";
	else
		tempStr = dollarSign;
	number.insert(0,tempStr);
	if (neg == "-" || neg == "(")
		number.insert(number.size(), ")");
	return number;

}


string totalDollars(string exchangeExt, string coreExt){
	/*Adds 2 decimal strings.  Should be safe, no garrantees though.*/
	vector<string> exchange;
	vector<string> coreV;
	vector<int> total;
	stringstream exch(exchangeExt);
	stringstream core(coreExt);
	string token;
	char delmiter = '.';
	while (getline(exch, token, delmiter)) {
		exchange.push_back(token);
	}
	if (exchange.size()<2)
		exchange.push_back("00");
	while (getline(core, token, delmiter)) {
		coreV.push_back(token);
	}
	if (coreV.size()<2)
		coreV.push_back("00");
	string coreneg = "";
	string exchneg = "";
	if (coreV[0][0] == '-' || coreV[0][0] == '('){
		coreneg = "-";
		coreV[0].erase(0, 1);
	}
	if (exchange[0][0] == '-' || exchange[0][0] == '('){
		exchneg = "-";
		exchange[0].erase(0, 1);
	}
	total.push_back(stoi(exchange[0]) + stoi(coreV[0]));
	total.push_back(stoi(exchange[1]) + stoi(coreV[1]));
	if (total[1] >= 100){
		total[1] = total[1] - 100;
		total[0] = total[0] + 1;
	}
	
	string strTotal = to_string(total[0]) + ".";
	if (total[1] < 10){
		strTotal = strTotal + "0" + to_string(total[1]);
	}
	else{
		strTotal = strTotal + to_string(total[1]);
	}
	return exchneg + strTotal;
}




HPDF_Page newPage(HPDF_Doc pdf, record* invoice, int pageCount){
	/*Creates a new page in the pdf doc and creates the header*/
	const char* image_path = "\\\\psserver1\\CNHR_Depts\\Accounting\\Shipment Reports\\Processing\\CNHI Reman Logo Compressed.jpg";
	HPDF_Image logo = HPDF_LoadJpegImageFromFile(pdf, image_path);

	HPDF_Page page = HPDF_AddPage(pdf);
	HPDF_REAL height = HPDF_Page_GetHeight(page);
	HPDF_REAL heightMargin = height*.05;
	HPDF_REAL width = HPDF_Page_GetWidth(page);
	HPDF_REAL widthMargin = width*.05;
	HPDF_Page_DrawImage(page, logo, width*.05, height*.95 - 56, 180, 63);
	vector<HPDF_REAL> pos = { 0, 0 };


	HPDF_Page_BeginText(page);
	HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica-Bold", NULL), 16);

	pos = setText(page, width / 2 - HPDF_Page_TextWidth(page, "CNH Reman, LLC") / 2, height*.95 - 16, pos);



	HPDF_Page_ShowText(page, "CNH Reman, LLC");
	pos = setText(page, 0, -16, pos);

	HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
	HPDF_Page_ShowText(page, "2707 N FR 123");
	pos = setText(page, 0, -12, pos);

	HPDF_Page_ShowText(page, "Springfield MO, 65803");

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
	pos = setText(page, width*.95 - HPDF_Page_TextWidth(page, date), -12, pos);
	HPDF_Page_ShowText(page, date);
	HPDF_Page_EndText(page);

	HPDF_Page_BeginText(page);
	HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 8);
	pos = { 0, 0 };
	tempStr = "Page # " + to_string(pageCount);
	pos = setText(page, width / 2 - HPDF_Page_TextWidth(page, tempStr.c_str()) / 2,  heightMargin, pos);
	HPDF_Page_ShowText(page, tempStr.c_str());

	tempStr = "Late Payments Will Incur a Fee of 1.5% Per Month";
	pos = setText(page, width / 2 - pos[0] - HPDF_Page_TextWidth(page, tempStr.c_str())/2,  heightMargin, pos);
	HPDF_Page_ShowText(page, tempStr.c_str());
	HPDF_Page_EndText(page);

	HPDF_Page_MoveTo(page, widthMargin, height - heightMargin - 61);
	HPDF_Page_LineTo(page, width - widthMargin, height - heightMargin - 61);
	HPDF_Page_Stroke(page);
	return page;

}

int drawLine(HPDF_Page page, HPDF_REAL x, HPDF_REAL y, HPDF_REAL xend, HPDF_REAL yend){
	HPDF_Page_MoveTo(page, x, -y);
	HPDF_Page_LineTo(page, xend, -yend);
	HPDF_Page_Stroke(page);
	return 0;

}


vector<HPDF_REAL> addiPageHeaders(HPDF_Doc pdf, HPDF_Page page, int pageCount){
	HPDF_REAL headerHeight;
	if (pageCount > 1)
		headerHeight = 100;
	else
		headerHeight = 234;
	HPDF_REAL height = HPDF_Page_GetHeight(page);
	HPDF_REAL heightMargin = height*.05;
	HPDF_REAL width = HPDF_Page_GetWidth(page);
	HPDF_REAL widthMargin = width*.05;
	vector<HPDF_REAL> pos = { 0, 0 };
	HPDF_Page_BeginText(page);
	HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 10);
	HPDF_REAL widthUnit = width*.95 / 100;
	pos = setText(page, widthMargin + widthUnit, height - heightMargin - headerHeight, pos);
	HPDF_Page_ShowText(page, "PSO");
	pos = setText(page, widthUnit * 9, 10, pos);
	HPDF_Page_ShowText(page, "Shipped");
	pos = setText(page, 0, -10, pos);
	HPDF_Page_ShowText(page, "Qty");
	pos = setText(page, widthUnit * 9, 0, pos);
	HPDF_Page_ShowText(page, "Part Number/Description");
	pos = setText(page, widthUnit * 23, 10, pos);
	HPDF_Page_ShowText(page, "Exchange");
	pos = setText(page, 0, -10, pos);
	HPDF_Page_ShowText(page, "Dollars");
	pos = setText(page, widthUnit * 10, 10, pos);
	HPDF_Page_ShowText(page, "Core");
	pos = setText(page, 0, -10, pos);
	HPDF_Page_ShowText(page, "Dollars");
	pos = setText(page, widthUnit * 10, 10, pos);
	HPDF_Page_ShowText(page, "Exchange");
	pos = setText(page, 0, -10, pos);
	HPDF_Page_ShowText(page, "Extended");
	pos = setText(page, widthUnit * 12, 10, pos);
	HPDF_Page_ShowText(page, "Core");
	pos = setText(page, 0, -10, pos);
	HPDF_Page_ShowText(page, "Extended");
	pos = setText(page, widthUnit * 12, 10, pos);
	HPDF_Page_ShowText(page, "Total");
	pos = setText(page, 0, -10, pos);
	HPDF_Page_ShowText(page, "Dollars");
	HPDF_Page_EndText(page);

	HPDF_Page_MoveTo(page, widthMargin, height - heightMargin - headerHeight -1);
	HPDF_Page_LineTo(page, width - widthMargin, height - heightMargin - headerHeight - 1);
	HPDF_Page_Stroke(page);
	vector<int> widthList = { 0, 9, 9, 24, 10, 10, 12, 11 };
	HPDF_REAL widthSum = 0;
	BOOST_FOREACH(int width, widthList){
		HPDF_Page_MoveTo(page, widthMargin + width * widthUnit*.9 + widthSum, height - heightMargin - headerHeight - 1);
		HPDF_Page_LineTo(page, widthMargin + width * widthUnit*.9 + widthSum, height - heightMargin - headerHeight - 1+25);
		HPDF_Page_Stroke(page);
		widthSum = widthSum + width * widthUnit;
	}
	HPDF_Page_MoveTo(page, width - widthMargin, height - heightMargin - headerHeight - 1);
	HPDF_Page_LineTo(page, width - widthMargin, height - heightMargin - headerHeight - 1 + 25);
	HPDF_Page_Stroke(page);
	return pos;

}




int create_PDF(record* invoice, vector<vector<string>> customers){
	/*Creates a PDF from a invoice record object and safes it.*/
	
	
	
	HPDF_Doc pdf = HPDF_New(error_handler, NULL);
	if (!pdf) {
		printf("error: cannot create PdfDoc object\n");
		return 1;
	}
	try{
		int pageCount = 0;
		pageCount += 1;
		HPDF_Page page = newPage(pdf, invoice, pageCount);
		HPDF_REAL height;
		height = HPDF_Page_GetHeight(page);
		HPDF_REAL heightMargin = height*.05;
		HPDF_REAL width;
		width = HPDF_Page_GetWidth(page);
		HPDF_REAL widthMargin = width*.05;
		vector<HPDF_REAL> pos;

		
		//Sold To and 
		string tempStr;
		HPDF_Page_BeginText(page);
		pos = { 0, 0 };
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 10);
		pos = setText(page, widthMargin, height - heightMargin - 100, pos);//
		HPDF_Page_ShowText(page, "Sold to");
		pos = setText(page, 10, -15, pos);
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		
		tempStr = invoice->get_header("Sold-Name") + " (" + invoice->get_header("SoldTO") + ")";
		while (HPDF_Page_TextWidth(page, tempStr.c_str())>= width/2-widthMargin){
			tempStr = invoice->get_header("Sold-Name");
			invoice->setHeader("Sold-Name", tempStr.erase(tempStr.length() - 1, tempStr.length()));
			tempStr = invoice->get_header("Sold-Name") + " (" + invoice->get_header("SoldTO") + ")";
		}
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -12, pos);
		HPDF_Page_ShowText(page, invoice->get_header("Sold-Ad1").c_str());
		pos = setText(page, 0, -12, pos);
		tempStr = invoice->get_header("Sold-City") + ", " + invoice->get_header("Sold-ST") + " " + invoice->get_header("Sold-Zip");
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -24, pos);
		tempStr = "PO Number: " + invoice->get_header("PO");
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -12, pos);
		tempStr = "Terms: " + invoice->get_header("Terms");
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -12, pos);
		tempStr = "Due Date: " + invoice->dueDate();
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -12, pos);
		tempStr = "Order Type: " + invoice->get_header("OT");
		HPDF_Page_ShowText(page, tempStr.c_str());
		
		pos = setText(page, width*.475-10, 102, pos);
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 10);
		HPDF_Page_ShowText(page, "Ship to");
		pos = setText(page, 10, -15, pos);
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);
		tempStr = invoice->get_header("Ship-Name") + " (" + invoice->get_header("ShipTO") + ")";
		while (HPDF_Page_TextWidth(page, tempStr.c_str()) >= width / 2 - widthMargin){
			tempStr = invoice->get_header("Ship-Name");
			invoice->setHeader("Ship-Name", tempStr.erase(tempStr.length() - 1, tempStr.length()));
			tempStr = invoice->get_header("Ship-Name") + " (" + invoice->get_header("ShipTO") + ")";
		}
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -12, pos);
		HPDF_Page_ShowText(page, invoice->get_header("Ship-Ad1").c_str());
		pos = setText(page, 0, -12, pos);
		tempStr = invoice->get_header("Ship-City") + ", " + invoice->get_header("Ship-ST")+ " " + invoice->get_header("Ship-Zip");
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -24, pos);
		tempStr = "Ship Date: " + invoice->get_header("Date");
		HPDF_Page_ShowText(page, tempStr.c_str());
		pos = setText(page, 0, -12, pos);
		tempStr = "Shipped Via: " + invoice->get_header("Ship Via");
		if (invoice->get_header("Ship Via") != ""){
			HPDF_Page_ShowText(page, tempStr.c_str());
			pos = setText(page, 0, -12, pos);
		}
		tempStr = "BOL: " + invoice->get_header("BOL");
		if (invoice->get_header("BOL") != ""){
			HPDF_Page_ShowText(page, tempStr.c_str());
		}
		HPDF_Page_EndText(page);
		HPDF_REAL widthUnit = width*.95 / 100;

		pos = addiPageHeaders(pdf, page, pageCount);
		vector<vector<string>> lines = invoice->get_transactions();

		
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 10);
		
		for (int i = 0; i < lines.size(); i++){
			if (pos[1] > -120- heightMargin){
				HPDF_Page_BeginText(page);
				pos = setText(page, widthMargin, pos[1] -20, pos);
				HPDF_Page_ShowText(page, "Continued on the next page...");
				HPDF_Page_EndText(page);
				pageCount += 1;
				page = newPage(pdf, invoice, pageCount);
				pos = addiPageHeaders(pdf, page, pageCount);
				
			}
			if (i % 2 == 0){
				HPDF_Page_SetRGBFill(page, .85, .85, .85);
				HPDF_Page_Rectangle(page, widthMargin, -pos[1] - 39, width - widthMargin*2, 26);
				HPDF_Page_Fill(page);
				HPDF_Page_SetRGBFill(page, 0.0, 0.0, 0.0);
			}
			HPDF_Page_BeginText(page);
			vector<HPDF_REAL> oldPos = pos;
			pos = { 0, 0 };
			pos = setText(page, widthMargin + widthUnit, -oldPos[1] - 26, pos);

			HPDF_Page_ShowText(page, lines[i][0].c_str());
			pos = setText(page, widthUnit * 9, 0, pos);
			HPDF_Page_ShowText(page, lines[i][1].c_str());
			pos = setText(page, widthUnit * 9, 0, pos);
			HPDF_Page_ShowText(page, (lines[i][2]).erase(0,1).c_str());
			
			pos = setText(page, 0, -10, pos);
			HPDF_Page_ShowText(page, lines[i][3].c_str());
			pos = setText(page, widthUnit * 21, 10, pos);
			HPDF_Page_ShowText(page, dollarFormat(lines[i][4]).c_str());
			pos = setText(page, widthUnit * 11, 0, pos);
			HPDF_Page_ShowText(page, dollarFormat(lines[i][5]).c_str());
			pos = setText(page, widthUnit * 10, 0, pos);
			HPDF_Page_ShowText(page, dollarFormat(lines[i][6]).c_str());
			pos = setText(page, widthUnit * 11, 0, pos);
			HPDF_Page_ShowText(page, dollarFormat(lines[i][7]).c_str());
			pos = setText(page, widthUnit * 11, 0, pos);
			HPDF_Page_ShowText(page, dollarFormat(totalDollars(lines[i][6], lines[i][7])).c_str());
			HPDF_Page_EndText(page);
			

		}

		drawLine(page, widthMargin, pos[1] + 18, width - widthMargin, pos[1] + 18);
		
		HPDF_Page_BeginText(page);
		vector<HPDF_REAL> oldPos = pos;
		pos = { 0, 0 };
		HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 10);
		pos = setText(page, widthMargin * 2, -oldPos[1] - 36, pos);
		vector<string> messages = { invoice->get_header("Message1"), invoice->get_header("Message2"), invoice->get_header("Message3") };

		for (int i = 0; i < messages.size(); i++){
			if (messages[i] != ""){
				HPDF_Page_ShowText(page, messages[i].c_str());
				pos = setText(page, 0, -10, pos);
			}

		}
		tempStr = "Invoice Total: " + dollarFormat(invoice->invoice_total());
		HPDF_REAL txtAlign = width - widthMargin - HPDF_Page_TextWidth(page, tempStr.c_str());

		pos = setText(page, txtAlign- pos[0], - 20, pos);
		HPDF_Page_ShowText(page, "Product Total: ");
		pos = setText(page, width - pos[0] - widthMargin - HPDF_Page_TextWidth(page, dollarFormat(invoice->productTotal(), false).c_str()), 0, pos);
		HPDF_Page_ShowText(page, dollarFormat(invoice->productTotal(),false).c_str() );

		pos = setText(page, txtAlign - pos[0], -10, pos);
		HPDF_Page_ShowText(page, "Freight Total: ");
		pos = setText(page, width - pos[0] - widthMargin - HPDF_Page_TextWidth(page, invoice->freight().c_str()), 0, pos);
		HPDF_Page_ShowText(page, invoice->freight().c_str());

		pos = setText(page, txtAlign - pos[0], -10, pos);;
		HPDF_Page_ShowText(page, "Invoice Total: ");
		pos = setText(page, width - pos[0] - widthMargin - HPDF_Page_TextWidth(page, dollarFormat(invoice->invoice_total()).c_str()), 0, pos);
		HPDF_Page_ShowText(page, dollarFormat(invoice->invoice_total()).c_str());
		HPDF_Page_EndText(page);
		




		//Uncomment for debuging in the current directory
		//HPDF_SaveToFile(pdf, (invoice->name() + ".pdf").c_str());

		string path1 = "\\\\psserver1\\CNHR_Depts\\Accounting\\Shipment Reports\\Invoices to Send\\";
		string ap;
		if (MOVE_TO_AP_TYPES.count(invoice->get_header("OT")) != 0 || MOVE_TO_AP_CUST.count(invoice->get_header("SoldTO")) != 0){
			ap = " AP";

		}
		string filename = path1 + invoice->name() + ap + ".pdf";
		HPDF_SaveToFile(pdf, (filename).c_str());


		//Commands for saving to a batch folder, increases processing time, because user has to wait to download until after batch is comple
		//string path2 = "\\\\psserver1\\CNHR_Depts\\Accounting\\Shipment Reports\\20" + invoice->get_header("Date").substr(6) + "\\" + MONTHS[stoi(invoice->get_header("Date").substr(0,2))-1] \
			//+ "\\Batch Detail\\" + invoice->get_header("Batch") + "\\";
		//HPDF_SaveToFile(pdf, (path2 + invoice->name() + ".pdf").c_str());


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
	vector<vector<string>> invoices = create_table("C:\\Download\\invoice.csv", '\t');
	vector<vector<string>> customers = { { "" } };
	
	string previous;
	record* invoice = NULL;
	vector<string> temp;
	HPDF_Doc Dummy = HPDF_New(error_handler, NULL);
	HPDF_Page page = HPDF_AddPage(Dummy);
	HPDF_Page_SetFontAndSize(page, HPDF_GetFont(Dummy, "Helvetica-Bold", NULL), 10);
	string credit = "";


	for (int i = 0; i < invoices.size(); i++){
		string* current = new string( invoices[i][0]);
		if (*current == previous){//If this line is associated with the current invoice add to it
			string qty = invoices[i][10];
			string part = invoices[i][4];
			if (invoices[i][9] == "C"){
				credit = "-";
			}
			else{
				credit = "";
			}

			if (part != "@MSG"  && qty != "0")
			{

				//		 PSO			 QTY            Part    DESCRIPTION       EXCHANGE			CORE		ExExt			CoreExt			Freight
				temp = { invoices[i][40], credit + qty, part, invoices[i][5], credit + invoices[i][11], credit + invoices[i][13], credit + invoices[i][12], credit + invoices[i][14], invoices[i][18] };
				if (temp[3] == ""){
					temp[3] = invoices[i][47];
				}
				for (int i = 0; i < temp.size(); i++){
					if (temp[i] == "" && i >3){
						temp[i] = "0.00";
					}
				}
				while (HPDF_Page_TextWidth(page, (temp[2]).c_str())>115){
					temp[2].erase(temp[2].length() - 1, temp[2].length());
				}
				while(HPDF_Page_TextWidth(page, (temp[3]).c_str())>115){
					temp[3].erase(temp[3].length()-1 , temp[3].length());
				}
				invoice->add_line(temp);
			}
		

		}
		else{//save the invoice and delete the record to release the memory
			if (invoice != NULL){
				if (invoice->invoice_total() != "0.00"){
					create_PDF(invoice, customers);
					//invoice->print();
				}
				delete invoice;
			}
			//create new record
			invoice = new record(invoices[i], customers);
		}
		previous = *current;
		delete current;
	}

	if (invoice != NULL){
		if (invoice->invoice_total() != "0.00"){
			create_PDF(invoice, customers);
			//invoice->print();
		}
		delete invoice;
	}



	return 0;

}