/*
 * mergeAlignment.cpp
 *
 *  Created on: 19-02-2011
 *      Author: root
 */

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iomanip>
#include <list>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>

using namespace std;

int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
    	files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

bool strContains(const string inputStr, const string searchStr)
{
        size_t contains;

        contains = inputStr.find(searchStr);

        if(contains != string::npos)
                return true;
        else
                return false;
}

int merge_alig(const char* prefix)
{
	//char* prefix = "ef.A3.final";

	const char* prefixLine = "Sentence pair";
	string dir = string(prefix);

    vector<string> files = vector<string>();
    vector<fstream*> filesStream;

    size_t found = 0;
    found=dir.find_last_of("/\\");
    string folder = ".";

    if(found < 4294967295) folder = dir.substr(0,found);
    string searchPrefix = dir.substr(found+1)+".";
      cout << " Folder with files: " << folder << endl;
      cout << " Prefix of files: " << searchPrefix << endl;

    getdir(folder,files);
    int number = 0;

    fstream out(prefix,ios::out);
    if (!out.is_open()){
    	cerr << "cannot open " << prefix << "\n";
    	exit(1);
    }

    int j =0;
    string s1;
    cout << "Looking for files with prefix: "<<searchPrefix<<endl;

    for (unsigned int i = 0;i < files.size();i++) {
        if(strContains(files[i], searchPrefix)){
    		 cout << "I find file: "<< files[i] << endl;
    		 fstream* in = new fstream;
    		 filesStream.push_back(in);
    		 string toOpen = folder+"/"+files[i];
    		 filesStream[number]->open(toOpen.c_str(),ios::in);
    		if (!filesStream[number]->is_open()){
    			cerr << "cannot open " << files[i] << "\n";
    			exit(1);
    		}
    		number++;
    	}
    }
    vector<string> a = vector<string>();
    vector< vector<string> > sent;

    int added =1; //actual added line
    int all = 2; //the last number of lines

    cout << "I begin composing with "<< number << " number of files" << endl;
    	for(int j=0;j<number;j++){
    			while (*filesStream[j]) {
        				char line[8192];
        				string s = "";
        				filesStream[j]->getline(line, 8192);
        				if(line[0] != '\0') {
        					s.assign(line);
        					a.push_back(s);
        				}

        			}
    			sent.push_back(a);
    			a.clear();
        		}
int last[number];
for(int i=0;i<number;i++) last[i]=0;

while(added<all){
	for(int d=0;d<number;d++)
    for(int i=last[d]; i<sent[d].size();i++){
    	if (sent[d][i] != "" ) if(strContains(sent[d][i], prefixLine)){
    		const char *str = sent[d][i].c_str();
    		string integer="";

    		while (*++str != '(') {}
    		while(*++str!=')') {
    			integer.push_back(*str);
    		}
    		int iii = atoi(integer.c_str());
    		if(all< iii) all = iii;

    		if(iii == added) {
    			out << sent[d][i] << endl;
    			out << sent[d][i+1] << endl;
    			out << sent[d][i+2] << endl;
    			added++;
    			sent[d][i] = "";
    			sent[d][i+1] = "";
    			sent[d][i+2] = "";
    		}
    		if(iii > (added+1)) {
    			if (i>6) last[d] = i-6;
    			break;
    		}
    	}

    }
}

    return 1;
}
