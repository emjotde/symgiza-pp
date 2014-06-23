
SyMGIZA++ 
=========

SyMGIZA++ is a tool based on GIZA++ and MGIZA++. It extends the algorithms employed in both programs with a symmetrization mechanism, i.e. two directed word alignment models are trained in parallel and parameters of both models can be used between iterations to compute the parameters of the next model. We were able to show that this information interchange between models tends to improve word alignment quality directly and translation quality indirectly. 

The main features of SyMGIZA++ are:
 * the symmetrization mechanism,
 * multi-threading - SyMGIZA++ inherits the multithreading capacities of MGIZA++, the two directed models are trained simultanously.

If you use this, please cite:

__SyMGiza++: Symmetrized Word Alignment Models for Machine Translation.__ Marcin Junczys-Dowmunt and Arkadiusz Szał. In Pascal Bouvry, Mieczyslaw A. Klopotek, Franck Leprévost, Malgorzata Marciniak, Agnieszka Mykowiecka, and Henryk Rybinski, editors, Security and Intelligent Information Systems (SIIS), volume 7053 of Lecture Notes in Computer Science, pages 379-390, Warsaw, Poland, 2012. Springer. http://emjotde.github.io/publications/pdf/mjd2011siis.pdf

    @inproceedings{junczys_siis_2012,
      author = {Marcin Junczys-Dowmunt and Arkadiusz Szał},
      title = {SyMGiza++: Symmetrized Word Alignment Models for Machine Translation},
      booktitle = {Security and Intelligent Information Systems (SIIS)},
      year = {2012},
      editor = {Bouvry, Pascal and Klopotek, Mieczyslaw A. and Leprévost, Franck and Marciniak, Malgorzata and Mykowiecka, Agnieszka and Rybinski, Henryk},
      series = {Lecture Notes in Computer Science},
      volume = {7053},
      pages = {379-390},
      address = {Warsaw, Poland},
      publisher = {Springer},
      url = {http://emjotde.github.io/publications/pdf/mjd2011siis.pdf},
    }


Installation
------------

The requirements for the compilation of SyMGIZA++ are the same as for MGIZA++: the boost libraries. These can be downloaded from http://www.boost.org. After unpacking, proceed with the following commands:
    
    ./configure --prefix=installation_folder
    make
    make install

Parameters
----------

This paragraph describes all parameters used in SyMGIZA++. We tried to mimic most of the parameters of GIZA++ and MGIZA++, but there are some required changes (If more than one aliases appear in the command line then the one appears latest will be kept):

    -ncpu – number of threads, this is a parameter affects both directed models, i.e. -ncpu 5 will run 5 threads for each direction, resulting in 10 threads (subject to change in future versions),

input files – description of input parameters:

    -testcorpusfile (or -tc) – input corpus file (the snt file), the file will only be used in alignment but the counts will not affect the models,
    -sourcevocabularyfile (or -s) – source vocabulary file (.vcb),
    -targetvocabularyfile (or -t) – target vocabulary (.vcb).

number of iteration parameters – defines the training sequence of particular models – it describes number of iteration for each model:

    -m1 (or -noiterationsmodel1) – number of iterations for model 1,
    -m2 (or -noiterationsmodel2) – number of iterations for model 2,
    -m3 (or -noiterationsmodel3) – number of iterations for model 3,
    -m4 (or -noiterationsmodel4) – number of iterations for model 4,
    -m5 (or -noiterationsmodel5) – number of iterations for model 5,
    -m6 (or -noiterationsmodel6) – number of iterations for model 6,
    -mh (or -numberofiterationsforhmmalignmentmodel) – number of iterations for mhmm,

output files – parameters below describe files that will be created by SyMGIZA++:

    -compactalignmentformat true/false – the compact format contains only alignment links, such as 1 2 3 2
    -countoutputprefix – do not dump the normalized model but the count tables before normalization,
    -dumpcount true/false – dumping counts in addition to the normalized models, the count is dumped in the final step,
    -dumpcountusingwordstring true/false – the word surface form will be used,
    -logfile (or -l) – the path for log file,
    -model1dumfrequency (or -t1) – dump model in every t1 iterations,
    -model2dumfrequency (or -t2) – dump model in every t2 iterations,
    -model345dumfrequency (or -t345) – dump model in every t345 iterations,
    -hmmdumfrequency (or -th) – dump model in every th iterations,
    -transferdumpfrequency (or -t2to3) – additional iteration to transfer from model 2 to model 3 is dumping,
    -nbestalignmets true/false – no dumps will be made for model files and alignment files,
    -onlyaldumps true/false – only dump alignment,

other parameters:

    -probsmooth float – when a probability entry cannot be fund on models, the value will be used.

New parameters implements in extension:

    -corpusfileEF (or -cef) – input corpus file for first direction – the snt file,
    -corpusfileFE (or -cfe) – input corpus file for second direction – the snt file,
    -CoocurrenceFileFE – co-occurrence file for first direction,
    -CoocurrenceFileEF - co-occurrence file for second direction,
    -oef – output alias for generated files in first direction,
    -ofe – output alias for generated files in second direction,
    -o – output alias for other generated files,
    -m1symfrequency - symmetrization frequency of iteration for model 1,
    -m2symfrequency - symmetrization frequency of iteration for model 2,
    -m345symfrequency - symmetrization frequency of iteration for model 3, 4 and 5,
    -mhsymfrequency - symmetrization frequency of iteration for hmm,
    -tm – symmetrization t tables multiplier – this parameter precise importance of symmetrization, if this parameter is bigger, then words connected find in both direction have bigger probability of good connection,
    -es - run final symmetrization (1 - yes, 0 - no),
    -alig - type of alignment for final symmetrization, available options (similar to Moses alignment option):
        intersect
        union
        grow
        srctotgt
        tgttosrc
    -diagonal - do diagonal on final symmetrization (yes or no),
    -final - (yes or no),
    -both - (yes or no).

The alignment output of SyMGIZA++ will be:

    prefixOfDirectionOne.A3.final.part0
    prefixOfDirectionOne.A3.final.part1
    ...
    prefixOfDirectionTwo.A3.final.part0
    prefixOfDirectionTwo.A3.final.part1
    ...
    
To combine the alignments you need to run:

    installation_folder/scripts/merge_alignment.py prefixOfDirectionOne.A3.final.part> prefixOfDirectionOne.A3.final
    installation_folder/scripts/merge_alignment.py prefixOfDirectionTwo.A3.final.part> prefixOfDirectionOne.A3.final


A minimal example how to run SyMGIZA++
--------------------------------------

Run plain2snt executables file to generate snt files for each direction from two corpuses (Corpus1 and Corpus2):

    installation_folder/src/plain2snt ./Corpus1 ./$Corpus2
    installation_folder/src/plain2snt ./Corpus2 ./$Corpus1

Next generate words classes:

    installation_folder/src/mkcls/mkcls -pCorpus1 –VCorpus1.vcb.classes
    installation_folder/src/mkcls/mkcls -pCorpus2 -VCorpus2.vcb.classes

Run snt2cooc executable to generate a co-occurrence file:
    
    installation_folder/src/snt2cooc c1-c2.cooc Corpus1.vcb Corpus2.vcb Corpus1_Corpus2.snt
    installation_folder/src/snt2cooc c2-c1.cooc Corpus2.vcb Corpus1.vcb Corpus2_Corpus1.snt

After preparing this file You can run the main process of computation – SyMGIZA++:
 
    installation_folder /src/symgiza -ncpus 2 -s Corpus1.vcb -t Corpus2.vcb -cef Corpus1_Corpus2.snt
     -cfe Corpus2_Corpus1.snt -oef c1 -ofe c2 -o all -CoocurrenceFileEF c1-c2.cooc -CoocurrenceFileFE c2-c1.cooc
     -m1 5 -m2 5 -m3 5 -m4 5 -mh 5 -t1 5 -t2 5 -th 5 -t345 5 -m1symfrequency 5 -m2symfrequency 5
     -mhsymfrequency 5 -m345symfrequency 5 -tm 2 -es 1 -alig intersect -diagonal yes -final yes -both yes

The following parameters are set in the above program call:

    -npus 2 – two thread for each direction of computation
    -s Corpus1.vcb – name of the source corpus
    -t Corpus2.vcb – name of the target corpus
    -cef Corpus2_Corpus1.snt – name of the snt file for first direction
    -cfe Corpus2_Corpus1.snt – name of the snt file for second direction
    -oef c1 – alias for output files in first direction
    -ofe c2 – alias for output files in second direction
    -o all - – alias for another output files
    -CoocurrenceFileEF c1-c2.cooc – co-occurence file for first direction
    -CoocurrenceFileFE c2-c1.cooc – co-occurence file for second direction
    -m1 5 – number of iterations for model 1
    -m2 5 – number of iterations for model 2
    -m3 5 – number of iterations for model 3
    -m4 5 – number of iterations for model 4
    -mh 5 – number of iterations for hmm
    -t1 5 – dump model 1 after fifth iteration
    -t2 5 – dump model 2 after fifth iteration
    -th 5 – dump hmm after fifth iteration
    -t345 5 – dump model 3, 4 and 5 after fifth iteration
    -m1symfrequency 5 – symmetrization after five iterations of model 1
    -m2symfrequency 5 – symmetrization after five iterations of model 2
    -mhsymfrequency 5 – symmetrization after five iterations of hmm
    -m345symfrequency 5 – symmetrisation after five iterations of model 3, 4 and 5
    -tm 2 – multiple symmetrisation through two (mjd: ???)
    -es 1 - run final symmetrization
    -alig intersect - run intersection on final symmetrization
    -diagonal yes
    -final yes
    -both yes

Final results:

    c1.A3.final - final alignment for computation from source to target without final symmetrization,
    c2.A3.final - final alignment for computation from target to source without final symmetrization,
    all.A3.final_symal - - final alignment with final symmetrization.

We have included a bash script that runs SymGIZA++ with the default parameters:
    
    ./run.sh source_file_name target_file_name input_files_path
