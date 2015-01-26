(* Download final-proj.zip to your unix directory and unzip it. *)

unzip final-proj.zip

(* Build Compile after cleaning *)

make clean
make all

(* To run tool and get all arguments needed to run *)

./biostat.native -help

(* Two Modes of Running tool *)
   
 Mode 1: Sequence Alignment : This will compare your
   entered sequence to ones stored in the corresponding database

   argument 1  - What is the sequence  
   DNA | RNA | or protein

   argument 2 - What algorithm to run
   local | global | repeats

   argument 3 - Output file name - must end in .html

(* Sample run 1 *)
./biostat.native DNA global output1.html


Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
-200
Enter the maximum number of hits you would like returned in this search: 10
Input a sequence or filename: dna_query1.seq

(*Within the code directory, check for your .html file.*)

(* Sample run 2 *)
./biostat.native rna local output2.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
10
Enter the maximum number of hits you would like returned in this search: 10
Input a sequence or filename: rna_query1.seq

(*Within the code directory, check for your .html file.*)

(* Sample run 3 *)
./biostat.native protein repeats protein_repeats.html 

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10
Input a sequence or filename: protein_query1.seq

(*Within the code directory, check for your .html file.*)

(****NOTE: The following sample runs use authentic 16S Ribosomal RNA sequences as queries and run them against our database of other RNA sequences.
The database includes all the authentic 16S sequences as well as other test sequences. Highly recommended!!!! *****)

(*All output files will show up in the code directory. *)

(* Sample run 4 *)
./biostat.native rna global human_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query2_human.seq

(* Sample run 5 *)
./biostat.native rna global yeast_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query3_yeast.seq

(* Sample run 6 *)
./biostat.native rna global corn_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query4_corn.seq

(* Sample run 7 *)
./biostat.native rna global ecoli_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query5_ecoli.seq

(* Sample run 8 *)
./biostat.native rna global anacystis_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query6_anacystis.seq

(* Sample run 9 *)
./biostat.native rna global thermatoga_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query7_thermatoga.seq

(* Sample run 10 *)
./biostat.native rna global methano_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query8_methano.seq

(* Sample run 11 *)
./biostat.native rna global thermo_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query9_thermo.seq

(* Sample run 12 *)
./biostat.native rna global sulfolobus_rna.html

Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.
0
Enter the maximum number of hits you would like returned in this search: 10

Input a sequence or filename: rna_query10_sulfolobus.seq


-----------------

(* Mode 2 : Sequence Conversion *)
  
    Arguments 1-3 which conversions to perform

args 1-3: DNA to RNA | DNA to cDNA | RNA to protein 
   
    Argument 4 what is the output file name

 arg 4: output_filename.seq *)

(* Sample run 1 *)

./biostat.native DNA to RNA dna_rna.seq

Input a sequence or filename: ATGCGATCG

Write a comment (50 characters or less) and then hit enter.
my_comment1

(*Within the code directory, check for your dna_rna.seq file.*)

(* Sample run 2 *)
./biostat.native DNA to cDNA dna_cdna.seq

Input a sequence or filename: ATGCGATCG

Write a comment (50 characters or less) and then hit enter.
my_comment2

(*Within the code directory, check for your dna_cdna.seq file.*)

(* Sample run 3 *)
./biostat.native RNA to protein rna_protein.seq 

Input a sequence or filename: AAAUUUAUGCCCUUUAAAGGG

Write a comment (50 characters or less) and then hit enter.
my_comment1

Enter (W) to wait for a start (AUG) codon. Enter (S) to start translating right away.

(* Suggestion: Try Sample run 3 twice, one using W and one using S. Compare the lengths of the resulting amino acid sequences.*)

(*Within the code directory, check for your rna_protein.seq file.*)


(* Sample run 4 *)
./biostat.native RNA to protein rna_protein.seq 

Input a sequence or filename: convert_this_rna.seq

Write a comment (50 characters or less) and then hit enter.
my_comment1

Enter (W) to wait for a start (AUG) codon. Enter (S) to start translating right away.

(* Suggestion: Try Sample run 4 twice, one using W and one using S. Compare the number of sequences and the lengths of the resulting amino acid sequences.

If you wait for a start codon, you will see four short sequences because there are start and stop codons interspersed through the whole sequence. If you start right away, you will see two longer sequences since the number of bases before the first stop codon is not a multiple of three. As a result, there is a frameshift that keeps most of the stop codons from being read in.*)

(*Within the code directory, check for your rna_protein.seq file.*)





