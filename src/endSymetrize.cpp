/*
 * endSymetrize.cpp
 *
 *  Created on: 19-02-2011
 *      Author: root
 */

#include <cassert>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>
#include "cmd.h"

using namespace std;

string FindAndReplace( std::string& tInput, std::string tFind, std::string tReplace ) {
	size_t uPos = 0;
	size_t uFindLen = tFind.length();
	size_t uReplaceLen = tReplace.length();

	if( uFindLen == 0 )
	{
		return tInput;
	}

	for( ;(uPos = tInput.find( tFind, uPos )) != std::string::npos; )
	{
		tInput.replace( uPos, uFindLen, tReplace );
		uPos += uReplaceLen;
	}
	return tInput;

}


int create_file(const char* input, const char* inputInv, const char* output){



	   cout << "Running composing for files "<< input << ", " << inputInv<<endl;
	   cout << "Save in " << output << endl;

fstream inp(input,ios::in);
fstream inpInv(inputInv,ios::in);
fstream out(output,ios::out);

if (!inp.is_open()){
	cerr << "cannot open " << input << "\n";
	exit(1);
}

if (!inpInv.is_open()){
		cerr << "cannot open " << inputInv << "\n";
		exit(1);
}

if (!out.is_open()){
	cerr << "cannot open " << output << "\n";
	exit(1);
}

string s1;
string s2;
vector<int> a = vector<int>();
vector<int> b = vector<int>();

vector< vector<int> > inta;
vector< vector<int> > intb;
vector<int> part = vector<int>();
//int a[100];
//int b[100];
//int a_size=0;
//int b_size=0;

while (inp) {
getline(inp,s1);
getline(inpInv,s2);

if(s1.find("NULL")!= string::npos){
const char *str1 = s1.c_str();
//a_size=0;
a.clear();
inta.clear();
str1=str1+9;

while (*str1 != '\0') {
	if(*++str1=='(' && *++str1=='{' && *++str1==' '){
		if(*++str1=='}' && *++str1==')') {
			//cout << "0" << " ";
			a.push_back(0);
			part.push_back(0);
			inta.push_back(part);
			part.clear();
			//a[a_size] = 0;
			//a_size++;
		}
		else {
			--str1;
			while(*++str1 != '}'){
				string integer="";
				--str1;
					while(*++str1!=' ') {
						integer.push_back(*str1);
					}
					part.push_back(atoi(integer.c_str()));
			}
			inta.push_back(part);
			part.clear();
			*str1--;
			while(*--str1!=' ') {}
			string integer="";
			while(*++str1!=' ') {
				//cout << *str1;
				integer.push_back(*str1);
			}
			//cout << " "<< integer;
			a.push_back(atoi(integer.c_str()));
			//a[a_size] = atoi(integer.c_str());
			//a_size++;

		}

	}
}


const char *str2 = s2.c_str();
		//b_size=0;
		b.clear();
		intb.clear();
		str2=str2+9;
		while (*str2 != '\0') {
			//cout << *str2++ << endl;
			if(*++str2=='(' && *++str2=='{' && *++str2==' '){
				if(*++str2=='}' && *++str2==')') {
					//cout << "0" << " ";
					b.push_back(0);
					part.push_back(0);
					intb.push_back(part);
					part.clear();
					//b[b_size] = 0;
					//b_size++;
				}
				else {
					--str2;
					while(*++str2 != '}'){
						string integer="";
						--str2;
						while(*++str2!=' ') {
							integer.push_back(*str2);
						}
						part.push_back(atoi(integer.c_str()));
					}
					intb.push_back(part);
					part.clear();
					*str2--;
					while(*--str2!=' ') {}
					string integer="";
					while(*++str2!=' ') {
						//cout << *str2;
						integer.push_back(*str2);
					}
					//cout << " "<< integer;
					b.push_back(atoi(integer.c_str()));
					//b[b_size] = atoi(integer.c_str());
					//b_size++;

				}

			}
		}

		/*for(int i=0;i<intb.size();i++){
			for(int j=0;j<intb[i].size();j++){
				cout << intb[i][j] << ", ";
			}
			cout << endl;
		}
		cout << endl << endl;*/

		//check for the same connection in each direction
		for(int i1=0; i1<inta.size();i1++)
			for(int j1=0;j1<inta[i1].size();j1++)
				if(inta[i1][j1] != 0)

					for(int i2=0; i2<intb.size();i2++)
								for(int j2=0;j2<intb[i2].size();j2++)
									if(intb[i2][j2] != 0)
										if(inta[i1][j1] == i2+1 && intb[i2][j2] == i1+1){
											b[i2] = i1+1;
											a[i1] = i2+1;
										}


		//instert into 0
		for(int k=0; k<a.size();k++)
			if(a[k] == 0)
				for(int i2=0; i2<intb.size();i2++)
					for(int j2=0;j2<intb[i2].size();j2++)
						if(intb[i2][j2] == k+1 ) a[k] = i2+1;

		for(int k=0; k<b.size();k++)
					if(b[k] == 0)
						for(int i1=0; i1<inta.size();i1++)
							for(int j1=0;j1<inta[i1].size();j1++)
								if(inta[i1][j1] == k+1) b[k] = i1+1;


		//remove from string
		s1 = FindAndReplace(s1, "NULL", "");
		size_t begin=0;
		size_t end=0;
		for( ;(begin = s1.find( "({ ", begin )) != std::string::npos; )
			{
				end = s1.find(" })")+4;
				s1.replace( begin, end-begin, "" );
			}

		s2 = FindAndReplace(s2, "NULL", "");
		begin=0;
		end=0;
		for( ;(begin = s2.find( "({ ", begin )) != std::string::npos; )
			{
				end = s2.find(" })")+4;
				s2.replace( begin, end-begin, "" );
			}

		int asp=0, bsp=0;
		begin=0;
		end=0;
		for( ;(begin = s1.find( " ", end )) != std::string::npos; )
		{
			end = begin+2;
			asp++;
		}
		begin=0;
		end=0;
		for( ;(begin = s2.find( " ", end )) != std::string::npos; )
		{
			end = begin+2;
			bsp++;
		}

		if((a.size()+1) == asp && (b.size()+1) == bsp ){

		out << "1" << endl;

		out << b.size() << s2 << " #";
		for(int i=0; i<b.size();i++) out	<< " " << b[i];
		out << endl;
		out << a.size() << s1 << " #";
		for(int i=0; i<a.size();i++) out	<< " " << a[i];
		out << endl;
		} else{
			cout << "Error in sentence "<< endl;
		}
}
}
inp.close();
inpInv.close();
out.close();
}

/*int main(int argc, char** argv){



	char* input="";
	char* inputInv="";	//input inverse
	char* output="/dev/stdout";

	DeclareParams("i", CMDSTRINGTYPE, &input,
					"input", CMDSTRINGTYPE, &input,
					"iinv", CMDSTRINGTYPE, &inputInv,
					"inputinverse", CMDSTRINGTYPE, &inputInv,
	                "o", CMDSTRINGTYPE, &output,
	                "output", CMDSTRINGTYPE, &output,
	                 (char *)NULL);

	GetParams(&argc, &argv, (char*) NULL);


			   cout << "Running composing for files "<< input << ", " << inputInv<<endl;
			   cout << "Save in " << output << endl;

	fstream inp(input,ios::in);
	fstream inpInv(inputInv,ios::in);
	fstream out(output,ios::out);

	if (!inp.is_open()){
			cerr << "cannot open " << input << "\n";
			exit(1);
	}

	if (!inpInv.is_open()){
				cerr << "cannot open " << inputInv << "\n";
				exit(1);
		}

	if (!out.is_open()){
			cerr << "cannot open " << output << "\n";
			exit(1);
	}

	string s1;
	string s2;
	int a[100];
	int b[100];
	int a_size=0;
	int b_size=0;

	while (inp) {
	getline(inp,s1);
	getline(inpInv,s2);

	if(s1.find("NULL")!= string::npos){
		const char *str1 = s1.c_str();
		a_size=0;
		str1=str1+9;

		while (*str1 != '\0') {
			//cout << *str1++ << endl;
			if(*++str1=='(' && *++str1=='{' && *++str1==' '){
				if(*++str1=='}' && *++str1==')') {
					//cout << "0" << " ";
					a[a_size] = 0;
					a_size++;
				}
				else {
					while(*++str1 != '}'){}
					*str1--;
					while(*--str1!=' ') {}
					string integer="";
					while(*++str1!=' ') {
						//cout << *str1;
						integer.push_back(*str1);
					}
					//cout << " "<< integer;
					a[a_size] = atoi(integer.c_str());
					a_size++;

				}

			}
		}

		const char *str2 = s2.c_str();
				b_size=0;
				str2=str2+9;
				while (*str2 != '\0') {
					//cout << *str2++ << endl;
					if(*++str2=='(' && *++str2=='{' && *++str2==' '){
						if(*++str2=='}' && *++str2==')') {
							//cout << "0" << " ";
							b[b_size] = 0;
							b_size++;
						}
						else {
							while(*++str2 != '}'){}
							*str2--;
							while(*--str2!=' ') {}
							string integer="";
							while(*++str2!=' ') {
								//cout << *str2;
								integer.push_back(*str2);
							}
							//cout << " "<< integer;
							b[b_size] = atoi(integer.c_str());
							b_size++;

						}

					}
				}

				//remove from string
				s1 = FindAndReplace(s1, "NULL", "");
				size_t begin=0;
				size_t end=0;
				for( ;(begin = s1.find( "({ ", begin )) != std::string::npos; )
					{
						end = s1.find(" })")+4;
						s1.replace( begin, end-begin, "" );
					}

				s2 = FindAndReplace(s2, "NULL", "");
				begin=0;
				end=0;
				for( ;(begin = s2.find( "({ ", begin )) != std::string::npos; )
					{
						end = s2.find(" })")+4;
						s2.replace( begin, end-begin, "" );
					}

				out << "1" << endl;

				out << b_size << s2 << " #";
				for(int i=0; i<b_size;i++) out	<< " " << b[i];
				out << endl;
				out << a_size << s1 << " #";
				for(int i=0; i<a_size;i++) out	<< " " << a[i];
				out << endl;
	}
	}
	inp.close();
	inpInv.close();
	out.close();

}*/

