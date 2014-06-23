/*

 EGYPT Toolkit for Statistical Machine Translation
 Written by Yaser Al-Onaizan, Jan Curin, Michael Jahr, Kevin Knight, John Lafferty, Dan Melamed, David Purdy, Franz Och, Noah Smith, and David Yarowsky.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, 
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 USA.

 */

#include <strstream>
#include "getSentence.h"
#include "TTables.h"
#include "model1.h"
#include "model2.h"
#include "model3.h"
#include "hmm.h"
#include "file_spec.h"
#include "defs.h"
#include "vocab.h"
#include "Perplexity.h"
#include "Dictionary.h"
#include "utility.h" 
#include "Parameter.h"
#include "myassert.h"
#include "D4Tables.h"
#include "D5Tables.h"
#include "transpair_model4.h"
#include "transpair_model5.h"
#include "endSymetrize.cpp"
#include "symalEnd.cpp"
#include "mergeAlignment.cpp"

#define ITER_M2 0
#define ITER_MH 5

#define UNION                      1
#define INTERSECT                  2
#define GROW                       3
#define SRCTOTGT                   4
#define TGTTOSRC                   5
#define BOOL_YES                   1
#define BOOL_NO                    0
/**
 Here we can see that Every model is iterated several times, and we do not need to do it 
 on all the corpora, instead we will only start a few.
 */
GLOBAL_PARAMETER3(bool,endSymmetrization,"endSymmetric","es","Symetrization at the end (Y/N)","0: no end symmetrization; 1: do end symmetrization",PARLEV_EM,0);

GLOBAL_PARAMETER3(int,Model1_Iterations,"Model1_Iterations","NO. ITERATIONS MODEL 1","m1","number of iterations for Model 1",PARLEV_ITER,5);
GLOBAL_PARAMETER3(int,Model2_Iterations,"Model2_Iterations","NO. ITERATIONS MODEL 2","m2","number of iterations for Model 2",PARLEV_ITER,ITER_M2);
GLOBAL_PARAMETER3(int,HMM_Iterations,"HMM_Iterations","mh","number of iterations for HMM alignment model","mh", PARLEV_ITER,ITER_MH);
GLOBAL_PARAMETER3(int,Model3_Iterations,"Model3_Iterations","NO. ITERATIONS MODEL 3","m3","number of iterations for Model 3",PARLEV_ITER,5);
GLOBAL_PARAMETER3(int,Model4_Iterations,"Model4_Iterations","NO. ITERATIONS MODEL 4","m4","number of iterations for Model 4",PARLEV_ITER,5);
GLOBAL_PARAMETER3(int,Model5_Iterations,"Model5_Iterations","NO. ITERATIONS MODEL 5","m5","number of iterations for Model 5",PARLEV_ITER,0);
GLOBAL_PARAMETER3(int,Model6_Iterations,"Model6_Iterations","NO. ITERATIONS MODEL 6","m6","number of iterations for Model 6",PARLEV_ITER,0);

GLOBAL_PARAMETER3(int,T_Multiplier,"T_Multiplier","Multipler of t values","tm","multipler of t values",PARLEV_ITER,1);

//GLOBAL_PARAMETER(bool,RunSymmetrization,"runSym","0: no; 1: yes",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER3(int,Model1_SymFrequency,"Model1_Symetrization_Frequence","Symetrization frequency of model 1","m1symfrequency","Symetrization frequency of model 1",PARLEV_ITER,1);
GLOBAL_PARAMETER3(int,Model2_SymFrequency,"Model2_Symetrization_Frequence","Symetrization frequency of model 2","m2symfrequency","Symetrization frequency of model 2",PARLEV_ITER,1);
GLOBAL_PARAMETER3(int,Model345_SymFrequency,"Model345_Symetrization_Frequence","Symetrization frequency of model 3","m345symfrequency","Symetrization frequency of model 3, 4, 5",PARLEV_ITER,1);
GLOBAL_PARAMETER3(int,HMM_SymFrequency,"HMM_Symetrization_Frequence","Symetrization frequency of HMM alignment model","mhsymfrequency","Symetrization frequency of HMM alignment model",PARLEV_ITER,1);

GLOBAL_PARAMETER(float, PROB_SMOOTH,"probSmooth","probability smoothing (floor) value ",PARLEV_OPTHEUR,1e-7);
GLOBAL_PARAMETER(float, MINCOUNTINCREASE,"minCountIncrease","minimal count increase",PARLEV_OPTHEUR,1e-7);

GLOBAL_PARAMETER2(int,Transfer_Dump_Freq,"TRANSFER DUMP FREQUENCY","t2to3","output: dump of transfer from Model 2 to 3",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER2(bool,Verbose,"verbose","v","0: not verbose; 1: verbose",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER(bool,Log,"log","0: no logfile; 1: logfile",PARLEV_OUTPUT,0);

GLOBAL_PARAMETER(double,P0,"p0","fixed value for parameter p_0 in IBM-3/4 (if negative then it is determined in training)",PARLEV_EM,-1.0);
GLOBAL_PARAMETER(double,M5P0,"m5p0","fixed value for parameter p_0 in IBM-5 (if negative then it is determined in training)",PARLEV_EM,-1.0);
GLOBAL_PARAMETER3(bool,Peg,"pegging","p","DO PEGGING? (Y/N)","0: no pegging; 1: do pegging",PARLEV_EM,0);

GLOBAL_PARAMETER(short,OldADBACKOFF,"adbackoff","",-1,0);
GLOBAL_PARAMETER2(unsigned int,MAX_SENTENCE_LENGTH,"ml","MAX SENTENCE LENGTH","maximum sentence length",0,MAX_SENTENCE_LENGTH_ALLOWED);

GLOBAL_PARAMETER(short, DeficientDistortionForEmptyWord,"DeficientDistortionForEmptyWord","0: IBM-3/IBM-4 as described in (Brown et al. 1993); 1: distortion model of empty word is deficient; 2: distoriton model of empty word is deficient (differently); setting this parameter also helps to avoid that during IBM-3 and IBM-4 training too many words are aligned with the empty word",PARLEV_MODELS,0);

/**
Here are parameters to support Load models and dump models
*/

GLOBAL_PARAMETER(int,restart,"restart","Restart training from a level,0: Normal restart, from model 1, 1: Model 1, 2: Model 2 Init (Using Model 1 model input and train model 2), 3: Model 2, (using model 2 input and train model 2), 4 : HMM Init (Using Model 1 model and train HMM), 5: HMM (Using Model 2 model and train HMM) 6 : HMM (Using HMM Model and train HMM), 7: Model 3 Init (Use HMM model and train model 3) 8: Model 3 Init (Use Model 2 model and train model 3) 9: Model 3, 10: Model 4 Init (Use Model 3 model and train Model 4) 11: Model 4 and on, ",PARLEV_INPUT,0);
GLOBAL_PARAMETER(bool,dumpCount,"dumpcount","Whether we are going to dump count (in addition to) final output?",PARLEV_OUTPUT,false);
GLOBAL_PARAMETER(bool,dumpCountUsingWordString,"dumpcountusingwordstring","In count table, should actual word appears or just the id? default is id",PARLEV_OUTPUT,false);
/// END
short OutputInAachenFormat=0;
bool Transfer=TRANSFER;
bool Transfer2to3=0;
short NoEmptyWord=0;
bool FEWDUMPS=0;
GLOBAL_PARAMETER(bool,ONLYALDUMPS,"ONLYALDUMPS","1: do not write any files",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER(short,NCPUS,"NCPUS","Number of CPUS",PARLEV_EM,2);
GLOBAL_PARAMETER(short,CompactAlignmentFormat,"CompactAlignmentFormat","0: detailled alignment format, 1: compact alignment format ",PARLEV_OUTPUT,0);
GLOBAL_PARAMETER2(bool,NODUMPS,"NODUMPS","NO FILE DUMPS? (Y/N)","1: do not write any files",PARLEV_OUTPUT,0);

GLOBAL_PARAMETER(WordIndex, MAX_FERTILITY, "MAX_FERTILITY",
		"maximal fertility for fertility models", PARLEV_EM, 10);

Vector<map< pair<int,int>,char > > ReferenceAlignment;

bool useDict = false;
string CoocurrenceFileEF;
string CoocurrenceFileFE;
string Prefix, PrefixEF, PrefixFE, Alignment, Diagonal, Both, Final, LogFilename, OPath, Usage, SourceVocabFilename,
		TargetVocabFilename, CorpusFilenameEF, CorpusFilenameFE, TestCorpusFilename, t_Filename,
		a_Filename, p0_Filename, d_Filename, n_Filename, dictionary_Filename;

// QIN: Variables required for reloading model and continue training

string prev_t, prev_p0, prev_a, prev_d, prev_d4,prev_d4_2, prev_hmm,prev_n;

// QIN: And below are for count outputAlignment
string countPrefix;

Mutex logmsg_lock;
ofstream logmsg;
const string str2Num(int n) {
	string number = "";
	do {
		number.insert((size_t)0, 1, (char)(n % 10 + '0'));
	} while ((n /= 10) > 0);
	return (number);
}

double LAMBDA=1.09;
sentenceHandler *testCorpus=0, *corpusEF=0, *corpusFE=0;
Perplexity trainPerpEF, trainPerpFE, testPerp, trainViterbiPerpEF, trainViterbiPerpFE, testViterbiPerp;

string ReadTablePrefix;


//_____thread for simmetrization______________
struct thread_model{
    model1 *m1;
    model2 *m2;
    model3 *m3;
    hmm *mh;
    int it;
    int itM4;
    int itM5;
    int itM6;
    bool seed;
    Dictionary *dict;
    bool useDict;
    pthread_t thread;
    bool dump;
    char *countPrefix;
    int valid;
    int result;
    int mode;
};

void* runThreadModel1(void *arg){
    thread_model* th =(thread_model *) arg;
    if(th->mode==0){
    th->result = th->m1->em_with_tricks(th->it, th->seed,
    						*th->dict, th->useDict,true,
    					    th->countPrefix,
    					    th->dump);
    }
    else if(th->mode==1){
    th->result = th->m1->em_with_tricks(th->it, true, *th->dict, th->useDict);
    }

    return arg;
}
void* runThreadModel2(void *arg){
    thread_model* th =(thread_model *) arg;
    if(th->mode==0){
    th->result = th->m2->em_with_tricks(th->it, true, th->countPrefix, th->dump);

    }
    else if(th->mode==1){
    th->result = th->m2->em_with_tricks(th->it);
    }

    return arg;
}
void* runThreadHMM(void *arg){
    thread_model* th =(thread_model *) arg;
    if(th->mode==0){
    th->result = th->mh->em_with_tricks(th->it, true, th->countPrefix, th->dump, restart == 6);

    }
    else if(th->mode==1){
    th->result = th->mh->em_with_tricks(th->it,false,NULL,false,restart==6);
    }

    return arg;
}
void* runThreadModel3(void *arg){
    thread_model* th =(thread_model *) arg;
    if(th->mode==0){
    th->result = th->m3->viterbi(th->it, th->itM4, th->itM5, th->itM6, NULL, NULL, true, th->countPrefix, th->dump);

    }
    else if(th->mode==1){
    th->result = th->m3->viterbi(th->it, th->itM4, th->itM5, th->itM6, NULL, NULL);
    }

    return arg;
}

//____________________________________________
void printGIZAPars(ostream&out) {
	out << "general parameters:\n"
		"-------------------\n";
	printPars(out, getGlobalParSet(), 0);
	out << '\n';

	out << "No. of iterations:\n-"
		"------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_ITER);
	out << '\n';

	out
			<< "parameter for various heuristics in GIZA++ for efficient training:\n"
				"------------------------------------------------------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_OPTHEUR);
	out << '\n';

	out << "parameters for describing the type and amount of output:\n"
		"-----------------------------------------------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_OUTPUT);
	out << '\n';

	out << "parameters describing input files:\n"
		"----------------------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_INPUT);
	out << '\n';

	out << "smoothing parameters:\n"
		"---------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_SMOOTH);
	out << '\n';

	out << "parameters modifying the models:\n"
		"--------------------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_MODELS);
	out << '\n';

	out << "parameters modifying the EM-algorithm:\n"
		"--------------------------------------\n";
	printPars(out, getGlobalParSet(), PARLEV_EM);
	out << '\n';
}

const char*stripPath(const char*fullpath)
// strip the path info from the file name 
{
	const char *ptr = fullpath + strlen(fullpath) - 1;
	while (ptr && ptr > fullpath && *ptr != '/') {
		ptr--;
	}
	if ( *ptr=='/')
		return (ptr+1);
	else
		return ptr;
}

void printDecoderConfigFile() {
	string decoder_config_file = Prefix + ".Decoder.config";
	cerr << "writing decoder configuration file to "
			<< decoder_config_file.c_str() <<'\n';
	ofstream decoder(decoder_config_file.c_str());
	if (!decoder) {
		cerr << "\nCannot write to " << decoder_config_file <<'\n';
		exit(1);
	}
	decoder
			<< "# Template for Configuration File for the Rewrite Decoder\n# Syntax:\n"
			<< "#         <Variable> = <value>\n#         '#' is the comment character\n"
			<< "#================================================================\n"
			<< "#================================================================\n"
			<< "# LANGUAGE MODEL FILE\n# The full path and file name of the language model file:\n";
	decoder << "LanguageModelFile =\n";

	decoder
			<< "#================================================================\n"
			<< "#================================================================\n"
			<< "# TRANSLATION MODEL FILES\n# The directory where the translation model tables as created\n"
			<< "# by Giza are located:\n#\n"
			<< "# Notes: - All translation model \"source\" files are assumed to be in\n"
			<< "#          TM_RawDataDir, the binaries will be put in TM_BinDataDir\n"
			<< "#\n#        - Attention: RELATIVE PATH NAMES DO NOT WORK!!!\n"
			<< "#\n#        - Absolute paths (file name starts with /) will override\n"
			<< "#          the default directory.\n\n";
	// strip file prefix info and leave only the path name in Prefix
	string path = Prefix.substr(0, Prefix.find_last_of("/")+1);
	if (path=="")
		path=".";
	decoder << "TM_RawDataDir = " << path << '\n';
	decoder << "TM_BinDataDir = " << path << '\n' << '\n';
	decoder << "# file names of the TM tables\n# Notes:\n"
			<< "# 1. TTable and InversTTable are expected to use word IDs not\n"
			<< "#    strings (Giza produces both, whereby the *.actual.* files\n"
			<< "#    use strings and are THE WRONG CHOICE.\n"
			<< "# 2. FZeroWords, on the other hand, is a simple list of strings\n"
			<< "#    with one word per line. This file is typically edited\n"
			<< "#    manually. Hoeever, this one listed here is generated by GIZA\n\n";

	int lastmodel;
	if (Model5_Iterations>0)
		lastmodel = 5;
	else if (Model4_Iterations>0)
		lastmodel = 4;
	else if (Model3_Iterations>0)
		lastmodel = 3;
	else if (Model2_Iterations>0)
		lastmodel = 2;
	else
		lastmodel = 1;
	string lastModelName = str2Num(lastmodel);
	string p=Prefix + ".t" + /*lastModelName*/"3" +".final";
	decoder << "TTable = " << stripPath(p.c_str()) << '\n';
	p = Prefix + ".ti.final";
	decoder << "InverseTTable = " << stripPath(p.c_str()) << '\n';
	p=Prefix + ".n" + /*lastModelName*/"3" + ".final";
	decoder << "NTable = " << stripPath(p.c_str()) << '\n';
	p=Prefix + ".d" + /*lastModelName*/"3" + ".final";
	decoder << "D3Table = " << stripPath(p.c_str()) << '\n';
	p=Prefix + ".D4.final";
	decoder << "D4Table = " << stripPath(p.c_str()) << '\n';
	p=Prefix + ".p0_"+ /*lastModelName*/"3" + ".final";
	decoder << "PZero = " << stripPath(p.c_str()) << '\n';
	decoder << "Source.vcb = " << SourceVocabFilename << '\n';
	decoder << "Target.vcb = " << TargetVocabFilename << '\n';
	//  decoder << "Source.classes = " << SourceVocabFilename + ".classes" << '\n';
	//  decoder << "Target.classes = " << TargetVocabFilename + ".classes" <<'\n';
	decoder << "Source.classes = " << SourceVocabFilename+".classes" << '\n';
	decoder << "Target.classes = " << TargetVocabFilename + ".classes" <<'\n';
	p=Prefix + ".fe0_"+ /*lastModelName*/"3" + ".final";
	decoder << "FZeroWords       = " <<stripPath(p.c_str()) << '\n';

	/*  decoder << "# Translation Parameters\n"
	 << "# Note: TranslationModel and LanguageModelMode must have NUMBERS as\n"
	 << "# values, not words\n"
	 << "# CORRECT: LanguageModelMode = 2\n"
	 << "# WRONG:   LanguageModelMode = bigrams # WRONG, WRONG, WRONG!!!\n";
	 decoder << "TMWeight          = 0.6 # weight of TM for calculating alignment probability\n";
	 decoder << "TranslationModel  = "<<lastmodel<<"   # which model to use (3 or 4)\n";
	 decoder << "LanguageModelMode = 2   # (2 (bigrams) or 3 (trigrams)\n\n";
	 decoder << "# Output Options\n"
	 << "TellWhatYouAreDoing = TRUE # print diagnostic messages to stderr\n"
	 << "PrintOriginal       = TRUE # repeat original sentence in the output\n"
	 << "TopTranslations     = 3    # number of n best translations to be returned\n"
	 << "PrintProbabilities  = TRUE # give the probabilities for the translations\n\n";
	 
	 decoder << "# LOGGING OPTIONS\n"
	 << "LogFile = - # empty means: no log, dash means: STDOUT\n"
	 << "LogLM = true # log language model lookups\n"
	 << "LogTM = true # log translation model lookups\n";
	 */
}

void printAllTables(vcbList& eTrainVcbList, vcbList& eTestVcbList,
		vcbList& fTrainVcbList, vcbList& fTestVcbList, model1& m1ef) {
	cerr << "writing Final tables to Disk \n";
	string t_inv_file = Prefix + ".ti.final";
	if ( !FEWDUMPS)
		m1ef.getTTable().printProbTableInverse(t_inv_file.c_str(),
				m1ef.getEnglishVocabList(), m1ef.getFrenchVocabList(),
				m1ef.getETotalWCount(), m1ef.getFTotalWCount());
	t_inv_file = Prefix + ".actual.ti.final";
	if ( !FEWDUMPS)
		m1ef.getTTable().printProbTableInverse(t_inv_file.c_str(),
				eTrainVcbList.getVocabList(), fTrainVcbList.getVocabList(),
				m1ef.getETotalWCount(), m1ef.getFTotalWCount(), true);

	string perp_filenameEF = Prefix + ".perpEF";
	ofstream of_perpEF(perp_filenameEF.c_str());
	string perp_filenameFE = Prefix + ".perpFE";
	ofstream of_perpFE(perp_filenameFE.c_str());

	cout << "Writing PERPLEXITY report to: " << perp_filenameEF << '\n';
	if (!of_perpEF) {
		cerr << "\nERROR: Cannot write to " << perp_filenameEF <<'\n';
		exit(1);
	}
	if (!of_perpFE) {
			cerr << "\nERROR: Cannot write to " << perp_filenameEF <<'\n';
			exit(1);
		}

	if (testCorpus)
		generatePerplexityReport(trainPerpEF, testPerp, trainViterbiPerpEF,
				testViterbiPerp, of_perpEF, (*corpusEF).getTotalNoPairs1(), (*testCorpus).getTotalNoPairs1(), true);
	else{
		generatePerplexityReport(trainPerpEF, testPerp, trainViterbiPerpEF,
				testViterbiPerp, of_perpEF, (*corpusEF).getTotalNoPairs1(), 0, true);
		generatePerplexityReport(trainPerpFE, testPerp, trainViterbiPerpFE,
						testViterbiPerp, of_perpFE, (*corpusFE).getTotalNoPairs1(), 0, true);
	}

	string eTrainVcbFile = Prefix + ".trn.src.vcb";
	ofstream of_eTrainVcb(eTrainVcbFile.c_str());
	cout << "Writing source vocabulary list to : " << eTrainVcbFile << '\n';
	if (!of_eTrainVcb) {
		cerr << "\nERROR: Cannot write to " << eTrainVcbFile <<'\n';
		exit(1);
	}
	eTrainVcbList.printVocabList(of_eTrainVcb) ;

	string fTrainVcbFile = Prefix + ".trn.trg.vcb";
	ofstream of_fTrainVcb(fTrainVcbFile.c_str());
	cout << "Writing source vocabulary list to : " << fTrainVcbFile << '\n';
	if (!of_fTrainVcb) {
		cerr << "\nERROR: Cannot write to " << fTrainVcbFile <<'\n';
		exit(1);
	}
	fTrainVcbList.printVocabList(of_fTrainVcb) ;

	//print test vocabulary list 

	string eTestVcbFile = Prefix + ".tst.src.vcb";
	ofstream of_eTestVcb(eTestVcbFile.c_str());
	cout << "Writing source vocabulary list to : " << eTestVcbFile << '\n';
	if (!of_eTestVcb) {
		cerr << "\nERROR: Cannot write to " << eTestVcbFile <<'\n';
		exit(1);
	}
	eTestVcbList.printVocabList(of_eTestVcb) ;

	string fTestVcbFile = Prefix + ".tst.trg.vcb";
	ofstream of_fTestVcb(fTestVcbFile.c_str());
	cout << "Writing source vocabulary list to : " << fTestVcbFile << '\n';
	if (!of_fTestVcb) {
		cerr << "\nERROR: Cannot write to " << fTestVcbFile <<'\n';
		exit(1);
	}
	fTestVcbList.printVocabList(of_fTestVcb) ;
	printDecoderConfigFile();
	if (testCorpus)
		printOverlapReport(m1ef.getTTable(), *testCorpus, eTrainVcbList,
				fTrainVcbList, eTestVcbList, fTestVcbList);

}

bool readNextSent(istream&is, map< pair<int,int>,char >&s, int&number) {
	string x;
	if ( !(is >> x))
		return 0;
	if (x=="SENT:")
		is >> x;
	int n=atoi(x.c_str());
	if (number==-1)
		number=n;
	else if (number!=n) {
		cerr << "ERROR: readNextSent: DIFFERENT NUMBERS: " << number << " "
				<< n << '\n';
		return 0;
	}
	int nS, nP, nO;
	nS=nP=nO=0;
	while (is >> x) {
		if (x=="SENT:")
			return 1;
		int n1, n2;
		is >> n1 >> n2;
		map< pair<int,int>,char >::const_iterator i=s.find(pair<int, int>(n1,
				n2));
		if (i==s.end()||i->second=='P')
			s[pair<int,int>(n1,n2)]=x[0];
		massert(x[0]=='S'||x[0]=='P');
		nS+= (x[0]=='S');
		nP+= (x[0]=='P');
		nO+= (!(x[0]=='S'||x[0]=='P'));
	}
	return 1;
}

bool emptySent(map< pair<int,int>,char >&x) {
	x = map<pair<int,int>, char>();
	return 1;
}

void ReadAlignment(const string&x, Vector<map< pair<int,int>,char > >&a) {
	ifstream infile(x.c_str());
	a.clear();
	map< pair<int,int>,char > sent;
	int number=0;
	while (emptySent(sent) && (readNextSent(infile, sent, number))) {
		if (int(a.size())!=number)
			cerr << "ERROR: ReadAlignment: " << a.size() << " " << number
					<< '\n';
		a.push_back(sent);
		number++;
	}
	cout << "Read: " << a.size() << " sentences in reference alignment."
			<< '\n';
}

void initGlobals(void) {
	cerr << "DEBUG: Enter";
	NODUMPS = false;
	Prefix = Get_File_Spec();
	cerr << "DEBUG: Prefix";
	LogFilename= Prefix + ".log";
	cerr << "DEBUG: Log";
	MAX_SENTENCE_LENGTH = MAX_SENTENCE_LENGTH_ALLOWED;
}

void convert(const map< pair<int,int>,char >&reference, alignment&x) {
	int l=x.get_l();
	int m=x.get_m();
	for (map< pair<int,int>,char >::const_iterator i=reference.begin(); i
			!=reference.end(); ++i) {
		if (i->first.first+1>int(m)) {
			cerr << "ERROR m to big: " << i->first.first << " "
					<< i->first.second+1 << " " << l << " " << m
					<< " is wrong.\n";
			continue;
		}
		if (i->first.second+1>int(l)) {
			cerr << "ERROR l to big: " << i->first.first << " "
					<< i->first.second+1 << " " << l << " " << m
					<< " is wrong.\n";
			continue;
		}
		if (x(i->first.first+1)!=0)
			cerr << "ERROR: position " << i->first.first+1 << " already set\n";
		x.set(i->first.first+1, i->first.second+1);
	}
}

double ErrorsInAlignment(const map< pair<int,int>,char >&reference,
		const Vector<WordIndex>&test, int l, int&missing, int&toomuch,
		int&eventsMissing, int&eventsToomuch, int pair_no) {
	int err=0;
	for (unsigned int j=1; j<test.size(); j++) {
		if (test[j]>0) {
			map< pair<int,int>,char >::const_iterator i=
					reference.find(make_pair(test[j]-1, j-1));
			if (i==reference.end() ) {
				toomuch++;
				err++;
			} else {
				if ( !(i->second=='S' || i->second=='P')) {
					cerr << "ERROR: wrong symbol in reference alignment '"
							<< i->second << ' ' << int(i->second) << " no:" << pair_no<< "'\n";
				}
			}
			eventsToomuch++;
		}
	}
	for (map< pair<int,int>,char >::const_iterator i=reference.begin(); i
			!=reference.end(); ++i) {
		if (i->second=='S') {
			unsigned int J=i->first.second+1;
			unsigned int I=i->first.first+1;
			if (int(J)>=int(test.size())||int(I)>int(l)||int(J)<1||int(I)<1)
				cerr
						<< "ERROR: alignment outside of range in reference alignment"
						<< J << " " << test.size() << " (" << I << " " << l
						<< ") no:" << pair_no << '\n';
			else {
				if (test[J]!=I) {
					missing++;
					err++;
				}
			}
			eventsMissing++;
		}
	}
	if (Verbose)
		cout << err << " errors in sentence\n";
	if (eventsToomuch+eventsMissing)
		return (toomuch+missing)/(eventsToomuch+eventsMissing);
	else
		return 1.0;
}

vcbList *globeTrainVcbList, *globfTrainVcbList;

double StartTraining(int&result) {
	double errors=0.0;
	Vector<WordEntry> evlist,fvlist;
	vcbList eTrainVcbList(evlist), fTrainVcbList(fvlist);
	globeTrainVcbList=&eTrainVcbList;
	globfTrainVcbList=&fTrainVcbList;

	// What is being done here?
	string repFilename = Prefix + ".gizacfg";
	ofstream of2(repFilename.c_str());
	writeParameters(of2, getGlobalParSet(), -1) ;
	// Write another copy of configure file

	cout << "reading vocabulary files \n";
	eTrainVcbList.setName(SourceVocabFilename.c_str());
	fTrainVcbList.setName(TargetVocabFilename.c_str());
	eTrainVcbList.readVocabList();
	fTrainVcbList.readVocabList();

	// Vocabulary can be optional ?!

	cout << "Source vocabulary list has " << eTrainVcbList.uniqTokens()
			<< " unique tokens \n";

	cout << "Target vocabulary list has " << fTrainVcbList.uniqTokens()
			<< " unique tokens \n";

	

	corpusEF = new sentenceHandler(CorpusFilenameEF.c_str(), &eTrainVcbList, &fTrainVcbList);
	corpusFE = new sentenceHandler(CorpusFilenameFE.c_str(), &fTrainVcbList, &eTrainVcbList);
	vcbList eTestVcbList(eTrainVcbList); // Copied directly
	vcbList fTestVcbList(fTrainVcbList);
	// This portion of code should not be copied to model one
	// training
	if (TestCorpusFilename == "NONE")
		TestCorpusFilename = "";
	/////////////////////////// MODULE_TEST_START //////////////////
	if (TestCorpusFilename != "") {
		cout << "Test corpus will be read from: " << TestCorpusFilename << '\n';

		testCorpus= new sentenceHandler(
				TestCorpusFilename.c_str(),
				&eTestVcbList, &fTestVcbList);

		cout << " Test total # sentence pairs : " <<(*testCorpus).getTotalNoPairs1() <<" weighted:" <<(*testCorpus).getTotalNoPairs2() <<'\n';

		cout << "Size of the source portion of test corpus: "
				<< eTestVcbList.totalVocab() << " tokens\n";
		cout << "Size of the target portion of test corpus: "
				<< fTestVcbList.totalVocab() << " tokens \n";
		cout << "In source portion of the test corpus, only "
				<< eTestVcbList.uniqTokensInCorpus()
				<< " unique tokens appeared\n";
		cout << "In target portion of the test corpus, only "
				<< fTestVcbList.uniqTokensInCorpus()
				<< " unique tokens appeared\n";
		cout << "ratio (target/source) : " << double(fTestVcbList.totalVocab()) / eTestVcbList.totalVocab() << '\n';
	}
	cout << " Train total # sentence pairs (weighted): "
			<< corpusEF->getTotalNoPairs2() << '\n';
	cout << "Size of source portion of the training corpus: "
			<< eTrainVcbList.totalVocab()-corpusEF->getTotalNoPairs2()
			<< " tokens\n";
	cout << "Size of the target portion of the training corpus: "
			<< fTrainVcbList.totalVocab() << " tokens \n";
	cout << "In source portion of the training corpus, only "
			<< eTrainVcbList.uniqTokensInCorpus()
			<< " unique tokens appeared\n";
	cout << "In target portion of the training corpus, only "
			<< fTrainVcbList.uniqTokensInCorpus()
			<< " unique tokens appeared\n";
	cout << "lambda for PP calculation in IBM-1,IBM-2,HMM:= " << double(fTrainVcbList.totalVocab()) << "/(" << eTrainVcbList.totalVocab() << "-"
			<< corpusEF->getTotalNoPairs2() << ")=";
	LAMBDA = double(fTrainVcbList.totalVocab())
			/ (eTrainVcbList.totalVocab()-corpusEF->getTotalNoPairs2());
	cout << "= " << LAMBDA << '\n';
	/////////////////////////// MODULE_TEST_FINISH /////////////////
	// load dictionary
	Dictionary *dictionary;
	if (useDict)
		dictionary = new Dictionary(dictionary_Filename.c_str());
	else
		dictionary = new Dictionary("");

	int minIter=0;
	cerr << "Dictionary Loading complete" << endl;

	if (CoocurrenceFileEF.length()==0) {
		cerr << "ERROR: NO COOCURRENCE FILE EF GIVEN!\n";
		abort();
	}

	if (CoocurrenceFileFE.length()==0) {
			cerr << "ERROR: NO COOCURRENCE FE FILE GIVEN!\n";
			abort();
		}

	//ifstream coocs(CoocurrenceFile.c_str());
	tmodel<COUNT, PROB> tTableFE(CoocurrenceFileFE);
	tmodel<COUNT, PROB> tTableEF(CoocurrenceFileEF);
	cerr << "cooc file loading completed" << endl;
	

	// Need to rule out some bad logic
	
	if(restart == 1 && Model1_Iterations == 0) { // Restart on model 1 but not train on model one
		cerr << "You specified to load model 1 and train model 1 (restart == 1) but you specified zero Model 1 iteration, please revise your parameters";
		exit(1);
	}
	if(restart == 2 && Model2_Iterations == 0) { // Restart on model 2 but not train on model 2
		cerr << "You specified to load model 1 and train model 2 (restart == 2) but you specified zero Model 2 iteration, please revise your parameters";
		exit(1);
	}
	
	if(restart == 3 && Model2_Iterations == 0) { // Restart on model 2 but not train on model 2
		cerr << "You specified to load model 2 and train model 2 (restart == 3) but you specified zero Model 2 iteration, please revise your parameters";
		exit(1);
	}
	
	if(restart == 4 && HMM_Iterations == 0) { // Restart on model 2 but not train on model 2
		cerr << "You specified to load model 1 and train hmm (restart == 4) but you specified zero HMM iteration, please revise your parameters";
		exit(1);
	}	

	if(restart == 5 && HMM_Iterations == 0) { // Restart on model 2 but not train on model 2
		cerr << "You specified to load model 2 and train hmm (restart == 5) but you specified zero HMM iteration, please revise your parameters";
		exit(1);
	}
	
	if(restart == 6 && HMM_Iterations == 0) { // Restart on model 2 but not train on model 2
		cerr << "You specified to load HMM and train hmm (restart == 6) but you specified zero HMM iteration, please revise your parameters";
		exit(1);
	}

	if(restart == 7 && Model3_Iterations == 0) { // Restart on model 3 but not train on model 3
		cerr << "You specified to load HMM and train model 3 (restart == 7) but you specified zero Model 3 iteration, please revise your parameters";
		exit(1);
	}

	if(restart == 8 && Model3_Iterations == 0) { // Restart on model 3 but not train on model 3
		cerr << "You specified to load model 2 and train model 3 (restart == 8) but you specified zero Model 3 iteration, please revise your parameters";
		exit(1);
	}

    if(restart == 9 && Model3_Iterations == 0) { // Restart on model 3 but not train on model 3
		cerr << "You specified to load model 3 and train model 3 (restart == 9) but you specified zero Model 3 iteration, please revise your parameters";
		exit(1);
	}

    if(restart == 10 && Model4_Iterations == 0) { // Restart on model 3 but not train on model 3
		cerr << "You specified to load model 3 and train model 4 (restart == 10) but you specified zero Model 4 iteration, please revise your parameters";
		exit(1);
	}

    if(restart == 11 && Model4_Iterations == 0) { // Restart on model 3 but not train on model 3
		cerr << "You specified to load model 4 and train model 4 (restart == 10) but you specified zero Model 4 iteration, please revise your parameters";
		exit(1);
	}
	
	//QIN: If restart level is larger than 0, then we need to load 
	if (restart > 0){
		cerr << "We are going to load previous model " << prev_t << endl;
		if(!tTableEF.readProbTable(prev_t.c_str())){
			cerr << "Failed reading " << prev_t << endl; 
			exit(1);
		}
	}
	
	
	
	cerr << "TTable initialization OK" << endl;
	// TModel is important! 
	model1 m1ef(CorpusFilenameEF.c_str(), eTrainVcbList, fTrainVcbList, tTableEF, trainPerpEF, *corpusEF, &testPerp, testCorpus, trainViterbiPerpEF, &testViterbiPerp);
	model1 m1fe(CorpusFilenameFE.c_str(), fTrainVcbList, eTrainVcbList, tTableFE, trainPerpFE, *corpusFE, &testPerp, testCorpus, trainViterbiPerpFE, &testViterbiPerp);

	m1ef.set_twin(m1fe, PrefixEF, Model1_SymFrequency, T_Multiplier);
	m1fe.set_twin(m1ef, PrefixFE, Model1_SymFrequency, T_Multiplier);


	cerr << "Model one initalization OK" << endl;
	amodel<PROB> aTableEF(false);
	amodel<PROB> aTableFE(false);
	
	if (restart >2 && restart != 4 ){ // 1 is model 1, 2 is model 2 init, both just need t-table, 4 is directly train HMM from model one
	                                  // and we do not need a model
		cerr << "We are going to load previous model from " << prev_a << endl;
		if(!aTableEF.readTable(prev_a.c_str())){
			cerr << "Failed reading " << prev_a << endl;
			exit(1);
		}
	}
	
	amodel<COUNT> aCountTableEF(false);
	amodel<COUNT> aCountTableFE(false);
	model2 m2ef(m1ef, aTableEF, aCountTableEF);
	model2 m2fe(m1fe, aTableFE, aCountTableFE);
	m2ef.set_twin(m2fe, PrefixEF, Model2_SymFrequency, T_Multiplier);
	m2fe.set_twin(m2ef, PrefixFE, Model2_SymFrequency, T_Multiplier);

	WordClasses frenchEF,englishEF;
	WordClasses frenchFE,englishFE;
	hmm hef(m2ef,englishEF,frenchEF);
	hmm hfe(m2fe,englishFE,frenchFE);
	hef.set_twin(hfe, PrefixEF, HMM_SymFrequency, T_Multiplier);
	hfe.set_twin(hef, PrefixFE, HMM_SymFrequency, T_Multiplier);
	
	if (restart == 6 || restart ==7){  // If we want to initialize model 3 or continue train hmm, need to read jumps
		string al = prev_hmm + ".alpha";
		string be = prev_hmm + ".beta";
		cerr << "We are going to load previous (HMM) model from " << prev_hmm <<"," << al << "," << be << endl;
		if(!hef.probs.readJumps(prev_hmm.c_str(),NULL,al.c_str(),be.c_str())){
			cerr << "Failed reading" <<  prev_hmm <<"," << al << "," << be << endl;
			exit(1);
		}
	}
	nmodel<PROB> nTableEF(m2ef.getNoEnglishWords()+1, MAX_FERTILITY);
	amodel<PROB> dTableEF(true);
	nmodel<PROB> nTableFE(m2fe.getNoEnglishWords()+1, MAX_FERTILITY);
	amodel<PROB> dTableFE(true);
	
	if(restart > 8){ // 9, 10, 11 requires ntable and d table, 
		cerr << "We are going to load previous N model from " << prev_n << endl;
		if(!nTableEF.readNTable(prev_n.c_str())){
			cerr << "Failed reading " << prev_n << endl;
			exit(1);
		}
		cerr << "We are going to load previous D model from " << prev_d << endl;
		if(!dTableEF.readTable(prev_d.c_str())){
			cerr << "Failed reading " << prev_d << endl;
			exit(1);
		}
	
	}
	
	
	
	model3 m3ef(m2ef, dTableEF, nTableEF);
	model3 m3fe(m2fe, dTableFE, nTableFE);
	m3ef.set_twin(m3fe, PrefixEF, Model345_SymFrequency, T_Multiplier);
	m3fe.set_twin(m3ef, PrefixFE, Model345_SymFrequency, T_Multiplier);

	if(restart > 8){
		double p0,p1;
		if (P0!=-1.0||prev_p0.length()==0) {
			p0 = P0;
			p1 = 1-P0;
		}else{
			cerr << "We are going to load previous P0 Value model from " << prev_p0 << endl;
			ifstream ifs(prev_p0.c_str());
			ifs >> p0;
			p1 = 1-p0;
		}
		m3ef.p0 = p0;
		m3ef.p1 = p1;
	}

	// For loading d4 table, we postpone it to model 4 iterations in the line marked with #LOADM4#
	
	if (ReadTablePrefix.length() ) {
		string number = "final";
		string tfile, afilennfile, dfile, d4file, p0file, afile, nfile; //d5file
		tfile = ReadTablePrefix + ".t3." + number;
		afile = ReadTablePrefix + ".a3." + number;
		nfile = ReadTablePrefix + ".n3." + number;
		dfile = ReadTablePrefix + ".d3." + number;
		d4file = ReadTablePrefix + ".d4." + number;
		//d5file = ReadTablePrefix + ".d5." + number ;
		p0file = ReadTablePrefix + ".p0_3." + number;
		tTableEF.readProbTable(tfile.c_str());
		aTableEF.readTable(afile.c_str());
		m3ef.dTable.readTable(dfile.c_str());
		m3ef.nTable.readNTable(nfile.c_str());
		sentPair sent;
		double p0;
		ifstream p0f(p0file.c_str());
		p0f >> p0;
		d4model d4m(MAX_SENTENCE_LENGTH,*(new WordClasses()), *(new WordClasses()));

		//d4m.readProbTable(d4file.c_str());
		//d5model d5m(d4m);
		//d5m.makeWordClasses(m1.Elist,m1.Flist,SourceVocabFilename+".classes",TargetVocabFilename+".classes");
		//d5m.readProbTable(d5file.c_str());
		makeSetCommand("model4smoothfactor", "0.0", getGlobalParSet(), 2);
		//makeSetCommand("model5smoothfactor","0.0",getGlobalParSet(),2);
		if (corpusEF||testCorpus) {
			sentenceHandler *x=corpusEF;
			if (x==0)
				x=testCorpus;
			cout << "Text corpus exists.\n";
			x->rewind();
			while (x&&x->getNextSentence(sent)) {
				Vector<WordIndex>& es = sent.eSent;
				Vector<WordIndex>& fs = sent.fSent;
				int l=es.size()-1;
				int m=fs.size()-1;
				transpair_model4 tm4(es, fs, m1ef.tTable, m2ef.aTable, m3ef.dTable,
						m3ef.nTable, 1-p0, p0, &d4m);
				alignment al(l, m);
				cout << "I use the alignment " << sent.sentenceNo-1 << '\n';
				//convert(ReferenceAlignment[sent.sentenceNo-1],al);
				transpair_model3 tm3(es, fs, m1ef.tTable, m2ef.aTable, m3ef.dTable,
						m3ef.nTable, 1-p0, p0, 0);
				double p=tm3.prob_of_target_and_alignment_given_source(al, 1);
				cout << "Sentence " << sent.sentenceNo << " has IBM-3 prob "
						<< p << '\n';
				p=tm4.prob_of_target_and_alignment_given_source(al, 3, 1);
				cout << "Sentence " << sent.sentenceNo << " has IBM-4 prob "
						<< p << '\n';
				//transpair_model5 tm5(es,fs,m1.tTable,m2.aTable,m3.dTable,m3.nTable,1-p0,p0,&d5m);
				//p=tm5.prob_of_target_and_alignment_given_source(al,3,1);
				//cout << "Sentence " << sent.sentenceNo << " has IBM-5 prob " << p << '\n';
			}
		} else {
			cout << "No corpus exists.\n";
		}
	} else {
		// initialize model1
		bool seedModel1 = false;
		if (Model1_Iterations > 0 && restart < 2) {
			if (t_Filename != "NONE" && t_Filename != "") {
				seedModel1 = true;
				m1ef.load_table(t_Filename.c_str());
				m1fe.load_table(t_Filename.c_str());
			}

			if(restart ==1) seedModel1 = true;
			if(Model2_Iterations == 0 && HMM_Iterations == 0 && 
			    Model3_Iterations == 0 && Model4_Iterations == 0 &&
			    Model5_Iterations == 0 && dumpCount){  // OK we need to output!
				//Prefix=PrefixEF;

				thread_model tM1;
				tM1.m1 = &m1ef;
				tM1.it=Model1_Iterations;
				tM1.seed=seedModel1;
				tM1.dict=dictionary;
				tM1.useDict=useDict;
				string pref=(countPrefix.length() == 0 ? "./" : countPrefix.c_str());
				char *s2 = new char[pref.size()+1];
				strcpy(s2, pref.c_str());
				tM1.countPrefix=s2;
				tM1.dump=dumpCountUsingWordString;
				tM1.result=0;
				tM1.mode=0;
				tM1.valid = pthread_create(&(tM1.thread),NULL,runThreadModel1,&(tM1));

				/*minIter=m1ef.em_with_tricks(Model1_Iterations, seedModel1,
				    *dictionary, useDict,true,
				    countPrefix.length() == 0 ? "./" : countPrefix.c_str(),
				    dumpCountUsingWordString
				    );*/
				//Prefix=PrefixFE;
				minIter=m1fe.em_with_tricks(Model1_Iterations, seedModel1,
								    *dictionary, useDict,true,
								    countPrefix.length() == 0 ? "./" : countPrefix.c_str(),
								    dumpCountUsingWordString
								    );
				pthread_join((tM1.thread),NULL);
			}else{
				//Prefix=PrefixEF;
				thread_model tM1;
				tM1.m1 = &m1ef;
				tM1.it=Model1_Iterations;
				tM1.dict=dictionary;
				tM1.useDict=useDict;
				tM1.result=0;
				tM1.mode=1;
				tM1.valid = pthread_create(&(tM1.thread),NULL,runThreadModel1,&(tM1));
				/*minIter=m1ef.em_with_tricks(Model1_Iterations, true,
				    *dictionary, useDict);*/
				//Prefix=PrefixFE;
				minIter=m1fe.em_with_tricks(Model1_Iterations, true,
								    *dictionary, useDict);
				pthread_join((tM1.thread),NULL);
			}
			
			errors=m1ef.errorsAL();
			errors+=m1fe.errorsAL();
		}
		{
			if (Model2_Iterations > 0 && (restart < 2 || restart ==2 || restart == 3)) {
				if(restart == 2) m2ef.initialize_table_uniformly(*corpusEF);
				if(HMM_Iterations == 0 && 
			    Model3_Iterations == 0 && Model4_Iterations == 0 &&
			    Model5_Iterations == 0 && dumpCount){
					thread_model tM2;
					tM2.m2 = &m2ef;
					tM2.it=Model2_Iterations;
					string pref=(countPrefix.length() == 0 ? "./" : countPrefix.c_str());
					char *s2 = new char[pref.size()+1];
					strcpy(s2, pref.c_str());
					tM2.countPrefix=s2;
					tM2.dump=dumpCountUsingWordString;
					tM2.result=0;
					tM2.mode=0;
					tM2.valid = pthread_create(&(tM2.thread),NULL,runThreadModel2,&(tM2));
					//minIter=m2ef.em_with_tricks(Model2_Iterations,true, countPrefix.length() == 0 ? "./" : countPrefix.c_str(), dumpCountUsingWordString);
					minIter=m2fe.em_with_tricks(Model2_Iterations,true, countPrefix.length() == 0 ? "./" : countPrefix.c_str(), dumpCountUsingWordString);
					pthread_join((tM2.thread),NULL);
				}else{
					thread_model tM2;
					tM2.m2 = &m2ef;
					tM2.it=Model2_Iterations;
					tM2.result=0;
					tM2.mode=1;
					tM2.valid = pthread_create(&(tM2.thread),NULL,runThreadModel2,&(tM2));
					//minIter=m2ef.em_with_tricks(Model2_Iterations);
					minIter=m2fe.em_with_tricks(Model2_Iterations);
					pthread_join((tM2.thread),NULL);
				}
				errors=m2ef.errorsAL();
				errors+=m2fe.errorsAL();
			}
			//cout << tTable.getProb(2, 2) << endl;
			
			
			if (HMM_Iterations > 0 && (restart < 2 || restart == 4 || restart == 5 || restart == 6)) {
				cout << "NOTE: I am doing iterations with the HMM model!\n";
				
				hef.makeWordClasses(m1ef.Elist, m1ef.Flist, SourceVocabFilename
						+".classes", TargetVocabFilename+".classes");
				hfe.makeWordClasses(m1fe.Elist, m1fe.Flist, TargetVocabFilename+".classes", SourceVocabFilename
						+".classes");
				if(restart != 6) {
					hef.initialize_table_uniformly(*corpusEF);
					hfe.initialize_table_uniformly(*corpusFE);
				}
				
				if(Model3_Iterations == 0 && Model4_Iterations == 0 &&
			       Model5_Iterations == 0 && dumpCount){
					thread_model tHMM;
					tHMM.mh = &hef;
					tHMM.it=HMM_Iterations;
					string pref=(countPrefix.length() == 0 ? NULL : countPrefix.c_str());
					char *s2 = new char[pref.size()+1];
					strcpy(s2, pref.c_str());
					tHMM.countPrefix=s2;
					tHMM.dump=dumpCountUsingWordString;
					tHMM.result=0;
					tHMM.mode=0;
					tHMM.valid = pthread_create(&(tHMM.thread),NULL,runThreadHMM,&(tHMM));

					/*minIter=hef.em_with_tricks(HMM_Iterations,true,
				    countPrefix.length() == 0 ? NULL : countPrefix.c_str(),
				    dumpCountUsingWordString, restart == 6);*/

					minIter=hfe.em_with_tricks(HMM_Iterations,true,
					countPrefix.length() == 0 ? NULL : countPrefix.c_str(),
					dumpCountUsingWordString, restart == 6);
					pthread_join((tHMM.thread),NULL);
				}else{			
					thread_model tHMM;
					tHMM.mh = &hef;
					tHMM.it=HMM_Iterations;
					tHMM.result=0;
					tHMM.mode=1;
					tHMM.valid = pthread_create(&(tHMM.thread),NULL,runThreadHMM,&(tHMM));
					//minIter=hef.em_with_tricks(HMM_Iterations,false,NULL,false,restart==6);

					minIter=hfe.em_with_tricks(HMM_Iterations,false,NULL,false,restart==6);
					pthread_join((tHMM.thread),NULL);
				}
				//multi_thread_em(HMM_Iterations, NCPUS, &h);
				errors=hef.errorsAL();
				errors+=hfe.errorsAL();
			}
			if ( ((Transfer2to3 && Model2_Iterations>0)||(HMM_Iterations==0&&Model2_Iterations>0)||restart==8) && (restart!=7 && restart < 9)) {
				if (HMM_Iterations>0)
					cout << "WARNING: transfor is not needed, as results "
						"are overwritten bei transfer from HMM.\n";
				string test_alignfile = Prefix +".tst.A2to3";
				if (testCorpus)
					m2ef.em_loop(testPerp, *testCorpus, Transfer_Dump_Freq==1
							&&!NODUMPS, test_alignfile.c_str(),
							testViterbiPerp, true);
				if (testCorpus)
					cout << "\nTransfer: TEST CROSS-ENTROPY "
							<< testPerp.cross_entropy() << " PERPLEXITY "
							<< testPerp.perplexity() << "\n\n";
				if (Transfer == TRANSFER_SIMPLE){
					m3ef.transferSimple(*corpusEF, Transfer_Dump_Freq==1&&!NODUMPS,
							trainPerpEF, trainViterbiPerpEF);
					m3fe.transferSimple(*corpusFE, Transfer_Dump_Freq==1&&!NODUMPS,
							trainPerpFE, trainViterbiPerpFE);
				}
				else
					{
					m3ef.transfer(*corpusEF, Transfer_Dump_Freq==1&&!NODUMPS,
							trainPerpEF, trainViterbiPerpEF);
					m3fe.transfer(*corpusFE, Transfer_Dump_Freq==1&&!NODUMPS,
												trainPerpFE, trainViterbiPerpFE);
					}
				errors=m3ef.errorsAL();
				errors+=m3fe.errorsAL();
			}
			if(restart == 7 ){
				hef.makeWordClasses(m1ef.Elist, m1ef.Flist, SourceVocabFilename
						+".classes", TargetVocabFilename+".classes");
				hfe.makeWordClasses(m1fe.Elist, m1fe.Flist, TargetVocabFilename+".classes", SourceVocabFilename
						+".classes");
			}
			if (HMM_Iterations>0 || restart == 7)
				m3ef.setHMM(&hef);
				m3fe.setHMM(&hfe);
			
			if (Model3_Iterations > 0 || Model4_Iterations > 0
					|| Model5_Iterations || Model6_Iterations) {
						
				if(restart == 11){ // Need to load model 4
				    if (Model5_Iterations==0 && Model6_Iterations==0 && dumpCount){
						minIter=m3ef.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations,prev_d4.c_str(),prev_d4_2.c_str()
						,true,
						countPrefix.length() == 0 ? "./" : countPrefix.c_str(),
						dumpCountUsingWordString); // #LOADM4#
					}else{
						minIter=m3ef.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations,prev_d4.c_str(),prev_d4_2.c_str());
					}
				}else{
				    if (Model5_Iterations==0 && Model6_Iterations==0 && dumpCount){
				    	thread_model tM3;
				    	tM3.m3 = &m3ef;
				    	tM3.it=Model3_Iterations;
				    	tM3.itM4 = Model4_Iterations;
				    	tM3.itM5 = Model5_Iterations;
				    	tM3.itM6 = Model6_Iterations;
				    	string pref=(countPrefix.length() == 0 ? "./" : countPrefix.c_str());
				    	char *s2 = new char[pref.size()+1];
				    	strcpy(s2, pref.c_str());
				    	tM3.countPrefix=s2;
				    	tM3.dump=dumpCountUsingWordString;
				    	tM3.result=0;
				    	tM3.mode=0;
				    	tM3.valid = pthread_create(&(tM3.thread),NULL,runThreadModel3,&(tM3));

				    	/*minIter=m3ef.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations,NULL,NULL
						,true,
						countPrefix.length() == 0 ? "./" : countPrefix.c_str(),
						dumpCountUsingWordString);*/ // #LOADM4#

						minIter=m3fe.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations,NULL,NULL
												,true,
												countPrefix.length() == 0 ? "./" : countPrefix.c_str(),
												dumpCountUsingWordString);
						pthread_join((tM3.thread),NULL);
				    }else{
						thread_model tM3;
						tM3.m3 = &m3ef;
						tM3.it=Model3_Iterations;
						tM3.itM4 = Model4_Iterations;
						tM3.itM5 = Model5_Iterations;
						tM3.itM6 = Model6_Iterations;
						tM3.result=0;
						tM3.mode=1;
						tM3.valid = pthread_create(&(tM3.thread),NULL,runThreadModel3,&(tM3));
						//minIter=m3ef.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations,NULL,NULL);
						minIter=m3fe.viterbi(Model3_Iterations,Model4_Iterations,Model5_Iterations,Model6_Iterations,NULL,NULL);
						pthread_join((tM3.thread),NULL);
				    }
				}
				/*multi_thread_m34_em(m3, NCPUS, Model3_Iterations,
						Model4_Iterations);*/
				errors=m3ef.errorsAL();
				errors+=m3fe.errorsAL();
			}
			if (FEWDUMPS||!NODUMPS) {
				printAllTables(eTrainVcbList, eTestVcbList, fTrainVcbList,
						fTestVcbList, m1ef);
				/*printAllTables(fTrainVcbList, fTestVcbList, eTrainVcbList,
										eTestVcbList, m1fe);*/
			}
		}
	}
	result=minIter;
	return errors;
}

/*!
 Starts here
 */
int main(int argc, char* argv[]) {
	////////////////////////////////////////////////////////
	//    Setup parameters
	///////////////////////////////////////////////////////
	cerr << "Starting MGIZA " << endl;
	getGlobalParSet().insert(new Parameter<string>(
			"CoocurrenceFileEF",
			ParameterChangedFlag,
			"",
			CoocurrenceFileEF,
			PARLEV_SPECIAL));
	getGlobalParSet().insert(new Parameter<string>(
				"CoocurrenceFileFE",
				ParameterChangedFlag,
				"",
				CoocurrenceFileFE,
				PARLEV_SPECIAL));
	getGlobalParSet().insert(new Parameter<string>(
			"ReadTablePrefix",
			ParameterChangedFlag,
			"optimized",
			ReadTablePrefix,-1));

	getGlobalParSet().insert(new Parameter<string>("S",
			ParameterChangedFlag,
			"source vocabulary file name",
			SourceVocabFilename,
			PARLEV_INPUT));
	getGlobalParSet().insert(new Parameter<string>(
			"SOURCE VOCABULARY FILE",
			ParameterChangedFlag,
			"source vocabulary file name",
			SourceVocabFilename,-1));

	getGlobalParSet().insert(new Parameter<string>("T",
			ParameterChangedFlag,
			"target vocabulary file name",
			TargetVocabFilename,
			PARLEV_INPUT));
	getGlobalParSet().insert(new Parameter<string>(
			"TARGET VOCABULARY FILE",
			ParameterChangedFlag,
			"target vocabulary file name",
			TargetVocabFilename,-1));
	getGlobalParSet().insert(new Parameter<string>(
			"CEF",
			ParameterChangedFlag,
			"training corpus EF file name",
			CorpusFilenameEF,PARLEV_INPUT));
	getGlobalParSet().insert(new Parameter<string>(
				"CFE",
				ParameterChangedFlag,
				"training FE corpus file name",
				CorpusFilenameFE,PARLEV_INPUT));
	getGlobalParSet().insert(new Parameter<string>(
			"CORPUS FILE",
			ParameterChangedFlag,
			"training corpus file name",
			CorpusFilenameEF,-1));
	getGlobalParSet().insert(new Parameter<string>("TC",
			ParameterChangedFlag,
			"test corpus file name",
			TestCorpusFilename,
			PARLEV_INPUT));

	getGlobalParSet().insert(new Parameter<string>(
			"TEST CORPUS FILE",
			ParameterChangedFlag,
			"test corpus file name",
			TestCorpusFilename,-1));

	getGlobalParSet().insert(new Parameter<string>("d",
			ParameterChangedFlag,
			"dictionary file name",
			dictionary_Filename,
			PARLEV_INPUT));

	getGlobalParSet().insert(new Parameter<string>(
			"DICTIONARY",
			ParameterChangedFlag,
			"dictionary file name",
			dictionary_Filename,-1));

	getGlobalParSet().insert(new Parameter<string>("l",
			ParameterChangedFlag,
			"log file name",
			LogFilename,PARLEV_OUTPUT));
	getGlobalParSet().insert(new Parameter<string>(
			"LOG FILE",
			ParameterChangedFlag,
			"log file name",
			LogFilename,-1));
	getGlobalParSet().insert(new Parameter<string>("o",
				ParameterChangedFlag,
				"output file prefix",
				Prefix,PARLEV_OUTPUT));
	getGlobalParSet().insert(new Parameter<string>("oef",
			ParameterChangedFlag,
			"output file prefix EF",
			PrefixEF,PARLEV_OUTPUT));
	getGlobalParSet().insert(new Parameter<string>("ofe",
				ParameterChangedFlag,
				"output file prefix FE",
				PrefixFE,PARLEV_OUTPUT));

	getGlobalParSet().insert(new Parameter<string>("alig",
			ParameterChangedFlag,
			"alignment at the end",
			Alignment,PARLEV_OUTPUT));

	getGlobalParSet().insert(new Parameter<string>("diagonal",
				ParameterChangedFlag,
				"diagonal at the end",
				Diagonal,PARLEV_OUTPUT));

	getGlobalParSet().insert(new Parameter<string>("final",
				ParameterChangedFlag,
				"final at the end",
				Final,PARLEV_OUTPUT));

	getGlobalParSet().insert(new Parameter<string>("both",
				ParameterChangedFlag,
				"both uncovered at the end",
				Both,PARLEV_OUTPUT));

	getGlobalParSet().insert(new Parameter<string>(
			"OUTPUT FILE PREFIX",
			ParameterChangedFlag,
			"output file prefix",PrefixEF,-1));

	getGlobalParSet().insert(new Parameter<string>(
			"OUTPUT PATH",
			ParameterChangedFlag,
			"output path",
			OPath,PARLEV_OUTPUT));

	getGlobalParSet().insert(new Parameter<string>(
			"Previous T",
			ParameterChangedFlag,
			"The t-table of previous step",
			prev_t,PARLEV_INPUT));	
	
	getGlobalParSet().insert(new Parameter<string>(
			"Previous A",
			ParameterChangedFlag,
			"The a-table of previous step",
			prev_a,PARLEV_INPUT));	
	
	getGlobalParSet().insert(new Parameter<string>(
			"Previous D",
			ParameterChangedFlag,
			"The d-table of previous step",
			prev_d,PARLEV_INPUT));		

	getGlobalParSet().insert(new Parameter<string>(
			"Previous N",
			ParameterChangedFlag,
			"The n-table of previous step",
			prev_n,PARLEV_INPUT));	
	
	getGlobalParSet().insert(new Parameter<string>(
			"Previous D4",
			ParameterChangedFlag,
			"The d4-table of previous step",
			prev_d4,PARLEV_INPUT));		
	getGlobalParSet().insert(new Parameter<string>(
			"Previous D42",
			ParameterChangedFlag,
			"The d4-table (2) of previous step",
			prev_d4_2,PARLEV_INPUT));			

	getGlobalParSet().insert(new Parameter<string>(
			"Previous P0",
			ParameterChangedFlag,
			"The P0 previous step",
			prev_p0,PARLEV_INPUT));	
	
	getGlobalParSet().insert(new Parameter<string>(
			"Previous HMM",
			ParameterChangedFlag,
			"The hmm-table of previous step",
			prev_hmm,PARLEV_INPUT));	

	getGlobalParSet().insert(new Parameter<string>(
			"Count Output Prefix",
			ParameterChangedFlag,
			"The prefix for output counts",
			countPrefix,PARLEV_OUTPUT));	
	// Timers
	time_t st1, fn;
	st1 = time(NULL); // starting time

//	cerr << "---------------- " << endl;
	// Program Name
	
	string temp(argv[0]);
	Usage = temp + " <config_file> [options]\n";

	// At least, config file should be provided.
	if (argc < 2) {
		printHelp();
		exit(1);
	}
	cerr << "Initializing Global Paras " << endl;
	// 
	initGlobals() ;

	cerr << "Parsing Arguments " << endl;	
	// 
	parseArguments(argc, argv);

	
	cerr << "Opening Log File " << endl;	
	if (Log) {
		logmsg.open(LogFilename.c_str(), ios::out);
	}

	cerr << "Printing parameters " << endl;	

	printGIZAPars(cout);

	int a=-1;

	double errors=0.0;

	if (OldADBACKOFF!=0)
		cerr
				<< "WARNING: Parameter -adBackOff does not exist further; use CompactADTable instead.\n";

	if (MAX_SENTENCE_LENGTH > MAX_SENTENCE_LENGTH_ALLOWED)
		cerr << "ERROR: MAX_SENTENCE_LENGTH is too big " << MAX_SENTENCE_LENGTH
				<< " > " << MAX_SENTENCE_LENGTH_ALLOWED << '\n';

	// Actually word is done here
	errors=StartTraining(a);

	fn = time(NULL); // finish time

	//merge all A3

	cout << '\n' << "Entire Training took: " << difftime(fn, st1)
				<< " seconds\n";
	cout << "Program Finished at: "<< ctime(&fn) << '\n';
	cout << "==========================================================\n";

	cout << "Merge alignments ..."<< endl;
	merge_alig((PrefixEF+".A3.final").c_str());
	merge_alig((PrefixFE+".A3.final").c_str());

	if(endSymmetrization == 1){
	cout << "Running end symmetrization ..."<< endl;
	//sym end;
	string sin = PrefixEF+".A3.final";
	string sinInv = PrefixFE+".A3.final";
	string sout = Prefix+".A3.final_comp";
	const char *in = sin.c_str();
	const char *inInv = sinInv.c_str();
	const char *out = sout.c_str();
	create_file(in, inInv, out);

	int alignment = 2;
	int diagonal = 1;
	int final = 1;
	int bothuncovered = 1;
	string sinput = Prefix+".A3.final_comp";
	string soutput = Prefix+".A3.final_symal";
	const char* input = sinput.c_str();
	const char* output = soutput.c_str();


		 if(strContains(Alignment, "union") || strContains(Alignment, "u")) alignment = UNION;
	else if(strContains(Alignment, "intersect") || strContains(Alignment, "i")) alignment = INTERSECT;
	else if(strContains(Alignment, "srctotgt") || strContains(Alignment, "s2t")) alignment = SRCTOTGT;
	else if(strContains(Alignment, "tgttosrc") || strContains(Alignment, "t2s")) alignment = TGTTOSRC;
	else if(strContains(Alignment, "grow") || strContains(Alignment, "g")) alignment = GROW;

	if(strContains(Diagonal, "yes") || strContains(Diagonal, "true") || strContains(Diagonal, "y") ) diagonal = BOOL_YES;
	if(strContains(Diagonal, "no") || strContains(Diagonal, "false") || strContains(Diagonal, "n") ) diagonal = BOOL_NO;

	if(strContains(Final, "yes") || strContains(Final, "true") || strContains(Final, "y") ) final = BOOL_YES;
	if(strContains(Final, "no") || strContains(Final, "false") || strContains(Final, "n") ) final = BOOL_NO;

	if(strContains(Both, "yes") || strContains(Both, "true") || strContains(Both, "y") ) bothuncovered = BOOL_YES;
	if(strContains(Both, "no") || strContains(Both, "false") || strContains(Both, "n") ) bothuncovered = BOOL_NO;


	cout << "Running last symmetrization with parameters: "<<endl;
	cout<< "-alignment: "<<  Alignment << " : " << alignment << endl;
	cout<< "-diagonal: "<<  Diagonal << " : " << diagonal << endl;
	cout<< "-final: "<<  final << " : " << final << endl;
	cout<< "-both: "<<  Both << " : " << bothuncovered << endl;

	last_symetrize(alignment, diagonal, final, bothuncovered, input, output);
	}
	return 0;
}

