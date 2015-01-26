(*Module for converting DNA to RNA, DNA to cDNA, and RNA to Protein.*)

module type CONVERTER = 
sig
  val matrix: char array array array
  val ribosome: string -> bool -> string
  val dna_rna: string -> string
  val to_cdna: string -> string
end


module SeqCon : CONVERTER = 
struct

  (*Amino Acid Codon Chart is stored in a 3D Array.*)
  (*first row*)
  let a1 = [|'F';'F';'L';'L'|];; (*Phenylalanine, Leucine*)
  let a2 = [|'S';'S';'S';'S'|];; (*Serine*)
  let a3 = [|'Y';'Y';'Z';'Z'|];; (*Tyrosine, Stop*)
  let a4 = [|'C';'C';'Z';'W'|];; (*Cystine, stop, stop, tryptophan*)

  (*second row*)	
  let b1 = [|'L';'L';'L';'L'|];; (* Leucine*)
  let b2 = [|'P';'P';'P';'P'|];; (*Proline*)
  let b3 = [|'H';'H';'Q';'Q'|];; (*His, Gln*)
  let b4 = [|'R';'R';'R';'R'|];; (*Arg*)   

  (*third row*)

  let c1 = [|'I';'I';'I';'M'|];; (*Ile,Met*)
  let c2 = [|'T';'T';'T';'T'|];; (*Threonine*)
  let c3 = [|'N';'N';'K';'K'|];; (*Asn,Lysn*)
  let c4 = [|'S';'S';'R';'R'|];; (*Cystine, stop, stop, tryptophan*)

  (*fourth row*)	
  let d1 = [|'V';'V';'V';'V'|];; (* Leucine*)
  let d2 = [|'A';'A';'A';'A'|];; (*Proline*)
  let d3 = [|'D';'D';'E';'E'|];; (*His, Gln*)
  let d4 = [|'G';'G';'G';'G'|];; (*Gly*)   

  (*create rows*)
  let row1 = [|a1;a2;a3;a4|];;
  let row2 = [|b1;b2;b3;b4|];;
  let row3 = [|c1;c2;c3;c4|];;
  let row4 = [|d1;d2;d3;d4|];;

  (*create codon chart*)
  let matrix = [|row1;row2;row3;row4|];;

(*converts DNA to RNA and RNA to DNA.*)
let dna_rna (seq:string) : string = 
  let _ = Printf.printf "%s - initial\n" seq in
  let seq = String.uppercase seq in
  let rec helper (count:int) = 
    if count >= String.length seq then 
    let _ = Printf.printf "%s - converted\n" seq in seq else
    let mychar = String.get seq count in
    match mychar with
    | 'T' -> String.set seq count 'U';helper (count+1)
    | 'U' -> String.set seq count 'T';helper (count+1)
    | _ -> helper (count+1) in
  helper 0;;

(*converts DNA to cDNA.*)
let to_cdna (seq:string) : string =
  let _ = Printf.printf "%s - initial\n" seq in 
  let seq = String.uppercase seq in
  let rec helper (count:int) = 
    if count >= String.length seq then 
    let _ = Printf.printf "%s - converted\n" seq in seq else
    let mychar = String.get seq count in
    match mychar with
    | 'A' -> String.set seq count 'T';helper (count+1)
    | 'T' | 'U' -> String.set seq count 'A';helper (count+1)
    | 'C' -> String.set seq count 'G';helper (count+1)
    | 'G' -> String.set seq count 'C';helper (count+1)
    | _ -> helper (count+1) in
  helper 0;;
  
 
  let ind_of_char char = 
  match char with 
  | 'U' -> 0
  | 'C' -> 1
  | 'A' -> 2
  | _ -> 3;;

  let ind_of_str sub pos = 
   ind_of_char (String.get sub pos);;
 
   
  let translate (arr:'a array array array) (codon:string)  = 
    let first = ind_of_str codon 0 in
    let secnd = ind_of_str codon 1 in
    let third = ind_of_str codon 2 in
   
    let two_d = Array.get arr first in
   
    let one_d = Array.get two_d secnd in

    let mychar = Array.get one_d third in
    mychar;;

(*converts a string of RNA bases into a protein list.*)
let to_protein_list (sequence:string) (wait:bool) : string list = 

    if String.length sequence < 3 then [] else

  let rec list_helper (subseq:string) (lst:string list) (wt:bool) : string list = 

    if String.length subseq < 3 then lst else

(*translate sequence to a protein*)
 let to_protein (seq:string) : string list = 
  (*if at end, stop*)
  if String.length seq < 3 then list_helper seq lst true else

  (*standardize sequence, get number of bases*)
  let seq = String.uppercase seq in
  let num_bases = String.length seq in

  (*get potential length of AA sequence & create string*)
  let length = num_bases/3 in
  let protein = String.create length in

  (*start helper; set index for base sequence based on AA count*)
  let rec prot_helper (count:int) : string list  = 
    let index = count*3 in
    if count >= (String.length seq)/3 then 
         let new_protein = String.sub protein 0 count in
         list_helper "" (lst @ [new_protein]) true else
   
     (*pull current codon & translate into amino acid*)
     let codon = String.sub seq index 3 in
     let aa = translate matrix codon in
     (*if at a stop codon, return the protein and remaining bases*)
     if aa = 'Z' then 
       let rem_bases = num_bases - (index+1) in
       let rem_seq = String.sub seq (index+1) rem_bases in
       (*edge case: if this is our first codon, don't count it; return the same list*)
       if count = 0 then list_helper rem_seq lst true else
       let new_protein = String.sub protein 0 count in
     

       (*return remaining base sequence / add new protein to the list*)
       list_helper rem_seq (lst @ [new_protein]) true
     else
      
     (*if not at STOP, add AA to string and recurse*)
     let _ = String.set protein count aa in prot_helper (count+1) in
  prot_helper 0 in

  (*scans the DNA base by base until an AUG codon is reached.*)
  let scan_seq (seq:string) : string list =
    let last  = (String.length seq) in
    let rec scan_helper (count:int) : string list = 
    if last - count < 3 then list_helper "" lst true else
    let codon = String.sub seq count 3 in
    match codon with 
    | "AUG" -> let start = count+3 in
               if last - start < 3 then list_helper "" lst true else
               let rem = last - start in
               let newseq = String.sub seq start rem in
               to_protein  newseq
    | _ -> scan_helper (count+1) 
    in scan_helper 0 in
  
   (*list_helper is calling this*)
   if wt = true then scan_seq subseq else to_protein subseq in
  
   (*main function is calling this*)
   list_helper sequence [] wait;;

  let proteins_to_string (lst:string list) : string = 
    let rec helper (my_list:string list) (count:int) : string = 
     
      match my_list with 
      | [] -> ""
      | hd::tl -> let num = string_of_int count in
                  (*carriage return before strings that are not the first*)
                  let head = if count = 1 then "" else "\n" in
                  head ^ hd ^ "\nend of seq " ^ num ^ helper tl (count+1)

    in helper lst 1;;

  (*the main function here. Named after the organelle that facilitates translation in the cell.*)
  let ribosome (str:string) (wait:bool): string = 
    let v = proteins_to_string (to_protein_list str wait) in
    Printf.printf "%s\n" v; v;;

end
 



